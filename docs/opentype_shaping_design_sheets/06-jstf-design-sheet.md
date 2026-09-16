# OpenType Design Sheet: `JSTF`

## Purpose

`JSTF` supplies script- and language-specific strategies that may be used when text must be expanded or compressed for justification.

It is not a complete paragraph justification algorithm.

---

## 1. What mathematical/semantic function is implied?

Conceptually:

```text
JustificationPlan justificationStrategies(script, language)
```

The engine wants to know:

> If this run must become wider or narrower, which shaping operations may be enabled, disabled, or adjusted, and in what priority order?

---

## 2. What information does the font contribute?

The font can supply prioritized suggestions such as:

```text
disable these GSUB lookups
enable these GSUB lookups

disable these GPOS lookups
enable these GPOS lookups

apply justification-specific positioning behavior

allow adjustments within specified limits
```

---

## 3. What algorithm does the engine contribute?

The layout engine contributes most of the high-level decision process:

```text
how much width must change?

expand or shrink?

which priority should be attempted first?

did that strategy produce enough change?

should the next strategy be tried?

how does this interact with whitespace and line breaking?
```

JSTF supplies candidate actions, not the overall justification engine.

---

## 4. Which serialized distinctions exist mainly because of compression or storage?

These can likely disappear:

```text
script/language table hierarchy
raw lookup indices
offset structure
array layout
```

A compiled form can reference stable semantic lookup IDs.

---

## 5. Which distinctions have genuine semantic or performance meaning?

These matter:

```text
priority order
expand vs shrink
enable vs disable
GSUB vs GPOS
maximum adjustment
script/language specificity
```

---

## 6. What would the API look like if OpenType did not exist?

Something like:

```text
Span<JustificationStage> justificationPlan(script, language)
```

where each stage contains semantic actions such as:

```text
disableSubstitutions
enableSubstitutions
disablePositioning
enablePositioning
maxShrink
maxExtend
```

---

## 7. What representation would I choose if I only cared about execution?

Something like:

```text
JustificationPlan
    Stage[]
        shrinkActions
        extendActions
```

with semantic lookup IDs and normalized adjustment limits.

---

## Compiler interpretation

```text
JSTF
   |
   v
ordered justification policy
```

The key observation is:

> JSTF is closer to a declarative policy program than to a simple database.
