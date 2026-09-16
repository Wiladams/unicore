# OpenType Design Sheet: `hmtx`

## Purpose

`hmtx` provides nominal horizontal metrics for glyphs.

---

## 1. What mathematical/semantic function is implied?

Simply:

```text
HorizontalMetrics metrics(GlyphId glyph)
```

returning:

```text
advanceWidth
leftSideBearing
```

---

## 2. What information does the font contribute?

For each glyph, conceptually:

```text
advance width
left side bearing
```

---

## 3. What algorithm does the engine contribute?

Almost none:

```text
retrieve metrics
initialize nominal glyph advance
```

Afterward, GPOS may adjust those nominal values.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

OpenType may store fewer full horizontal metric records than there are glyphs.

Trailing glyphs can reuse the final stored advance width while supplying separate left side bearings.

That compact representation is an on-disk optimization, not a semantic distinction.

---

## 5. Which distinctions have genuine semantic or performance meaning?

The semantic values are simply:

```text
advanceWidth
leftSideBearing
```

Variation-dependent adjustments may later modify them through variable-font machinery.

---

## 6. What would the API look like if OpenType did not exist?

Simply:

```text
metrics(glyph)
```

---

## 7. What representation would I choose if I only cared about execution?

Possibly:

```text
HorizontalMetrics[glyph]
```

or parallel arrays:

```text
advance[glyph]
lsb[glyph]
```

For typical glyph counts, fully expanding the compact on-disk representation may be entirely reasonable.

---

## Compiler interpretation

```text
compressed hmtx
      |
      v
direct glyph metrics database
```

The key observation is:

> The compact storage format has almost no reason to survive into the shaping runtime.
