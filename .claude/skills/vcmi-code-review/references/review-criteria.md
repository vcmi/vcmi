# VCMI Code Review Criteria

Review the change against its intended behavior, VCMI's invariants, and the regressions its tests actually prevent. A task specification improves the intent comparison but is optional.

## Establish the baseline

If a specification is supplied, extract whatever it actually states:

- intended behavior and observable outcomes;
- explicit exclusions and compatibility requirements;
- architectural or implementation constraints;
- requested files or subsystems, if any;
- verification criteria.

Do not require a fixed specification schema. Do not treat the specification as infallible: flag an implementation that follows it but violates a stronger correctness, safety, or repository invariant.

Every finding must trace to the supplied specification, an established contract or invariant, or concrete code behavior. Do not invent requirements. When the specification is incomplete or unconventional, use the evidence it provides, disclose consequential gaps, and continue the review instead of treating missing context as either failure or approval.

Without a specification, infer intent conservatively from, in descending order of usefulness:

1. public API and data-format contracts;
2. player- and modder-facing documentation;
3. callers and consumers of the changed behavior;
4. analogous implementations and established repository patterns;
5. configuration schemas, constants, and game-domain invariants;
6. tests and names as supporting evidence, never sole proof;
7. the implementation itself.

Do not reason circularly from “the implementation returns X” to “the expected value is X.” If the available evidence permits multiple interpretations, report only risks that remain under those interpretations or mark the finding `SUSPECTED`.

## Seven dimensions

### 1. Specification Adherence

When a specification exists, compare each meaningful commitment with the implementation:

- `MATCH`: implemented with the specified observable behavior.
- `DRIFT`: implemented behavior differs materially.
- `MISSING`: required behavior is absent.
- `EXTRA`: behavior contradicts or materially exceeds the declared scope.

Judge semantics, not whether named files or wording match. Grade `FAIL` for missing behavior or major drift, `WARNING` for minor drift, and `PASS` when the commitments are met.

Without a specification, grade this dimension `NOT ASSESSED`. Inferred intent belongs in the other dimensions and must not be presented as a contractual requirement.

### 2. Scope Discipline

With a specification, look for unrelated additions, omitted required work, speculative flexibility, and contradictions with explicit exclusions. Without one, judge only whether the diff is cohesive and whether code is unused, duplicated, or unsupported by a current caller.

Search the whole checkout for existing functionality before accepting a new helper. Flag parameters, flags, virtual methods, enum values, configuration, or abstractions added for hypothetical consumers. Do not demand a shared abstraction for code paths likely to diverge.

A rewrite is not scope creep merely because it is large. Judge whether it serves the stated change and leaves the code more maintainable.

### 3. Safety & Quality

Inspect four categories:

- **Correctness:** lifetime and ownership bugs, invalid state, boundary errors, nondeterminism, and unit or coordinate-space mistakes.
- **Performance:** unsuitable complexity, repeated expensive work, allocations or I/O in hot paths, unbounded per-frame work, and missed caching, batching, or instancing opportunities. Focus on plausible hot paths such as per-unit battle loops, AI evaluation, and per-frame rendering; do not optimize rare work.
- **Reliability:** resource leaks and missing error handling at meaningful file, asset, engine, or platform boundaries.
- **Conditional risks:** inspect concurrency, determinism, serialization, numerical stability, platform compatibility, and security when the changed subsystem makes them relevant.

Apply these VCMI rules when relevant:

- **Determinism:** gameplay results must agree across clients. Flag unseeded RNG, gameplay-dependent iteration over unordered containers, and floating-point math that can affect gameplay.
- **Serialization:** preserve older-save compatibility with version-gated handling when layouts change. Treat save and network data as trust boundaries and produce contextual errors for invalid input.
- **Error origin:** internal invariant failures should fail fast; invalid mod/content data should log mod, entity, and field context and skip or default without preventing startup; illegal player actions should be blocked in the UI and rejected by the server if received.
- Never silently swallow errors or catch all exceptions and continue as if successful. Prefer APIs that expose invalid access; add context at trust boundaries instead of relying on bare container exceptions.
- Do not add logging inside hot per-unit or per-frame loops.

Do not spend review findings repeating diagnostics reliably provided by existing tooling, including mechanical cross-DLL linkage checks and routine static-analysis findings. Report them only when tooling cannot see the semantic problem or the issue has a concrete behavioral consequence.

### 4. Architecture

Check dependency direction and placement across `lib`, `server`, `client`, rendering backends, apps, scripts, and AI modules.

Apply relevant subsystem invariants:

- AI and player-facing code accesses game state through the appropriate callbacks.
- Game-state mutation and request validation remain server-side.
- Shared serialized types follow the project's serialization and linkage conventions.
- Bonus behavior respects DAG inheritance, explicit propagators, and descendant limiters.
- Battle rules belong in core battle logic; attack damage calculation belongs in `scripts/damage/damageCalculator.lua`.
- MainGUI, runNetwork, runServer, and TBB AI work do not cross unsafe ownership or thread-affinity boundaries.
- Backend-facing changes account for SDL2 and SDL3 implementations when both expose the affected behavior.
- Consumers link the `vcmi` facade rather than reaching around it to `vcmiMain`.

Grade `FAIL` for a boundary violation likely to cause incorrect state, protocol/save incompatibility, or unsafe execution. Use `WARNING` for maintainability-level architectural drift.

### 5. Pattern Consistency

Compare changed code with 1–2 relevant sibling implementations. Check substantive differences in naming, error handling, ownership, serialization, network packs, config loading, tests, and platform/backend handling.

Scale this comparison to the change. A small, localized diff usually needs only the closest analogue; investigate more broadly when risk, novelty, or ambiguity warrants it.

Prefer existing constants and identifiers over magic values: numeric entity IDs live in `lib/constants/EntityIdentifiers.h`, string IDs in `lib/constants/StringConstants.h`, gameplay sizes and counts in `lib/constants/NumericConstants.h`, and mechanic states in `lib/constants/Enumerations.h`. Follow existing JSON schemas for configuration. Keep comments minimal and technical: document contracts or non-obvious reasons, not the code's visible mechanics. `///` documents the following declaration and `///<` the member on the same line.

Every non-logging human-readable string requires an i18n key, with English text supplied through the established code or `english.json` path.

Flag only differences that increase defect risk or maintenance cost. Formatting and ordinary static-analysis issues belong to tooling.

Player- and modder-facing documentation must stay current when observable behavior, configuration, scripting, or data formats change. Do not request updates under `docs/developers/`; those pages are not actively maintained. Lua reference pages under `docs/modders/Lua_Reference/` are generated: update binding descriptions in `luascript/api/` and regenerate instead of editing generated pages.

### 6. Test Quality & Coverage

#### Determine expected behavior independently

Use the specification when present. Otherwise reconstruct the business and game invariants from contracts, callers, analogous behavior, configuration, and domain rules. Tests can clarify intent but do not become correct merely because they match the implementation.

Do not calculate expected results with the same algorithm or production helper used by the code under test. That arrangement can reproduce the same defect on both sides of the assertion.

#### Examine usefulness

For each changed test or directly relevant existing test, grouping parameterized cases logically, determine:

- whether setup reaches the scenario named by the test;
- whether assertions check an observable result rather than internal calls, private details, or incidental representation;
- whether the assertion is discriminating enough to fail on an incorrect result;
- whether null/empty data, invalid input, dependency failure, boundaries, or other risk-derived cases are covered when relevant;
- whether mocks isolate real boundaries without replacing the behavior being tested;
- whether each test protects a distinct behavior.

Avoid demanding multiple near-identical tests. Flag duplication only when cases catch the same regression and the duplication adds maintenance cost; parameterization is useful when it improves clarity, not as an end in itself.

Do not demand a regression test merely to memorialize a particular coding mistake. The scenario must represent behavior that matters independently of knowing how the bug was written.

#### Name the regression caught

For each changed or directly relevant test group, identify a plausible change in production code that would make it fail. Use this mutation question to classify weak tests:

- **SHALLOW TEST:** verified evidence shows the test still passes when its claimed behavior is wrong, or it asserts only implementation details.
- **DUPLICATE TEST:** another test catches the same meaningful regression without a distinct boundary or outcome.
- **UNCOVERED BEHAVIOR:** changed behavior or a risk-derived path has no test capable of detecting its regression.
- **MISSING REQUIRED TEST:** the supplied specification explicitly commits to a test that is absent.
- **FAILING TEST:** a relevant executed test fails because of the change.

If a concrete killed regression cannot be named but the evidence is insufficient to prove the test weak, mark the concern `SUSPECTED` and explain the missing context. Do not call it a verified defect.

Missing tests are a `FAIL` when explicitly required or when the untested change risks save corruption, protocol incompatibility, invalid authoritative state, or a similarly severe regression. Use `WARNING` for meaningful uncovered behavior. Do not flag trivial constants, mechanical wiring, or behavior already exercised adequately elsewhere.

### 7. Verification

Run the smallest relevant existing tests or checks when practical. If the specification contains success criteria, evaluate those that are safe and locally available without imposing a fixed format.

Record each command and its result. A check that runs and exposes a change-related failure is `FAIL`; an environment or tooling failure is an unverified limitation instead. Do not run a full build or test suite when a targeted command provides the needed signal.

Do not claim to verify visual, multiplayer, platform, or performance behavior that was not exercised.

## Findings

Assign severity:

- ❌ **CRITICAL:** likely crash, corruption, security issue, save/protocol incompatibility, invalid authoritative game state, deterministic divergence, or central task failure.
- ⚠️ **WARNING:** concrete correctness, regression, test, maintainability, or documentation problem that should be resolved.
- 👁 **OBSERVATION:** non-blocking but useful evidence, uncertainty, or localized improvement. Omit preference-only comments.

Always pair the icon with its severity word.

Every finding also states:

- **Status:** `VERIFIED` when demonstrated from code or a check; `SUSPECTED` when evidence is incomplete.
- **Impact:** `LOW` for an obvious narrow fix, `MEDIUM` for a real tradeoff or non-trivial edit, and `HIGH` for architectural stakes or broad blast radius.

Severity describes the consequence if the problem is ignored. Impact describes how much effort or judgment the fix decision requires; the two are independent.

Default to one fix. For `LOW` impact, give one direct fix. For `MEDIUM` or `HIGH`, include strength, tradeoff, confidence, and blind spot. Offer two fixes only for a genuine tradeoff between credible approaches, and mark exactly one recommended.

Findings need a specific failure scenario or violated invariant. Avoid generic “could be cleaner,” speculative defensive coding, style preferences, or restating tool output. Consolidate repetitions and cap the report at 10 findings.

If a finding concerns a credential, token, or other secret, redact the value. Describe its location and identifying pattern without reproducing it.

## Scorecard and verdict

Use `PASS`, `WARNING`, `FAIL`, or `NOT ASSESSED` for every dimension:

| Dimension | Typical grade |
|---|---|
| Specification Adherence | `NOT ASSESSED` without a specification; otherwise grade drift |
| Scope Discipline | Grade cohesion, unnecessary work, and declared scope |
| Safety & Quality | `FAIL` on a verified critical finding; `WARNING` on warning-severity findings |
| Architecture | `FAIL` on dangerous boundary violations; otherwise `WARNING` for substantive drift |
| Pattern Consistency | Usually `WARNING`; `FAIL` only when the mismatch breaks behavior or compatibility |
| Test Quality & Coverage | Grade missing, shallow, duplicate, uncovered, and failing tests by risk |
| Verification | `PASS` when relevant checks ran successfully; `WARNING` when important checks could not run; `FAIL` on a change-related failure; `NOT ASSESSED` when no meaningful runnable check was identified |

Overall verdict:

- **APPROVED:** no `FAIL` dimensions, no `CRITICAL` findings, and at most two `LOW`-impact `WARNING` findings.
- **NEEDS ATTENTION:** no rejection condition applies, but there is a `FAIL` dimension, a `MEDIUM`- or `HIGH`-impact `WARNING`, more than two `LOW`-impact `WARNING` findings, or an important suspected issue.
- **REJECTED:** a verified critical finding, central specification failure, deterministic divergence, data-safety issue, relevant tests failing because of the change, or missing tests explicitly required for high-risk behavior.

`OBSERVATION` findings do not change the overall verdict by themselves.

`NOT ASSESSED` is not a pass. Qualify the verdict, for example: “APPROVED based on code and tests; Specification Adherence was not assessed.”

## Canonical output example

This example is synthetic. Match its structure, not its subject matter; replace the illustrative location with a clickable path into the current checkout.

```markdown
## Findings

### F1 — Resource loaded on every frame

- **Severity:** ⚠️ WARNING
- **Status:** VERIFIED
- **Impact:** MEDIUM — the fix changes resource lifetime and ownership
- **Dimension:** Safety & Quality
- **Location:** `client/widgets/ExampleWidget.cpp:84`
- **Detail:** `showAll` loads the same animation on every frame. The render loop therefore repeats resource lookup and allocation, while sibling widgets retain the animation handle after construction.
- **Fix:** Load the animation during widget initialization and retain its handle for the widget lifetime.
  - Strength: Matches sibling widget ownership and removes repeated work from the render loop.
  - Tradeoff: Keeps the animation resident for the widget lifetime.
  - Confidence: HIGH — the call site is per-frame and the loader returns a reusable handle.
  - Blind spot: No profiler was run, so the runtime cost was not measured.

## Scorecard

| Dimension | Verdict |
|---|---|
| Specification Adherence | NOT ASSESSED |
| Scope Discipline | PASS |
| Safety & Quality | WARNING |
| Architecture | PASS |
| Pattern Consistency | PASS |
| Test Quality & Coverage | NOT ASSESSED |
| Verification | NOT ASSESSED |

## Verdict

**NEEDS ATTENTION** — one verified, medium-impact warning. Specification Adherence, Test Quality & Coverage, and Verification were not assessed.

## Context and assumptions

- No task specification or custom review instructions were supplied.
- Intent was inferred as rendering an existing animation without changing gameplay behavior.

## Checks

- Inspected the render call path and two sibling widget implementations.
- No commands run; no narrow runnable check was identified for this static performance finding.
```
