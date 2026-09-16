# OpenType Design Sheet: `GDEF`

## Purpose

`GDEF` provides glyph-role information and supporting metadata used by OpenType layout processing.

It does not normally perform substitutions or positioning itself. Instead, it helps GSUB/GPOS interpret glyphs correctly.

---

## 1. What mathematical/semantic function is implied?

Several supporting queries are implied:

```text
glyphClass(glyph)
markAttachmentClass(glyph)
markSetContains(set, glyph)
attachmentPoints(glyph)
ligatureCarets(glyph)
variationValue(index, coords)
```

---

## 2. What information does the font contribute?

GDEF may contribute:

```text
glyph -> Base
glyph -> Ligature
glyph -> Mark
glyph -> Component

mark -> attachment class

mark filtering sets

attachment points

ligature caret positions

variation-store data
```

---

## 3. What algorithm does the engine contribute?

The engine interprets the metadata while executing GSUB/GPOS:

```text
should this glyph be ignored by the lookup?

is this glyph a mark?

does this mark belong to the required attachment class?

does this glyph belong to the lookup's mark filtering set?

where are valid caret positions inside this ligature?
```

The shaping executor supplies the policy for consuming these properties.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

These can usually disappear during compilation:

```text
ClassDef formats
Coverage formats
offset topology
MarkGlyphSets storage layout
variation-store serialization details
```

---

## 5. Which distinctions have genuine semantic or performance meaning?

These do matter:

```text
Base
Ligature
Mark
Component

mark attachment class

mark filtering-set membership

attachment points

ligature caret positions
```

The variation semantics also matter, even though their serialized form does not.

---

## 6. What would the API look like if OpenType did not exist?

Something like:

```text
GlyphRole role(glyph)

uint16_t markAttachmentClass(glyph)

bool belongsToMarkSet(setId, glyph)

Span<AttachmentPoint> attachmentPoints(glyph)

Span<CaretPosition> ligatureCarets(glyph)
```

However, the shaping executor may not need to expose all of these directly.

---

## 7. What representation would I choose if I only cared about execution?

Conceptually:

```text
GlyphSemanticInfo[glyph]
    role
    markAttachmentClass
```

plus sparse/shared structures such as:

```text
GlyphSet[]
AttachmentPointMap
LigatureCaretMap
VariationStore
```

Much of GDEF can potentially disappear into already-compiled lookup state:

```text
GDEF metadata
+
LookupFlag
    |
    v
CompiledLookupFilter
```

---

## Compiler interpretation

```text
raw GDEF
   |
   +--> compiled lookup filters
   +--> attachment metadata
   +--> caret/editing metadata
   +--> variation semantics
```

The key observation is:

> GDEF is best understood as a glyph-semantics database and compiler input, not as a standalone shaping algorithm.
