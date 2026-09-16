# OpenType Design Sheet: `cmap`

## Purpose

`cmap` maps Unicode code points, or other character codes, to glyph IDs in the font.

---

## 1. What mathematical/semantic function is implied?

Conceptually:

```text
GlyphId glyphForCodePoint(CodePoint cp)
```

For Unicode variation sequences:

```text
GlyphId glyphForVariation(CodePoint cp, VariationSelector vs)
```

The various `cmap` formats are serialized representations of this mapping.

---

## 2. What information does the font contribute?

The font contributes mappings such as:

```text
U+0041  -> glyph 36
U+0E01  -> glyph 421
U+1F600 -> glyph 1932
```

It also contains platform/encoding metadata describing which character maps are available.

A missing character normally maps to glyph 0 (`.notdef`).

---

## 3. What algorithm does the engine contribute?

Very little semantically:

```text
select the appropriate cmap
look up the code point
return the glyph ID
```

Most implementation complexity exists only because the serialized formats are compact and historically varied.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

Examples include:

```text
Format 0
Format 2
Format 4
Format 6
Format 10
Format 12
Format 13
```

These are largely alternate ways to encode sparse mappings, ranges, deltas, or historical character encodings.

`Format 14` is somewhat different because it carries Unicode variation-selector mappings.

---

## 5. Which distinctions have genuine semantic or performance meaning?

The important semantic distinctions are:

```text
Unicode mapping
legacy/non-Unicode mapping
Unicode variation-sequence mapping
```

The density and distribution of mappings may also influence the most efficient runtime representation.

---

## 6. What would the API look like if OpenType did not exist?

Something close to:

```text
glyphForCodePoint(cp)
glyphForVariation(cp, variationSelector)
```

No OpenType format numbers would be visible.

---

## 7. What representation would I choose if I only cared about execution?

A sparse Unicode lookup structure, for example:

```text
Unicode scalar
    -> sparse page
    -> glyph ID
```

with a separate structure for variation-selector mappings.

The runtime should care about the mapping, not whether it originally came from `cmap` Format 4, 12, or another encoding.

---

## Compiler interpretation

```text
cmap formats
    |
    v
CharacterMap
    |
    v
code point -> glyph ID
```

The key observation is:

> The `cmap` table is a compressed serialization of a simple semantic function.
