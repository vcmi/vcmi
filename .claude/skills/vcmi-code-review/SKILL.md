---
name: vcmi-code-review
description: >
  Review VCMI code changes when explicitly requested. Accepts an optional task
  specification and custom review instructions, or infers intent from the
  change and repository. Uses a seven-dimension scorecard, VCMI-specific
  invariants, and test-usefulness analysis without editing code.
---

# VCMI Code Review

Review a developer's changes locally and return actionable findings. This is a read-only contributor workflow: do not edit source files, create commits, push, or post generated GitHub comments. Write a report file only when the user asks for one.

Read [references/review-criteria.md](references/review-criteria.md) fully before reviewing. It owns the review dimensions, VCMI invariants, grading, and finding format.

## Inputs

The user may supply either, both, or neither:

- **Task specification**: pasted text or any referenced issue, document, or file. No particular format is required.
- **Custom review instructions**: extra focus, exclusions, risk assumptions, or output requirements.

Treat the specification as evidence of intended behavior, not permission to ignore correctness, safety, or repository rules. Custom instructions may narrow the review; mark dimensions outside an explicitly narrowed scope `NOT ASSESSED` instead of pretending they passed.

When no specification exists, infer intent from public contracts, callers, analogous code, configuration, player/modder documentation, tests, and the diff. Do not assume that either the implementation or its tests define correct behavior. State only the assumptions that materially affect findings.

## Resolve the review target

Use an explicitly named diff, range, commit, PR, or file set when supplied.

Otherwise:

1. Review staged and unstaged changes relative to `HEAD`, plus relevant untracked files.
2. If the worktree is clean and the current branch is not `develop`, review the branch from its merge-base with the locally available `origin/develop` or `develop`.
3. If no change can be identified, stop and ask the user for a target.

Do not fetch remotes or contact GitHub unless explicitly requested. Exclude generated files and third-party code unless the change modifies how VCMI produces or integrates them.

## Review workflow

1. Read all applicable repository instructions, the optional specification, custom instructions, and the complete diff.
2. Read affected functions and types in full. Search their callers, relevant tests, and 1–2 analogous implementations. Search the whole checkout when checking for duplicated functionality. Use `rg` or `ast-grep`; do not judge isolated hunks when surrounding state matters.
3. Establish intended behavior independently. With a specification, compare code against it. Without one, reconstruct the smallest defensible set of invariants and label consequential uncertainty.
4. Apply all seven dimensions in the reference. Use `NOT ASSESSED` for Specification Adherence without a specification; do not convert missing context into `PASS`.
5. Run the smallest relevant existing checks when practical. Prefer a targeted test over a full suite and an existing build directory over configuring a new one. Do not run expensive broad builds merely to complete the scorecard. Record commands, outcomes, and checks not run.
6. Normalize and prioritize findings. Verify file and line references against the current checkout immediately before reporting them.

## Output

Lead with findings, ordered by severity and then impact. Cap the list at 10 and consolidate closely related instances. Do not invent a finding to populate a dimension.

Each finding must include:

- ID and short title.
- Severity: `CRITICAL`, `WARNING`, or `OBSERVATION`.
- Status: `VERIFIED` or `SUSPECTED`.
- Impact: `LOW`, `MEDIUM`, or `HIGH`.
- Dimension and clickable `file:line` location where possible.
- The concrete failure, violated invariant, or maintenance cost and the evidence for it.
- The smallest credible fix. Include tradeoffs only when there is a real choice.

After findings, include:

1. **Scorecard** — all seven dimensions with `PASS`, `WARNING`, `FAIL`, or `NOT ASSESSED`.
2. **Verdict** — `APPROVED`, `NEEDS ATTENTION`, or `REJECTED`, with any limitation caused by missing specification or checks.
3. **Context and assumptions** — specification/custom instructions used and material inferred intent.
4. **Checks** — commands run and important checks not run.

If there are no findings, say so explicitly, then provide the scorecard and residual risks. Keep low-confidence possibilities out of the main findings unless the suspected risk is important and evidence is stated.
