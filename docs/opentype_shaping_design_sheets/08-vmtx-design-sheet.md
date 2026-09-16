# OpenType Design Sheet: `vmtx`

## Purpose

`vmtx` provides nominal vertical metrics for glyphs.

It is the vertical analogue of `hmtx`.

---

## 1. What mathematical/semantic function is implied?

Conceptually:

```text
VerticalMetrics metrics(GlyphId glyph)
```

returning:

```text
advanceHeight
topSideBearing
```

---

## 2. What information does the font contribute?

For each glyph:

```text
advance height
top side bearing
```

---

## 3. What algorithm does the engine contribute?

Almost none:

```text
retrieve nominal vertical metrics
initialize vertical advance
```

GPOS may subsequently adjust vertical positioning.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

Like `hmtx`, repeated advance heights may be compacted so that trailing glyphs reuse the final stored advance while retaining separate top side bearings.

That compact form does not need to survive into runtime semantics.

---

## 5. Which distinctions have genuine semantic or performance meaning?

The actual semantic values are:

```text
advanceHeight
topSideBearing
```

plus any variable-font adjustments.

---

## 6. What would the API look like if OpenType did not exist?

Simply:

```text
verticalMetrics(glyph)
```

---

## 7. What representation would I choose if I only cared about execution?

Probably:

```text
VerticalMetrics[glyph]
```

or parallel arrays:

```text
advanceHeight[glyph]
topSideBearing[glyph]
```

---

## Compiler interpretation

```text
compressed vmtx
      |
      v
direct vertical metrics database
```

The key observation is:

> `vmtx` is a compact serialized glyph-property database, not a shaping algorithm.
