# Unicode Coverage Architecture: Blend2D vs. UCDB

## Overview

Blend2D's new coverage container and the `UnicodeCoverage` implementation used by the UCDB project arrive at a very similar underlying geometry:

- fixed-size 1024-code-point leaf pages
- 128-byte bit pages
- a shallow directory/master/sub-page hierarchy
- direct address decomposition instead of searching a sparse sorted page list

The important difference is not primarily the page size or hierarchy. It is the intended workload.

Blend2D is implementing a **mutable runtime coverage/set container** intended for operations such as add, remove, test, add-range, remove-range, and iteration.

UCDB's `UnicodeCoverage` is primarily an **immutable, persistent Unicode property representation** built offline and consumed through direct memory mapping or `ByteSpan` access.

That distinction explains most of the differences between the two designs.

---

## 1. Common Geometry

Blend2D uses 1024-bit sub-planes, or 128 bytes per leaf page.

UCDB uses the same effective leaf geometry:

```cpp
struct UnicodeBitPage {
    uint64_t bits[16];
};
```

which gives:

```text
16 * 64 bits = 1024 bits = 128 bytes
```

The UCDB address decomposition is:

```text
Unicode code point
      |
      +-- master page:  cp >> 15
      |
      +-- sub-page:    (cp >> 10) & 31
      |
      +-- bit:          cp & 1023
```

Each master therefore covers:

```text
32 sub-pages * 1024 code points = 32768 code points
```

and the complete Unicode address space fits naturally into 34 master entries:

```text
34 * 32768 = 1,114,112 code points
```

This is the complete Unicode code-point range from U+000000 through U+10FFFF.

The significant architectural property is that page discovery is arithmetic rather than search-based.

---

## 2. Contrast With a Sorted Sparse Page Representation

A sparse-page implementation such as the one described for HarfBuzz conceptually performs:

```text
code point
    |
    +-- compute page number
    |
    +-- search sorted page directory
    |
    +-- locate page
    |
    +-- test bit
```

Even if the binary search is short, it introduces:

- comparisons
- branches
- indirect memory access
- potentially poor prediction behavior
- a reason to maintain a "last page accessed" cache

A direct hierarchy instead performs:

```text
code point
    |
    +-- compute master index
    +-- compute sub-page index
    +-- compute bit index
    |
    +-- direct indexed loads
    |
    +-- test bit
```

The hierarchy effectively behaves like a very small software page table.

This is particularly attractive for shaping workloads because text processing usually performs a very large number of small classification and membership queries.

---

## 3. Blend2D's Use Case

Blend2D's coverage object is fundamentally a **runtime set container**.

The benchmark operations illustrate the intended API:

```text
add
remove
test
add range
remove range
iteration / population operations
```

This makes mutation performance important.

The internal representation therefore needs to efficiently support:

- creation of previously absent pages
- mutation of individual bits
- mutation of ranges
- deletion or collapse of empty pages
- cardinality tracking
- traversal of populated regions

Kobalicek mentioned that a master-plane stores:

```text
indexes to sub-planes
+
cardinality of each sub-plane
```

Cardinality is especially useful in a mutable set implementation.

It can help with:

- determining when a page becomes empty
- determining when a page becomes full
- iteration
- set-size accounting
- deciding whether a page can be collapsed or replaced
- accelerating set operations

For Blend2D, the coverage structure is therefore closer to a high-performance specialized:

```cpp
Set<uint32_t>
```

with Unicode-friendly geometry.

That is a natural fit for shaping because shaping engines create many temporary or semi-temporary sets:

- glyph coverage sets
- lookup coverage
- feature coverage
- character sets
- glyph-class sets
- accelerator structures
- subsets generated from font tables

These sets can be modified dynamically during font loading, shaping, subsetting, or cache construction.

---

## 4. UCDB's Use Case

UCDB solves a different problem.

Its coverage representation is designed primarily for properties that are:

1. constructed offline,
2. immutable at runtime,
3. shared extensively,
4. accessed extremely frequently.

Examples include:

```text
Unicode Block
Script
Extended_Pictographic
other binary Unicode properties
```

The same master/sub-page geometry is also reused conceptually for value tables such as:

```text
General_Category
Canonical_Combining_Class
Bidi_Class
Grapheme_Cluster_Break
Indic_Conjunct_Break
Script
```

The runtime does not need to perform:

```text
add()
remove()
addRange()
removeRange()
```

on these objects.

That allows the database builder to spend substantially more work during generation so that runtime lookup becomes extremely simple.

---

## 5. Global Deduplication

The largest architectural difference is probably that UCDB can globally canonicalize pages.

During database construction, identical 128-byte bit pages are stored once:

```text
Coverage A ----+
Coverage B ----+
Coverage C ----+----> shared bit page #42
Coverage D ----+
```

Master pages can also be deduplicated.

This is possible because the runtime representation is immutable.

A mutable runtime container normally cannot freely share pages this way without introducing:

- copy-on-write
- reference counting
- ownership tracking
- mutation barriers

Those mechanisms would work against the simplicity and mutation speed Blend2D is targeting.

UCDB can therefore optimize for total database footprint rather than individual object ownership.

This becomes increasingly valuable as the number of Unicode properties grows.

---

## 6. Empty and Full Sentinels

UCDB also takes advantage of immutable canonical states.

A sub-page can be represented as:

```text
EMPTY
FULL
PARTIAL -> page reference
```

The master page additionally contains masks such as:

```cpp
uint32_t nonEmptyMask;
uint32_t fullMask;
```

This means many membership queries can be resolved without reading a bit page at all.

Conceptually:

```cpp
if (master.fullMask & subMask)
    return true;

if (!(master.nonEmptyMask & subMask))
    return false;

return bitPage.test(bit);
```

This is particularly useful for Unicode properties because large regions often contain:

- no members
- complete ranges
- repeated page patterns

Blend2D may derive equivalent information from per-page cardinality, but the UCDB representation can encode these states directly because its pages never change.

---

## 7. The UCDB Hierarchy Is More Than a Bitset

An important consequence of the UCDB design is that the hierarchy became a reusable Unicode address translation mechanism.

The same decomposition:

```text
master
sub-page
offset
```

can address different kinds of leaf data.

For example:

```text
UnicodeCoverage
    -> 1024 bits

UnicodeValueTable8
    -> 1024 uint8_t values

possible future tables
    -> uint16_t values
    -> packed enums
    -> script-set references
    -> shaping metadata references
```

So the master/sub-page hierarchy is not merely a bitset optimization.

It acts more like a compact Unicode virtual-memory layout.

That is especially useful in a shaping pipeline because many classifications need exactly the same address decomposition.

---

## 8. Implications for Shaping

The two approaches are complementary.

### Blend2D-style mutable coverage

Best suited for data that is produced or modified at runtime:

```text
font cmap coverage
glyph sets
GSUB/GPOS lookup coverage
temporary shaping sets
font-subsetting sets
runtime cache structures
```

The useful properties are:

- fast mutation
- fast membership
- direct indexing
- inexpensive range insertion
- efficient iteration
- no binary search for page discovery

### UCDB-style immutable tables

Best suited for stable Unicode semantics:

```text
Script
Script_Extensions / script sets
Bidi_Class
General_Category
Combining_Class
Grapheme_Cluster_Break
Indic_Conjunct_Break
Extended_Pictographic
normalization data
other Unicode-derived properties
```

The useful properties are:

- mmap-friendly representation
- no object construction at runtime
- global page deduplication
- shared canonical pages
- EMPTY/FULL sentinels
- compact references
- deterministic lookup cost

A shaping engine potentially benefits from using both kinds of structure.

---

## 9. A Possible Shaping Pipeline View

Conceptually, a shaping system can divide its data into two classes.

### Static semantic data

```text
UTF-8
  |
  v
Unicode code point
  |
  +--> UCDB Script
  +--> UCDB Bidi_Class
  +--> UCDB CCC
  +--> UCDB Grapheme_Break
  +--> UCDB Indic_Conjunct_Break
  +--> UCDB Extended_Pictographic
```

These tables are immutable and shared globally.

### Dynamic font/shaping data

```text
font
  |
  +--> cmap coverage
  +--> GSUB lookup coverage
  +--> GPOS lookup coverage
  +--> glyph classes
  +--> temporary glyph sets
```

These naturally benefit from a mutable Blend2D-style coverage object.

The common 1024-code-point/page geometry means both layers can have similarly predictable cache behavior and extremely cheap address calculations.

---

## 10. Why This Matters in a Shaper

A text shaper repeatedly asks questions of the form:

```text
What is this character?
Does this property contain it?
Does this font contain it?
Does this lookup apply here?
What class does this glyph belong to?
```

The individual operation is tiny.

Therefore, the overhead surrounding the lookup can dominate the actual useful work.

Replacing:

```text
search -> compare -> branch -> locate page -> test
```

with:

```text
shift -> mask -> indexed load -> test
```

is exactly the type of optimization that accumulates over millions of classification operations.

The Blend2D benchmark appears consistent with this: the improvement over HarfBuzz is meaningful but not enormous, generally around the scale expected from removing page-search overhead while leaving the final bitmap operation essentially unchanged.

---

## 11. Different Optimization Targets

The two implementations can be summarized as optimizing opposite sides of the same problem.

### Blend2D

Optimizes:

```text
mutable runtime Unicode/glyph sets
```

Priorities:

```text
mutation speed
set operations
range operations
runtime construction
cardinality
iteration
```

### UCDB

Optimizes:

```text
immutable shared Unicode knowledge
```

Priorities:

```text
storage density
deduplication
mmap access
zero runtime construction
shared pages
predictable classification lookup
```

The hierarchy is similar because the underlying addressing problem is the same.

The metadata and ownership strategies differ because the lifetime and mutation models differ.

---

## 12. Architectural Convergence

The interesting result is that both designs independently converge on:

```text
128-byte leaf pages
1024 code points per page
shallow direct indexing
master/sub-page decomposition
no sorted page search in the hot lookup path
```

That suggests this geometry is a strong general-purpose solution for Unicode and shaping workloads.

The remaining differences are largely consequences of mutability.

Blend2D treats coverage as an active runtime container.

UCDB treats coverage as compiled data.

For a complete shaping engine, those are not competing designs. They occupy different layers of the system and can coexist naturally:

```text
          STATIC UNICODE SEMANTICS
                 UCDB
                  |
                  v
text --> classification/itemization
                  |
                  v
          DYNAMIC FONT SEMANTICS
       Blend2D-style coverage
                  |
                  v
               shaping
```

The common lesson is that Unicode's bounded address space is small enough that a shallow direct-index hierarchy is often preferable to storing populated pages in a general sorted sparse container.

For shaping in particular, predictable direct lookup is attractive because property tests and coverage queries occur at extremely high frequency, while the address space itself is fixed and known in advance.
