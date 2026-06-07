# Adventure AI Evaluation Plan

This document describes a practical workflow for improving the main-game
computer player, especially `Nullkiller2`. The goal is not to prove that every
loss is a bug. The goal is to find high-profile, reproducible bad decisions,
understand their root cause, fix them, and verify that the fixed AI plays better
than the baseline.

## Scope

The target is adventure-map AI behavior:

- hero movement;
- object capture;
- exploration;
- town development;
- recruitment and army gathering;
- town defense;
- resource trading;
- hero roles and hero chains.

Battle AI can be kept constant during these tests. When evaluating adventure AI
changes, battle behavior should not be the uncontrolled variable.

## Core Principle

Evaluate the AI only with information available to that player. A decision is
not stupid just because a developer can see the full map or knows future random
events. A candidate issue must be judged from the AI player's visible state,
known objects, reachable paths, known enemies, resources, towns, heroes, and
legal actions.

## Candidate Lifecycle

### 1. Discover Suspicious Decisions

Run AI games on generated maps, fixed maps, and saved games. Capture decisions
that look strategically wrong, for example:

- a hero ends the turn with movement points while a valuable visible target is
  safely reachable;
- an empty or weakly defended visible town is ignored;
- a town under visible threat is not defended;
- creatures are not recruited when they are clearly affordable and useful;
- resources are traded or saved in a way that blocks stronger progress;
- a hero makes a suicidal attack with no compensating strategic gain;
- the AI repeats low-value movement while a high-value objective is available.

The output of this stage is only a suspicion, not a confirmed AI bug.

### 2. Record Decision Context

For each suspicious decision, save enough data to explain it later:

- map or save identifier;
- random seed, if available;
- day/week/month;
- player color and difficulty;
- visible objects and known enemies;
- owned heroes, towns, armies, resources, and artifacts;
- candidate tasks produced by the AI;
- task priorities and priority tier;
- selected task;
- rejected high-priority-looking tasks;
- path cost, danger, expected reward, and relevant decomposition details.

Nullkiller is mostly explainable because it generates candidate goals/tasks and
scores them. The audit log should expose that decision surface instead of only
recording the final action.

### 3. Apply the Non-Cheating Check

Before calling a decision bad, verify that the better alternative was actually
available to the AI:

- the target was visible or remembered according to the AI player's knowledge;
- the path was legal from the AI callback data;
- required resources, movement, army strength, and map access were available;
- no hidden enemy, hidden guard, or future event is required to justify the
  alternative;
- the suggested action would be something a legal player could choose in the
  same visible state.

If the better action requires hidden information, reject the candidate.

### 4. Verify It Is Actually Stupidity

Some bad-looking outcomes are legitimate failures. The AI may take a reasonable
risk, make a meaningful trade-off, or choose one plausible plan among several.
Do not fix those as "stupidity" unless the evidence is strong.

A confirmed stupidity candidate should satisfy one of these:

- **Dominated action:** another legal visible action is at least as good on all
  relevant visible dimensions and strictly better on at least one important
  dimension.
- **No compensating upside:** the chosen action worsens the position and has no
  credible strategic benefit.
- **Missing obvious objective:** the AI does not even generate or consider an
  important legal task that should be in scope for its current behavior.
- **Systemic mis-scoring:** the AI considers the good action but consistently
  assigns it a much lower priority than weaker alternatives.

Examples of dimensions to compare:

- immediate material gain or loss;
- town ownership;
- hero safety;
- army strength;
- movement efficiency;
- resource availability;
- future recruitment;
- threat coverage;
- map control;
- strategic timing, especially end-of-week and end-of-day effects.

If a decision is a meaningful trade-off, keep it as an analysis note but do not
promote it to a high-priority fix.

### 5. Reproduce Systemically

One strange save is not enough for a high-profile fix. After finding a candidate,
generate or collect many game states that reproduce the same situation under
different circumstances.

Vary:

- random map template and seed;
- terrain and road layout;
- player color and starting faction;
- hero class and army composition;
- day of week;
- available resources;
- nearby enemy pressure;
- target value;
- fog-of-war boundary;
- distance to town or object;
- garrison strength;
- available scouts and main heroes.

The question is: does the same bad decision pattern happen repeatedly when the
strategic situation is equivalent?

Expected output:

- a small corpus of reproduction saves or generated scenarios;
- per-scenario audit logs;
- a summary of how often the bad decision appears;
- classification of whether the decision is dominated, probably dominated, or
  ambiguous.

Only promote systemic, non-ambiguous cases to the improvement queue.

### 6. Find the Root Cause

Map the confirmed bad decision to one or more code causes:

- missing behavior;
- missing candidate task;
- bad pathfinder input or path filtering;
- incorrect danger estimate;
- incorrect reward estimate;
- bad priority tier;
- bad hero role assignment;
- over-aggressive resource locking;
- stale memory;
- object visibility problem;
- decomposition loop or decomposition cutoff;
- task conflict filtering discarding the right task.

Prefer small fixes that address the root cause directly. Avoid broad heuristic
tuning unless the evidence shows the scoring model itself is the problem.

### 7. Validate the Fix Locally

Run the reproduction corpus again with the fix:

- the previously bad action should disappear or become rare;
- the better action should be generated and selected;
- the task explanation should make sense;
- no illegal action or hidden-information behavior should appear;
- turn time should not regress meaningfully.

If the fix only handles one exact save and fails on nearby generated cases, it
is not robust enough.

### 8. Run Baseline vs Fixed AI

After local validation, compare fixed AI against the baseline:

- same maps and seeds;
- swapped colors/sides where possible;
- same battle AI;
- same difficulty and game settings;
- enough games for the result to be meaningful;
- record wins, losses, days to win/loss, score, towns, heroes, army value,
  resources, crashes, timeouts, and average turn time.

The fix should improve or preserve overall play strength. A local behavior fix
that makes the AI worse globally needs more analysis before merging.

### 9. Produce Human Demo Artifacts

Machine-readable logs are good for analysis, but high-profile cases should also
be demonstrable to humans. A demo should show the same reproduced situation
before and after the fix:

- baseline AI reaches the state and makes the bad decision;
- fixed AI reaches the same state and no longer makes that decision;
- the important visible context is clear;
- the explanation is tied to the structured decision log;
- hidden information is not used as the proof.

The video is not the source of truth. The source of truth is the replayable save
or generated scenario plus the decision audit log. The video is a review aid.

Recommended artifacts per high-profile case:

- `case.json` describing the scenario, tags, visible-state claim, and expected
  behavior;
- one or more saves or generated-state specs;
- baseline JSONL decision log;
- fixed JSONL decision log;
- baseline clip;
- fixed clip;
- short markdown summary with the root cause and result.

For systemic cases, keep several short clips rather than one long recording.
The ideal demo is a small gallery: the same stupidity appears in multiple
different maps or circumstances, then disappears after the fix.

## High-Profile Case Criteria

Focus first on cases with large expected payoff:

- reproducible across many generated states;
- clearly dominated or near-dominated;
- visible to a normal player without map cheating;
- affects important strategic outcomes such as towns, main hero safety, or
  weekly army growth;
- root cause is identifiable;
- fix is likely to be contained;
- benchmark result can be measured.

Low-profile cases can be tracked, but they should not dominate early work.

## Tooling Needed

Some pieces exist already, but a full workflow needs more automation.

### Decision Audit Logger

Add structured logging around Nullkiller task selection:

- generated candidate tasks;
- decomposition chain;
- priority components;
- selected task;
- rejected top-N tasks;
- reason for filtering or invalidation;
- affected heroes and objects.

The log should be machine-readable, ideally JSON lines.

The log should also contain stable identifiers usable by demo tooling:

- case id or run id;
- turn and pass;
- player color;
- selected hero;
- selected task id;
- target object id and position;
- top rejected task ids;
- coordinates that a spectator/demo camera should focus on.

### Suspicion Detectors

Add optional detectors for likely bad states:

- idle hero with useful movement available;
- reachable valuable object ignored;
- visible weak/empty town ignored;
- visible town threat not answered;
- affordable army not recruited;
- repeated no-op or low-value passes;
- unsafe attack with poor reward/danger ratio.

These detectors should flag candidates, not decide correctness.

### Reproduction Corpus

Store confirmed candidates as replayable saves or generated scenario specs.
Each case should include:

- baseline behavior;
- expected better behavior;
- visible-state justification;
- root-cause hypothesis;
- tags such as `town-capture`, `defense`, `recruitment`, `pathfinding`,
  `resource-trade`, or `hero-role`.

Each case should be runnable in two modes:

- **analysis mode:** headless or fast run that emits JSONL logs and metrics;
- **demo mode:** GUI/spectator run that records a short visual clip.

### Scenario Generator

Build a way to produce many variations of a candidate situation. This can start
simple: generate random maps, fast-forward to similar states, or mutate saved
states manually. Later it can become a dedicated scenario generator.

### Batch Runner

Run baseline and candidate AI versions over the same maps/saves:

- headless mode;
- fixed settings;
- repeat count;
- seed control;
- timeout;
- result collection;
- crash detection.

### Demo Recorder

Use replayable states and the spectator UI to create review clips. The existing
client has test-map/test-save and spectator-style options, so the first version
can be an external wrapper around the normal GUI client:

- launch the game from a saved candidate state;
- enable spectator mode and deterministic settings where possible;
- set hero and battle animation speeds appropriate for review;
- focus the camera on the relevant hero, town, object, or threat;
- capture the game window with an external recorder;
- stop recording after the decision is made or after a fixed time limit.

This can be implemented incrementally:

1. Record the whole window for a replayed candidate save.
2. Add log-driven timestamps to cut the important segment.
3. Add captions from the decision audit log, such as selected task, rejected
   better task, priority values, and visible-state reason.
4. Add before/after batch rendering for baseline and fixed builds.
5. Optionally add an in-engine screenshot/frame capture path later if external
   window capture is too fragile.

The demo recorder must not make the correctness judgment. It should render what
the case corpus and logs already establish.

### Results Database

Persist run results in a simple table:

- commit or build id;
- map/save id;
- seed;
- player side;
- winner;
- days played;
- final statistics;
- decision-candidate tags;
- runtime and failure status.

## Acceptance Checklist

A fix is ready only if:

- the original issue is reproducible;
- the better action is legal under visible information;
- the selected baseline action is dominated or clearly unjustified;
- the root cause is identified;
- the fix removes the bad pattern across a varied reproduction corpus;
- baseline-vs-fixed games show no significant strength regression;
- performance remains acceptable;
- at least one focused regression test or replayable scenario is kept;
- high-profile cases have regenerated baseline and fixed demo artifacts.

## Recommended First Milestone

Start with instrumentation, not tuning:

1. Add structured Nullkiller decision audit logs.
2. Create a small manual corpus of suspicious saves.
3. Add one or two suspicion detectors for high-profile cases.
4. Reproduce one confirmed case across varied states.
5. Fix that single root cause.
6. Run baseline-vs-fixed games on the reproduction corpus and a small random
   map sample.
7. Generate one before/after demo clip from the same replayable case.

This keeps the work evidence-driven and avoids changing heuristics based on a
single anecdotal game.
