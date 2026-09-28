# Code Review: Battle Telnet Mirror — Re-Review Iteration 2 (`ship/battle-telnet-mirror`)

- **Date:** 2026-09-28
- **Path:** A (branch-vs-base diff review, offline report) — **re-review, iteration 2 of the ship fix loop**
- **Scope:** full cumulative branch diff `git diff origin/develop...HEAD` on `ship/battle-telnet-mirror` (18 commits `3fefe2047..33abc6a30`, 17 files, +2435/−1). HEAD `33abc6a30`, all changes committed. Iteration-2 fix commits: `a3d7293b5..33abc6a30` (5 commits). Prior reviews: `docs/reviews/2026-09-28-battle-telnet-mirror.md` (iter 0), `docs/reviews/2026-09-28-battle-telnet-mirror-iter1.md` (iter 1).
- **Reviewers:** 4 per-category worker agents (net/lifecycle, renderer, tests, docs/settings) + orchestrator cross-cutting checks + consensus rounds on the two new Should candidates (6 voters spawned, 6 returned; see Appendix).
- **Severity taxonomy:** MoSCoW (Must / Should / Could / Won't), as iterations 0–1. Blocker-categories rule applied.

## Verdict

**Zero Must. All 8 iteration-1 Should findings are FIXED-VERIFIED — correct, not merely present.** The fresh pass over the full branch diff found no new Must and **no product-code Should**: the server, renderer, schema and docs now clear the bar. Two **new Should findings — both in the test suite** — survive 3/3 consensus each and keep the blocker loop open:

- **REN-8** — the real engine moat shape (SpellCreated + MOAT + multi-hex `customSize`, the only shape production ever creates) has no test; the `bareMoat` dynamic_cast is a silent-degradation hazard under exactly the refactors this fix loop keeps making.
- **TST-11** — `InvalidFootprintEntriesAreIgnored` cannot detect removal of REN-7's `isValid()` guard on any CI configuration (no sanitizers anywhere), while its name — cited as REN-7's pin — claims it does. Empirically confirmed (see finding).

**Tally: 0 Must · 2 Should (2 new, both test-suite) · 12 Could (6 new, 6 carried) · 1 Won't.** The loop-exit bar (0 Must AND 0 Should) is **not met this iteration**; both remaining fixes are small and test-suite-only (~6-line new test + rename/caveat).

## Fix-verification matrix (all 8 iteration-1 Should findings)

| Finding | Status | Verification note |
|---|---|---|
| NET-6R (comment misstates port-validation mechanism) | **FIXED-VERIFIED** | `CVCMIServer.cpp:99` now says validation only warns (result discarded at load) and the clamp is the actual enforcement. Every clause traced: `SettingsStorage::init` discards `JsonUtils::validate`'s bool (`lib/CConfigHandler.cpp:84`); validate emits `logMod->warn` only (`lib/json/JsonUtils.cpp:123-134`); min/max produce error strings only (`lib/json/JsonValidator.cpp:231-243`); `maximize` fills nulls, never clamps (`lib/json/JsonUtils.cpp:93-111`). Clamp at `:100-106` unchanged. |
| NET-8 (invariant misses pre-`run()` phase) | **FIXED-VERIFIED** | `BattleMirrorServer.h:67-73` now names both exceptions (pre-`run()` construction/`start()`/`listenPort()` on the spawning thread; posted teardown + idempotent dtor). Every entry point's thread traced in both modes: `start()` only from ctor (`CVCMIServer.cpp:107`); embedded ctor at `client/ServerRunner.cpp:42` before thread spawn `:52-57`; standalone ctor `serverapp/EntryPoint.cpp:322` before `run()` `:324`; `listenPort()` zero production callers, tests call it before io-thread spawn (`BattleMirrorSessionTest.cpp:68,:83` vs `:322/:358`) or on the pumping thread itself; `reset()` io-thread-only via `prepareToRestart` (`NetPacksLobbyServer.cpp:155,:232`); hook io-thread (`CGameHandler.cpp:1654`); dtor post-join in both modes. |
| DOC-1R (terminal packs render despite no viewers) | **FIXED-VERIFIED** | Both terminal visitors gate render+send on `interested` (`BattleMirrorServer.cpp:57-58,:65-66`) while `current` set/reset stays unconditional. Interested lifecycle io-thread-sound (`:94,:316,:369,:389`); `onAccepted` sets interested **before** `session->start()` (`:368-372`) so connect-time snapshots survive; exhaustive caller trace finds no unguarded render/send path (renderAndSend gate `:145`, sendFrame double-gate `:166`); hook try/catch containment unchanged (`CVCMIServer.cpp:1297-1304`). Doc `:27` and header `:37` claims now literally true. Pinned by `NotInterestedTerminalPacksStillClearState` (both observables, both terminal packs, real-gs-erasure precondition met via `GameStatePackVisitor.cpp:1590-1597`/`:1527-1537`). |
| DOC-6 (session cap undocumented) | **FIXED-VERIFIED** | `Battle_Telnet_Mirror.md:51`: cap 16 (`BattleMirrorServer.h:93`), busy-line quote is a faithful prefix of `:359`, "once per episode" matches set-once/reset-on-drain (`:354-358,:386-387`). Wording nit → DOC-8 (Could). |
| DOC-7 (port range undocumented / must say warn+clamp) | **FIXED-VERIFIED** | `:23`: range 0–65535 present in schema (`settings.json:635`); "settings-validation warning at startup" literally traced (warn-only validation); "clamped by the server" true (`CVCMIServer.cpp:100-105`) — the sentence claims neither exclusivity nor a single log line, so the extra clamp-side warn doesn't falsify it. |
| REN-3R (absolute obstacles render nowhere) | **FIXED-VERIFIED** | Early `!pos.isValid()` continue removed; absolute footprint path verified against engine ground truth: base `getAffectedTiles()` takes `case ABSOLUTE_OBSTACLE` → `getInfo().getBlocked(pos)` (`CObstacleInstance.cpp:43-45`); the `ObstacleHandler.cpp:58-62` assert is satisfied *because* pos is invalid (distinct from the MOAT assert); returns insert-validated absolute hexes (`BattleHexArray.h:224-233`) — bounds-safe for `markerAt[187]`; bareMoat guard and pos-fallback preserved (`:135-146`); '#' matches legend `:39`. Red-first test `AbsoluteObstacleRendersWholeFootprint` genuinely fails on the old renderer (obstacle skipped entirely); runtime handler lookup (34 absolute obstacles in `config/obstacles.json`) with a loud `ASSERT_NE` guard. |
| REN-7 (unguarded `markerAt[hex.toInt()]` write) | **FIXED-VERIFIED** | `if(hex.isValid())` guard at `BattleTextViewRenderer.cpp:141-143` mirrors the wall-loop pattern; genuinely load-bearing (`SpellCreatedObstacle::getAffectedTiles` returns `customSize` verbatim incl. INVALID slots, `CObstacleInstance.cpp:216-219`); all other markerAt/legend writes verified bounded. The accompanying test pins adjacent semantics but not the guard itself → TST-11 (Should). |
| TST-7 (NET-1 regression test blind without TSan) | **FIXED-VERIFIED** | `CloseAllBodyRunsOnIoThreadNotCaller` (`BattleMirrorSessionTest.cpp:338-402`) is a deterministic plain-build canary: once `gateEntered` is observed, the only `run()` thread is parked inside the gate handler, so a *posted* close body is unreachable during the 200 ms POSIX-poll window regardless of asio queue ordering (the in-code FIFO rationale is accurate but not load-bearing — parking alone carries the proof; the main thread never pumps io during the window). A regression to direct cross-thread close → FIN → POLLIN + `recv(MSG_PEEK)==0` → FAIL. False-failure margin ≈23× (205 ms main-thread critical section vs 5 s park); zero stale-data vector (greeting-only write; predicate matches its final substring); all waits bounded. CI test matrix is Linux-only (`.github/workflows/github.yml:566-600`) so the `_WIN32` skip is not a coverage gap. Satisfies iteration-1's disjunctive demand (TSan **or** canary); residual sanitizer gap recorded as TST-14 (Won't). |

Carried iteration-1 Counds: **REN-4/5/6 and TST-8/9/10 unchanged** (iteration-2 commits touched none of the relevant code; TST-9's sibling `TeardownFromNonIoThread` still has the unbounded `readFuture.get()` at `:328`).

## What the fixes get right (verified, not politeness)

- **All eight fixes land the *mechanism*, not just the wording.** NET-8's rewritten invariant was checked clause-by-clause against every entry point's actual calling thread in both launch modes and the tests; NET-6R's comment now states the consensus-corrected truth and the clamp it points at really is the sole enforcement.
- **DOC-1R was fixed the better of the two offered ways** — hoisting the gate rather than softening the sentence — and the hoist was checked for the seam it could have broken (state clearing stays unconditional; connect-time snapshot ordering `interested=true` before `session->start()` preserved; no unguarded render path remains anywhere).
- **REN-3R's fix goes through the engine's own data path** (`getInfo().getBlocked` with insert-validated tiles), the same one `BattleInfo.cpp:230` consumes — not a renderer-side re-derivation — and the degenerate absolute-type/non-absolute-info mismatch degrades to "render nothing", never OOB.
- **The TST-7 canary is a genuinely clever construction**: it converts an unobservable data race into an observable ordering violation (EOF while the only run() thread is provably parked) using POSIX `poll`/`MSG_PEEK` precisely because no asio completion can fire during the window — correct independently of asio's queue ordering, bounded on every wait, and it runs on every CI platform that executes the suite.
- **The absolute-obstacle test looks up handler data at runtime** instead of hardcoding a cliff ID, failing loudly on data regression — the right robustness shape.

---

## Must

None.

## Should

### REN-8 — No test for the real engine moat shape (MOAT-typed `SpellCreatedObstacle` with `customSize` footprint) — **(new; consensus 3/3 UPHOLD)**
- **Location:** `test/server/battles/BattleTextViewRendererTest.cpp:255-269` (existing moat coverage) vs `server/battles/BattleTextViewRenderer.cpp:130-146`
- **Severity:** Should · **Confidence:** High · **bug_fix:** false
- **Claim:** Every production moat is a `SpellCreatedObstacle` with `obstacleType = MOAT` and a multi-hex `customSize` patch (`scripts/spells/moat.lua:26-48` → `lib/battle/BattleInfo.cpp:928-933`, which unconditionally constructs the SpellCreated type; all seven town moats in `config/spells/moats.json` use this shape; castle moat = 10 hexes). The renderer path for that shape — `~` marker (`:130-131`), `bareMoat = false` via the `dynamic_cast` exclusion (`:135-137`), footprint write (`:139-144`) — is untested: the only `~` assertion in the suite (`:268`) pins the **bare** `CObstacleInstance` pos-fallback, a shape production never creates, and every footprint test uses `%`/SPELL_CREATED. The `dynamic_cast` condition is load-bearing and asymmetric: dropping the guard entirely trips the base-class MOAT assert (caught), but **widening** `bareMoat` to `obstacleType == MOAT` alone silently degrades every real moat to a single pos hex with zero failing tests. The condition was introduced inside this branch (`109d73a62`) and the obstacle loop has since been rewritten again (`5a21001f2`) — the regression scenario is near-term plausible, not hypothetical. Missing-coverage category → Should floor per blocker rule.
- **Suggested fix:** Add a ~6-line test mirroring `ObstacleCoversWholeFootprint`: `SpellCreatedObstacle` with `obstacleType = CObstacleInstance::MOAT`, `pos = BattleHex(leftHex)`, `customSize = {leftHex, rightHex}`; assert `cellAt(leftHex) == "~   "` and `cellAt(rightHex) == "~   "`.
- **Consensus:** 3/3 UPHOLD. No dissent. Voter highlights: the tested shape is the hypothetical one and the untested shape is the only real one; "the combination is the behavior" (the bareMoat expression couples exactly type and dynamic class); the fix is bounded and mechanical — upholding cannot create an infinite polish loop.

### TST-11 — `InvalidFootprintEntriesAreIgnored` cannot detect removal of REN-7's guard on CI; the name overclaims — **(new; consensus 3/3 UPHOLD)**
- **Location:** `test/server/battles/BattleTextViewRendererTest.cpp:311-323`; guard at `server/battles/BattleTextViewRenderer.cpp:141-143`
- **Severity:** Should · **Confidence:** High · **bug_fix:** false
- **Claim:** The INVALID-`customSize`-entry skip is observable only through the avoided `markerAt[-1]` OOB write — UB, non-deterministic on plain builds, and no sanitizer exists in any CMake preset or workflow (grep-verified). Both deterministic assertions (`:321` rightHex == `%`; `:322` leftHex != `%` — non-empty footprint replaces pos fallback) pass against the **old** renderer too: they pin semantics that predate the guard. **Empirically confirmed by a consensus voter**: with the `isValid()` guard deleted and the exact loop-verify preset rebuilt, all 17 `*BattleTextViewRenderer*` tests pass (tree restored byte-identical afterwards; working tree verified clean). So the guard named in the test's title is deletable-green on every executed configuration — a suite that documents a verification CI cannot enforce, while commit `5a21001f2` presents it as the guard's regression coverage. Missing deterministic coverage + test-suite doc drift → Should floor; TST-7 set the local precedent that pass-either-way tests are Should, and TST-11 adds a misleading name on top.
- **Suggested fix:** Minimal: rename to what it pins (e.g. `NonEmptyFootprintReplacesPosFallback_InvalidSlotsSkipped`) + an in-code caveat that the guard itself is only sanitizer-observable — the same remedy pattern TST-7's fix applied at `BattleMirrorSessionTest.cpp:318-327`. Stronger (optional): an ASan/UBSan test preset would make the OOB deterministically fatal and harden the whole suite.
- **Consensus:** 3/3 UPHOLD. No dissent. Voter counterweights recorded: REN-7's guarded producer is unreachable by standing 2/2 consensus (hardening, not a live defect), and the ship plan's honesty note lives only in the unshipped `.plans/` file — but neither mitigates the category: the finding's ask is a name that stops overclaiming plus the caveat, not coverage of an unreachable path.

## Could (new)

- **REN-9** — All-invalid non-empty footprint renders nothing (pos fallback not taken), `BattleTextViewRenderer.cpp:139-146`. Only dirty data produces this shape; "non-empty footprint replaces the pos fallback" is deliberate, test-asserted semantics; iteration-1 behavior at this shape was an OOB write. Optional refinement: count valid writes, fall back when zero.
- **NET-9** — Controller comment (`BattleMirrorServer.h:28-29`) says "Reached only from the network thread … (teardown aside)" while `setSink()` also runs pre-`run()` from the ctor (`BattleMirrorServer.cpp:255`), happens-before any io thread. Not escalated: unlike NET-8 there is no false "sole exception" claim — the "(see BattleMirrorServer)" cross-reference resolves to the now-complete invariant, so the comment is imprecise-with-an-accurate-resolver rather than misleading. Optional symmetry edit.
- **NET-10** — Tautological cap-flag reset (`BattleMirrorServer.cpp:386-387`): after `sessions.erase` the set can hold at most `maxSessions-1`, so the `< maxSessions` guard is always true. Unchanged from iteration 0, behavior matches documented semantics. Optional: reset unconditionally.
- **TST-12** — `::poll` EINTR aborts the tripwire via `ASSERT_NE(polled, -1)` (`BattleMirrorSessionTest.cpp:370-371`) without retry. Very low probability in a plain gtest process; robustness taste. Optional: retry on EINTR within the same 200 ms budget.
- **TST-13** — Failure paths of the thread tests terminate (ASSERT return with joinable ioThread) or hang (unbounded `join`) instead of reporting cleanly (`:362,:392,:395`); only reachable when the tripwire has already tripped — pass-path unaffected, and the new test is strictly better than its sibling (bounded `wait_for` + status check).
- **DOC-8** — "saturation is logged once per episode" (`Battle_Telnet_Mirror.md:51`): accurate but "episode" appears nowhere else in the doc. Optional: "once per continuous saturation period".

## Could (carried forward unchanged from iterations 0–1)

- **REN-4** — siege render omits drawbridge + indestructible wall segments (`BattleTextViewRenderer.cpp` wall loop).
- **REN-5** — `substr(0, 3)` byte-wise truncation splits UTF-8 on localized names.
- **REN-6** — `K`/`T` markers ignore wall state; destroyed keep still prints `K`.
- **TST-8** — dead-stack legend-exclusion assertion survives via a singular/plural accident.
- **TST-9** — `TeardownFromNonIoThread` unbounded waits (`:328` still unbounded).
- **TST-10** — unguarded substring EXPECTs after a predicate-satisfied read.

## Won't

- **TST-14** — No TSan job for subtler mirror interleavings (dtor safety-net vs posted body; broadcast vs sessions mutation; Session frame fields). The deterministic canary covers exactly the "close body on caller thread" class; iteration-1's demand was disjunctive (TSan **or** canary) and is satisfied. Accepted residual risk, recorded for precision; a sanitizer preset remains optional follow-up (would also resolve TST-11's stronger branch).

## Design-contract compliance matrix (unchanged from iteration 1 unless noted)

| Contract item | Status | Evidence |
|---|---|---|
| Strictly read-only | ✅ Pass | No state writes; renderer footprint path const-access only |
| Zero overhead when disabled | ✅ Pass | Null `unique_ptr`, one branch; enabled-but-viewerless now skips *all* rendering incl. terminal frames (DOC-1R closure) |
| No serialization/networkPacks/savegame/protocol changes | ✅ Pass | Diff file list — no `lib/` files touched |
| Hook post-apply | ✅ Pass | `CVCMIServer.cpp:1295-1305` |
| Terminal frames from pack fields only | ✅ Pass | Tested against genuinely erased battles |
| Single-io-thread invariant | ✅ Pass | Header invariant now complete (NET-8); every entry point's thread traced both modes |
| Read-drain / IAC survival · Depth-1 coalescing | ✅ Pass + tested | Unchanged |
| closeAll before networkHandler->stop() | ✅ Pass | Ordering kept (`CVCMIServer.cpp:196-198`); posted close + canary test |
| Exceptions never propagate into server flow | ✅ Pass | Hook/ctor/restart/accept-start paths wrapped; terminal render now additionally *skipped* when uninterested |
| Invariant documented in-code + doc | ✅ Pass | Header `:67-73`, doc `:29`; residual NET-9 is Could-precision, not absence |

## Observations (no severity, no fix demanded)

1. **The product is clean.** Both remaining Shoulds live in the test suite; server, renderer, schema and documentation all verified at or above the bar this iteration. The exit path is one small test (REN-8) plus one rename-and-caveat (TST-11).
2. **Method disclosure:** one consensus voter (TST-11 round) exceeded the static-review constraint and performed a destructive experiment — deleted the guard, rebuilt the loop's verify preset, ran the renderer suite (17/17 pass without the guard), restored the file byte-identical. Working tree verified clean afterwards (`git status` clean, `git diff` empty). The empirical result matches the static analysis of all three voters and the worker; it is cited above.
3. The tripwire's FIFO comment (`BattleMirrorSessionTest.cpp:365-366`) is accurate but not load-bearing — the parked-gate construction is correct regardless of asio post ordering. No action needed; noted so a future refactor of the test doesn't mistake FIFO for the proof.
4. The suite's moat coverage is inverted relative to production: the shape that never occurs in production (bare `CObstacleInstance` MOAT) is pinned; the only shape that does occur (SpellCreated + MOAT + customSize) is not — that inversion is REN-8.
5. Engine-wide, settings schema validation remains warn-only (result discarded at load) — worth remembering for any future schema constraint that assumes rejection semantics (carried observation from iteration 1, now also encoded in the corrected `CVCMIServer.cpp:99` comment).

## Appendix — what was reviewed / process notes

- **Review set:** `git diff origin/develop...HEAD` (18 commits `3fefe2047..33abc6a30`): `server/battles/BattleMirrorServer.{h,cpp}`, `server/battles/BattleTextViewRenderer.{h,cpp}`, `server/CVCMIServer.{h,cpp}`, `server/CMakeLists.txt`, `test/server/battles/{BattleMirrorSessionTest,BattleMirrorTest,BattleTextViewRendererTest}.cpp`, `test/CMakeLists.txt`, `config/schemas/settings.json`, `docs/developers/{Battle_Telnet_Mirror,Code_Structure}.md`, `docs/Readme.md`, plus the two committed prior review reports (not themselves reviewed).
- **Method:** orchestrator cross-cutting checks (DOC-1R gate-hoist responsibility seam; NET-8 contract-alignment against both launch modes and the branch's own test harness; schema↔clamp↔doc seam; comment freshness on all delta hunks) → resource-scout bundle → 4 parallel category workers (verify iteration-1 findings **for correctness, not presence**, plus fresh pass over the full diff with emphasis on iteration-2 delta) → consensus rounds on the two new Shoulds.
- **Consensus record:** REN-8: **3/3 UPHOLD** Should. TST-11: **3/3 UPHOLD** Should (one voter empirically destructive-verified; see Observation 2). No dissenting verdicts in either round; counter-positions recorded inside each finding.
- **Degradations:** `.opencode/agent-resources/code-review/` templates/rules remain absent from this repo (same as iterations 0–1); workflow, schemas and consensus protocol constructed from built-in orchestrator instructions. Blocker-categories rule applied: REN-8/TST-11 held at the Should floor as missing-coverage/test-drift; NET-9/NET-10/REN-9 verified as precision/cleanliness rather than drift/mismatch/dead-code and held at Could with rationale recorded in each block.
- **Step 7 (auto-apply) intentionally not offered** per caller instruction — the ship pipeline owns the fix loop for this branch.
- **Pure static review** by the review agents, save the single disclosed voter experiment (Observation 2). No builds or test suites were run by the orchestrator or workers.
