# OpenType Design Sheet: `BASE`

## Purpose

`BASE` supplies baseline information that allows different scripts, sizes, and fonts to align typographically.

---

## 1. What mathematical/semantic function is implied?

Conceptually:

```text
Coordinate baseline(script, language, baselineType, direction)
```

and potentially:

```text
Extent minExtent(...)
Extent maxExtent(...)
```

---

## 2. What information does the font contribute?

The font may provide:

```text
baseline types
baseline coordinates
per-script baseline choices
language-specific information
horizontal-layout baseline data
vertical-layout baseline data
minimum and maximum extents
variation-dependent coordinates
```

---

## 3. What algorithm does the engine contribute?

The text-layout engine decides:

```text
which baseline is appropriate
which runs need alignment
how much each run must be translated
how baseline alignment participates in line layout
```

`BASE` supplies measurements and relationships. It does not itself move glyphs.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

Candidates include:

```text
BaseCoord format variants
offset hierarchy
TagList serialization
script-record organization
variation-reference encoding
```

These can be normalized into a direct semantic model.

---

## 5. Which distinctions have genuine semantic or performance meaning?

Important distinctions include:

```text
horizontal vs vertical layout
script
language
baseline type
baseline coordinate
min/max extents
```

---

## 6. What would the API look like if OpenType did not exist?

Something like:

```text
baselineCoordinate(script, language, baselineKind, direction)

defaultBaseline(script, direction)

extent(script, language, feature, direction)
```

---

## 7. What representation would I choose if I only cared about execution?

A small relational structure such as:

```text
Script
    -> baseline kind -> coordinate

Script + Language
    -> optional overrides

Script + Feature
    -> optional min/max extents
```

The total data is typically small compared with GSUB/GPOS.

---

## Compiler interpretation

```text
BASE serialized hierarchy
        |
        v
BaselineDatabase
        |
        v
script/language -> alignment geometry
```

The key observation is:

> BASE is structured relational data, not a shaping program.
