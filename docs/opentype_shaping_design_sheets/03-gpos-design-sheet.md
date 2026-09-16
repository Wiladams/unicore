# OpenType Design Sheet: `GPOS`

## Purpose

`GPOS` describes glyph positioning behavior: individual adjustments, pair positioning, cursive attachment, mark attachment, and contextual positioning.

---

## 1. What mathematical/semantic function is implied?

Conceptually:

```text
position(GlyphBuffer, Geometry, FeaturePlan)
```

GPOS can change:

```text
x placement
y placement
x advance
y advance
attachment relationships
```

---

## 2. What information does the font contribute?

The font contributes:

```text
single-glyph adjustments
explicit pair adjustments
class-based pair adjustments

cursive entry anchors
cursive exit anchors

mark anchors
base anchors

ligature-component anchors

mark-to-mark anchors

contextual positioning rules

variation-dependent values
```

---

## 3. What algorithm does the engine contribute?

The executor supplies operations such as:

```text
find eligible glyphs
apply lookup filtering
match pair or contextual patterns
retrieve semantic adjustments
modify placement and advances

match mark classes
resolve mark/base anchors
establish attachment relationships

resolve cursive entry/exit attachment

execute nested contextual positioning lookups
```

The font contributes geometry and applicability. The executor contributes the generic positioning algorithms.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

Likely candidates include:

```text
Coverage Format 1 / 2
ClassDef Format 1 / 2
ExtensionPos

Context Format 1 / 2 / 3
ChainContext Format 1 / 2 / 3

raw offsets
subtable boundaries
anchor encoding details
```

`PairPos Format 1` and `PairPos Format 2` may also collapse to one semantic operation while retaining different optimized storage representations.

---

## 5. Which distinctions have genuine semantic or performance meaning?

These are real semantic operations:

```text
Single adjustment
Pair adjustment
Cursive attachment
Mark-to-base attachment
Mark-to-ligature attachment
Mark-to-mark attachment
Contextual positioning
Chained contextual positioning
```

Also meaningful:

```text
anchor class
ligature component
lookup order
lookup filtering
variation-dependent values
```

For performance, an explicit pair map and a class-by-class matrix may deserve different backing structures even if they implement the same abstract operation.

---

## 6. What would the API look like if OpenType did not exist?

Conceptually:

```text
adjust(glyph, adjustment)

adjustPair(left, right,
           leftAdjustment,
           rightAdjustment)

attachCursive(left, right,
              exitAnchor,
              entryAnchor)

attachMarkToBase(mark, base,
                 markAnchor,
                 baseAnchor)

attachMarkToLigature(mark, ligature,
                     componentIndex,
                     markAnchor,
                     componentAnchor)

attachMarkToMark(mark, previousMark,
                 markAnchor,
                 baseAnchor)

applyContextRule(...)
```

---

## 7. What representation would I choose if I only cared about execution?

Likely semantic structures such as:

```text
ValueAdjustment

PairMap
PairClassMatrix

Anchor

MarkAttachmentRule
CursiveAttachmentRule

ContextRule
ChainContextRule

VariationValue
```

The glyph buffer may also carry temporary attachment relationships so that dependent glyph positions can be resolved cleanly.

---

## Compiler interpretation

```text
GPOS serialization
       |
       v
geometry + attachment semantics
       |
       v
small positioning executor
```

The central design question is:

> Which GPOS distinctions reflect true positioning semantics, and which exist only because OpenType needed compact binary encodings?
