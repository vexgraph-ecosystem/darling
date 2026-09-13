# Contributing to darling-editor

All code in `darling-editor` must strictly comply with the overarching constitutional law of the `vexgraph` ecosystem codified in [preferences.md](../../preferences.md).

## Non-Negotiable Invariants
1. **Rule 1 (No Arrow Sugar)**: Always write `(*ptr).field`. Never use `->`.
2. **Rule 3 (One Class Per File)**: Each `.h`/`.c` pair contains exactly one class struct.
3. **Rule 10 (Two-Layer Access Cap)**: Maximum 2 member hops per expression (`(*a).b`).
4. **Rule 23 (Living Overview Blueprint)**: Implementation files begin with `;;OVERVIEW` detailing struct fields and function registry.
5. **Rule 24 (Symmetric Getters/Setters)**: Complete mutator and accessor pairs for every stored struct field.
