# OpenType Design Sheet: `GSUB`

## Purpose

`GSUB` describes glyph substitution behavior: single substitutions, multiple substitutions, alternates, ligatures, contextual substitutions, chained contextual substitutions, and reverse-chain substitutions.

---

## 1. What mathematical/semantic function is implied?

Approximately:

```text
GlyphSequence' = substitute(GlyphSequence, FeaturePlan)
```

A more realistic runtime API is:

```text
applyLookup(LookupId, MutableGlyphBuffer)
```

because GSUB operates incrementally on a mutable glyph stream.

---

## 2. What information does the font contribute?

The font contributes rules such as:

```text
A -> A.alt

f i -> fi

glyph X -> glyphs A B C
```

and contextual rules such as:

```text
if preceding glyphs match P
and input glyphs match I
and following glyphs match F
then invoke lookup L at sequence position N
```

It also contributes:

```text
script/language -> features
feature -> ordered lookups
lookup flags
coverage sets
glyph classes
```

---

## 3. What algorithm does the engine contribute?

The executor supplies the generic behavior:

```text
matching
lookup filtering
glyph-buffer traversal
buffer mutation
nested lookup execution
sequenceIndex semantics
recursion limits
operation limits
transactional rollback
```

The font supplies the rules. The engine supplies the execution model.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

These can disappear during semantic compilation:

```text
Coverage Format 1 / 2
ClassDef Format 1 / 2

Context Format 1 / 2 / 3
ChainContext Format 1 / 2 / 3

ExtensionSubst
raw offsets
subtable boundaries
binary lookup-index representation
```

They are encoding choices, not fundamental shaping semantics.

---

## 5. Which distinctions have genuine semantic or performance meaning?

These remain meaningful:

```text
SingleSubst
MultipleSubst
AlternateSubst
LigatureSubst
ContextSubst
ChainContextSubst
ReverseChainSingleSubst
```

Also semantically important:

```text
lookup order
feature order
lookup filtering
nested lookup invocation
mutable sequenceIndex behavior
```

---

## 6. What would the API look like if OpenType did not exist?

Something like:

```text
featureLookups(FeatureId)
applyLookup(LookupId, GlyphBuffer)
```

with semantic lookup payloads such as:

```text
SingleSubstitution
MultipleSubstitution
AlternateSubstitution
LigatureSubstitution
ContextRule
ChainContextRule
ReverseChainRule
```

---

## 7. What representation would I choose if I only cared about execution?

A semantic shaping IR containing:

```text
Feature -> LookupId[]

Lookup
    filter
    semantic payload

GlyphSet
GlyphClassMap
ContextRule
ChainContextRule
ReverseChainRule
stable LookupId
```

The runtime should never need to know about Coverage formats, ClassDef formats, Extension lookups, or raw table offsets.

---

## Compiler interpretation

```text
GSUB binary rule structures
          |
          v
semantic substitution program
          |
          v
small glyph-buffer executor
```

The central observation is:

> GSUB is better understood as a serialized declarative rule program than as a collection of binary tables.
