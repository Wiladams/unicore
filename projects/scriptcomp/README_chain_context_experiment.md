# ChainContext accept-state experiment

This extends the earlier predicate-trie experiment.

## Goal

The first trie run showed that GSUB ChainContext input sequences collapse heavily:
many source rules reach the same accepting input state. This experiment measures
what remains after that input recognition step.

For every accepting input-trie node it reports:

- number of candidate source rules
- distinct backtrack patterns
- distinct lookahead patterns
- distinct `(backtrack, lookahead)` constraint pairs
- duplicate rules with the same complete constraint pair
- backtrack predicate positions and trie compression
- lookahead predicate positions and trie compression

The largest accept states are printed first.

## Invocation

```cpp
testOpenTypeShapingChainContextAnalysisExperiment("w:/fonts/google/fonts");
```

The test currently runs:

- Noto Serif Dives Akuru
- Noto Nastaliq Urdu

## Interpretation

The key questions are:

1. After the input trie accepts, how many candidate rules remain?
2. Do those candidates collapse to only a few distinct backtrack/lookahead patterns?
3. Do backtrack and lookahead sequences themselves have strong prefix sharing?
4. Are `(backtrack, lookahead)` pairs nearly unique, or are many rules still duplicates
   at the full constraint level?

If candidate clusters are large but the constraint tries are compact, the likely
runtime architecture is:

```text
input trie
    -> accepting state
        -> backtrack constraint trie
        -> lookahead constraint trie
        -> ordered semantic actions
```

If the constraint-pair count stays nearly equal to the source-rule count, a more
combined decision graph may be appropriate instead.

## Files

- `opentype_shaping_rule_trie.h`
  Existing input predicate-trie compiler from the previous experiment.

- `opentype_shaping_chain_context_analysis.h`
  New accept-state and constraint-pattern analyzer.

- `test_opentype_shaping_chain_context_analysis.h`
  Test harness selecting the largest GSUB ChainContext subtable in each test face.

The analysis header has been syntax-checked under C++20 against the current
`opentype_shaping_ir.h`. The full test harness depends on the normal project test
and font-loading headers and should be compiled in the project tree.
