# Code Review: Battle Telnet Mirror — Re-Review Iteration 1 (`ship/battle-telnet-mirror`)

- **Date:** 2026-09-28
- **Path:** A (branch-vs-base diff review, offline report) — **re-review, iteration 1 of the ship fix loop**
- **Scope:** full cumulative branch diff `git diff origin/develop...HEAD` on `ship/battle-telnet-mirror` (13 commits, 16 files, +2112/−1). HEAD `a3d7293b5`, all changes committed. First review: `docs/reviews/2026-09-28-battle-telnet-mirror.md` (in-branch at `c1a7336e8`).
- **Reviewers:** 4 per-category worker agents (net/lifecycle, renderer, tests, docs) + orchestrator cross-cutting checks + consensus rounds on the four high-stakes questions (12 voters spawned, 10 returned; see Appendix).
- **Severity taxonomy:** MoSCoW (Must / Should / Could / Won't), as iteration 0. Blocker-categories rule applied (see TST-7 escalation note).

## Verdict

**Zero Must.** The iteration-0 Must (NET-1 cross-thread teardown race) is **verified fixed — consensus 3/3** — and the fresh pass over the fix commits surfaced no new Must: the one candidate (a potential OOB write in the new obstacle-footprint loop) was **downset to Should by consensus 2/2** (mechanics real, no reachable producer; hardening guard recommended).

Of the 24 first-round Must/Should findings: **22 verified fully fixed**, **2 functionally fixed but incomplete** (NET-6: enforcement works but the justifying comment misstates the mechanism; REN-3: footprints correct for usual/spell-created/moat obstacles, but absolute obstacles (cliffs) still render nowhere — re-filed as Should). The 3 iteration-0 Counds (REN-4/5/6) remain open by agreement, plus 3 new test-robustness Counds.

**Tally: 0 Must · 8 Should (2 residuals, 6 new) · 6 Could (3 carried, 3 new) · 0 Won't.**

## Fix-verification matrix (all iteration-0 findings)

| Finding | Status | Verification note |
|---|---|---|
| NET-1 (Must: embedded-mode shutdown race) | **FIXED-VERIFIED — 3/3 consensus** | `closeAll()` = pure `asio::post` of idempotent `closeAllImpl` (`BattleMirrorServer.cpp:296-315`); dtor repeat runs provably post-join (embedded) / post-`run()` (standalone); member order kills `battleMirror` before the io_context (`CVCMIServer.h:39-44`). All 5 scrutiny points hold (see Appendix). |
| NET-2 (accept re-arm) | FIXED-VERIFIED | Re-arms on every non-abort outcome exactly once; abort-only skip (`:336-348, :377`). |
| NET-3 (session cap) | FIXED-VERIFIED | Cap 16 + busy line + close + re-arm (`:350-363`); `sessionCapLogged` once-per-episode, resets on drain (`:384-385`); pinned by a real 17-viewer loopback test. |
| NET-4 (accept→start exceptions) | FIXED-VERIFIED | try/catch → `dropSession`; `startAccept()` outside the try re-arms unconditionally (`:368-377`). |
| NET-5 (gameState dangle on restart) | FIXED-VERIFIED | `reset()` clears exactly the dangling trio (`BattleMirrorServer.cpp:112-117`); `renderFrame` null-checks both pointers; **all** `prepareToRestart` callers traced to io-thread-only (lobby-pack visitors, `NetPacksLobbyServer.cpp:155, :232`) — no new race; regression test at `BattleMirrorTest.cpp:188-205`. |
| NET-6 (port wrap) | **FIXED-INCOMPLETE** | Schema min/max + int64 clamp+warn work (functional defect gone). Residual: the comment "schema validation already rejects out-of-range ports" is wrong — validation is **warn-only** (consensus 3/3, CLAIM A). Re-filed as **NET-6R**. |
| NET-7 (acceptor fd leak in start() catch) | FIXED-VERIFIED | `is_open()`-guarded `error_code` close before logging (`:277-284`). |
| REN-1 (dead stacks on grid) | FIXED-VERIFIED | Single early `!alive()` continue before both hex writes (`:148-160`) — kills the dead-masks-living hazard by construction; turrets handled via `position.isValid()`; legend/grid now one predicate; tests pin both. |
| REN-2 (LF-only endings) | FIXED-VERIFIED | All 11 newline literals are `\r\n`; zero bare `\n` (grep-verified); test asserts the CRLF contract structurally over whole frames. |
| REN-3 (obstacle footprints) | **FIXED-INCOMPLETE** | `getAffectedTiles()` + bare-MOAT guard + `pos` fallback correct for USUAL/SPELL_CREATED/live MOAT (the assert trap is defused). But ABSOLUTE_OBSTACLE (cliffs) still render **nowhere** — consensus 2/2 UPHOLD. Re-filed as **REN-3R**. |
| DOC-1 (fast-path claim vs code) | FIXED (residual → DOC-1R) | `interested` gate now precedes `renderFrame()` (`:143-145`); doc + header match. Tiny overclaim remains for terminal packs → DOC-1R. |
| DOC-2 (io-thread invariant in doc) | FIXED-VERIFIED | Doc describes the **post-fix** teardown (posted closeAll + idempotent dtor), matches code. |
| DOC-3 (coalescing doc) | FIXED-VERIFIED | Depth-1 latest-wins bullet accurate (`:49` vs code `:186-193, :216-235`). |
| DOC-4 (shutdown/containment doc) | FIXED-VERIFIED | Ordering + posted rationale + hook containment documented, all cited paths current. |
| DOC-5 (gate legend) | FIXED-VERIFIED | "G closed or blocked (creature-held)" matches `gateMarker()` default branch; test covers BLOCKED→`G`. |
| TST-1 (double-wide second hex) | FIXED-VERIFIED | `DoubleWideStackOccupiesBothHexes` uses engine `occupiedHex()` ground truth, asserts both cells. |
| TST-2 (BattleCancelled path) | FIXED-VERIFIED | Full controller-level cancelled path incl. `hasBattle()==false` and frame-count freeze. |
| TST-3 (session/asio harness) | FIXED-VERIFIED | New loopback harness covers all 4 demanded scenarios **plus** the NET-1 non-io-thread teardown, session cap, acceptor re-arm, abrupt disconnect. Public-API only, port 0, poll()-pumped, bounded. Residual coverage note → TST-7. |
| TST-4 (11 untested visitors) | FIXED-VERIFIED | `BattleEventPacksEachProduceAFrame`: 11 packs, exactly-one-frame each (12 total counted). |
| TST-5 (erasure precondition) | FIXED-VERIFIED | Both terminal tests run `server.applyPack(...)` (real gs erase) before the controller hook. |
| TST-6 (marker mapping) | FIXED-VERIFIED | DAMAGED/INTACT walls, OPENED/DESTROYED/BLOCKED gate, towers/keep, ANSI wrapper, dead-stack grid+legend, stack precedence — all pinned. |

## What the fixes get right (verified, not politeness)

- **The NET-1 fix is the important thing and it is done properly.** Teardown marshalling is complete and lifetime-sound: post-only `closeAll`, idempotent `closeAllImpl`, post-join destructor safety net, correct member destruction order (asio handles die before the context), `shared_from_this()` in every completion handler, and both stop-order interleavings (closure drains vs abandoned-uninvoked) are safe. Three independent voters tried to break it from five angles and failed.
- **The accept loop is now airtight**: every `onAccepted` outcome re-arms exactly once except `operation_aborted`; cap rejection and the exception path both re-arm; no busy-loop (event-driven only).
- **NET-5's fix introduces no new race** — the tempting worry (`reset()` from a non-io thread racing a snapshot render) was traced end-to-end and cleared: `prepareToRestart` is reachable only from lobby-pack application on the io thread.
- **REN-1's single early-continue is exactly the right shape** — one guard covers head + occupied hex, eliminating the masking hazard by construction, with consistent legend/grid predicates and both-halves regression coverage.
- **The loopback harness is genuinely well built**: drives the private `Session` entirely through the public API on real sockets, port-0, `poll()`-based (no sleeps), quiescence detection converts hangs into failures, correct fixture destruction order — and it pins behaviors nobody asked for (cap, re-arm, abrupt disconnect).
- **Tests use engine ground truth, not re-derived tables** (`wallPartToBattleHex`, `BattleHex` accessors), so renderer/engine divergence fails rather than silently co-varies.

---

## Must

None. (NET-1 verified fixed; no new Must found — the single candidate was downset by consensus, see REN-7.)

## Should

### NET-6R — Comment misstates the port-validation mechanism ("rejects" vs warn-only) — residual of NET-6 — **consensus 3/3 (CLAIM A)**
- **Location:** `server/CVCMIServer.cpp:99` (comment above the clamp at `:100-106`)
- **Severity:** Should · **Confidence:** High · **bug_fix:** false (functional wrap defect itself is fixed)
- **Claim:** The comment "schema validation already rejects out-of-range ports; the clamp is defense in depth" is wrong about the mechanism: `SettingsStorage::init` calls `JsonUtils::validate(...)` and **discards the bool return** (`lib/CConfigHandler.cpp:81-85`); `validate` takes `const JsonNode&`, cannot mutate, and on failure only emits `logMod->warn("Data in settings is invalid!")` (`lib/json/JsonUtils.cpp:136-147`; min/max checks at `lib/json/JsonValidator.cpp:231-243` produce error strings only; no `normalize` exists anywhere in `lib/json`). `maximize` fills nulls and erases non-required keys only — a present `70000` survives untouched. The clamp is therefore the **sole** enforcement, not defense in depth. A reader trusting the comment could remove the clamp as "redundant" and reintroduce NET-6's silent wrap.
- **Suggested fix:** Reword: "schema validation *warns* on out-of-range ports but does not reject them; this clamp is the actual enforcement."
- **Note:** This also corrects iteration-1 docs-worker basis that claimed validation rejects at load — overruled 3/3 by the consensus trace.

### NET-8 — Header thread-invariant comment inaccurate in the other direction: the pre-`run()` phase is uncovered by the "sole exception" wording — **(new, doc drift)**
- **Location:** `server/battles/BattleMirrorServer.h:67-69` (invariant comment); `server/CVCMIServer.cpp:80-118` (construction/start on spawning thread)
- **Severity:** Should · **Confidence:** High · **bug_fix:** false
- **Claim:** The rewritten invariant says every entry point runs on the single network thread with `closeAll` as the sole exception — but construction, `start()` (open/bind/listen + first `async_accept` arm) and `listenPort()` run on the **spawning** thread (client thread in embedded mode: `CVCMIServer` is fully constructed at `ServerRunner.cpp:42` before the server thread exists at `:52`; main thread standalone). Safe today (happens-before thread start; `listenPort()` has no production caller outside tests), but a future reader trusting "sole exception is closeAll" could add a pre-run or concurrent call and silently break the no-locks contract the comment exists to protect. (A voter nit — `listenPort()` formally violates asio same-object concurrency if called cross-thread with the acceptor outstanding — folds into this same wording gap.)
- **Suggested fix:** Extend the header sentence: "…teardown and the pre-`run()` phase (construction, `start()`, `listenPort()` on the spawning thread, before the io thread exists) are the sole exceptions."

### REN-3R — Absolute obstacles (cliffs) still render nowhere — residual of REN-3 — **consensus 2/2 UPHOLD**
- **Location:** `server/battles/BattleTextViewRenderer.cpp:127-128` (early `pos.isValid()` continue) vs `:139` (footprint query)
- **Severity:** Should · **Confidence:** High (upgraded from Medium by voter evidence) · **bug_fix:** true
- **Claim:** The footprint fix is correct for USUAL, SPELL_CREATED and live-MOAT obstacles, but the pre-existing early `if(!obstacle->pos.isValid()) continue;` drops every `ABSOLUTE_OBSTACLE` — and absolute obstacles never get `pos` set **by design** (creation sets only type/ID/uniqueID, `lib/battle/BattleInfo.cpp:224-228`; `ObstacleHandler.cpp:60` asserts the hex is invalid for absolutes). Their footprint is nonetheless available and valid (`getAffectedTiles()` → `getBlocked` returns insert-validated absolute `blockedTiles`, ignoring `pos`). Cliffs roll in ~40% of open-field battles with obstacles allowed (`r.rand(1,100) <= 40`, `BattleInfo.cpp:219`; 34 absolute obstacles ship in `config/obstacles.json`) and block ~5+ pathfinding-relevant hexes (`BattleInfo.cpp:230-231` seeds placement from the same call) — all invisible, so REN-3's core harm ("actually-blocked hexes appear passable") persists for this obstacle class. No other render path covers them.
- **Suggested fix:** Query the footprint before (or instead of) the `pos.isValid()` gate, keeping the bare-MOAT guard and the `pos` fallback only for pos-valid obstacles with empty tiles — mirrors how `BattleInfo.cpp:230` already consumes the same data. Add a test with an ABSOLUTE_OBSTACLE (`pos` invalid, cliff `ID`) asserting footprint cells render.

### REN-7 — Unguarded `markerAt[hex.toInt()]` write trusts a deserialize path that can carry INVALID entries — **(new; Must-escalation downset 2/2 to hardening)**
- **Location:** `server/battles/BattleTextViewRenderer.cpp:141-143`
- **Severity:** Should · **Confidence:** Medium · **bug_fix:** true (hardening; no reachable producer today)
- **Claim:** The new footprint loop writes `markerAt[hex.toInt()] = marker;` with no `isValid()` check on a stack `std::array<char, BFIELD_SIZE>`. `SpellCreatedObstacle::customSize` deserialization is a `resize(size())` (default-fills INVALID, `lib/battle/BattleHexArray.h:59-63`) followed by `set()` calls that **silently skip** invalid/duplicate entries without compacting (`:85-100`) — so a dirty serialized entry would leave an INVALID(-1) slot that `getAffectedTiles()` returns verbatim (`lib/battle/CObstacleInstance.cpp:216-219`) → `markerAt[-1]` OOB stack write (UB). **Consensus verdict (2/2): the mechanics hold, but reachability fails** — every mod-reachable producer sanitizes first (`SpellObstacleDescriptor::toObstacle()` fills `customSize` via the validating `insert()`, `luascript/api/battle/SpellObstacleDescriptor.cpp:45-46`, before any JSON exists; `BattleObstaclesChanged` is server→client only; saves are trusted local content). The wall loop (`:121-122`) and the engine's own consumer (`BattleInfo.cpp:283-284` re-inserts into a validating array) already use validity guards — the new loop is the one consumer that trusts the invariant raw.
- **Suggested fix:** `if(hex.isValid()) markerAt[hex.toInt()] = marker;` — one line, matching the file's own wall-loop pattern (`:121-122`) and `ObstacleHandler.cpp:71-74`.

### DOC-1R — "Nothing is rendered" fast-path wording overclaims for terminal packs — residual of DOC-1
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:27`; `server/battles/BattleMirrorServer.h:37-38` vs `server/battles/BattleMirrorServer.cpp:54-66`
- **Severity:** Should (doc-mismatch floor) · **Confidence:** Low · **bug_fix:** false
- **Claim:** The gate now correctly precedes `renderFrame()` for battle frames, but `visitBattleEnded`/`visitBattleCancelled` build their (tiny) frame strings unconditionally before `sendFrame` discards them when uninterested — so "with no viewers, nothing is rendered or sent" is, strictly, an overclaim. Cost is a few bytes; no reader harm, but the sentence and header comment are falsifiable as written.
- **Suggested fix:** Scope the wording ("no battle frame is rendered…; ended/cancelled notices are also never sent") or hoist the `interested` check in the two terminal visitors.

### DOC-6 — Session cap and busy rejection are user-visible but undocumented — **(new, doc gap)**
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:45-57` (Behavior notes / Limitations) vs `server/battles/BattleMirrorServer.h:91`, `.cpp:350-363`
- **Severity:** Should · **Confidence:** Medium · **bug_fix:** false
- **Claim:** Connections beyond 16 get a single `battle mirror busy, try again later` line and are closed (plus a once-per-saturation log line) — behavior an operator reading only the doc cannot explain when telnet connections are refused. Nothing in Behavior notes or Limitations mentions a viewer cap.
- **Suggested fix:** One bullet under Behavior notes: "At most 16 concurrent viewers; further connections are refused with a 'battle mirror busy' line."

### DOC-7 — Port valid range undocumented; description must say warn+clamp, not "rejected" — **(new, doc gap; basis corrected by consensus)**
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:23` vs `config/schemas/settings.json:635`, `server/CVCMIServer.cpp:99-106`
- **Severity:** Should (doc-gap floor) · **Confidence:** Low · **bug_fix:** false
- **Claim:** The port key is documented (default 3033, `0` = OS-assigned, actual port logged) but the valid range 0–65535 is not stated. **Per the schema-validation consensus (3/3 CLAIM A), the accurate description is**: an out-of-range value produces a startup *warning* in the log and is then *clamped* by the server — it is not rejected. (The docs worker's original basis claimed rejection and was overruled; any doc fix must not repeat that claim.)
- **Suggested fix:** Extend the port sentence: "valid range 0–65535; out-of-range values log a settings-validation warning at startup and are clamped by the server."

### TST-7 — NET-1 regression test cannot detect the race class it guards without TSan, and CI runs no sanitizer job — **(escalated from Could: coverage limitation on a Must-fix's guard)**
- **Location:** `test/server/battles/BattleMirrorSessionTest.cpp:297-327` (`TeardownFromNonIoThread`, honest in-code caveat at `:318`); `.github/workflows/` (no sanitizer configuration)
- **Severity:** Should (blocker-categories rule: effective coverage gap) · **Confidence:** High · **bug_fix:** false
- **Claim:** The test drives `closeAll()` from a real non-io thread against a live `io.run()` and asserts orderly teardown — if `closeAll()` reverted to a direct unposted cross-thread close, sockets still close, EOF still arrives, join still succeeds: the test passes. Only a TSan-instrumented run turns it into a real NET-1 tripwire, and no workflow in the repo enables sanitizers. The test does pin the posted-close semantics and is the practical black-box maximum — but the race class itself remains unguarded in CI.
- **Suggested fix:** Add a TSan (or ASan+TSan) CI build executing the mirror session tests, or mark the test as the TSan canary in workflow docs.

## Could

### TST-8 — Dead-stack legend-exclusion assertion survives via a singular/plural accident — **(new, test brittleness)**
- **Location:** `test/server/battles/BattleTextViewRendererTest.cpp:183-184`
- **Severity:** Could · **Confidence:** High · **bug_fix:** false
- **Claim:** The "dead stack absent from legend" check matches `victim->getName()` evaluated *after* death — which returns the plural ("Pikemen") — so the still-living defender Pikeman (singular) escapes the `EXPECT_EQ(find, npos)` by grammatical accident. Passes today and catches the REN-1 regression, but a fixture change to a same-singular/plural creature (or count ≥ 2) false-fails on a healthy renderer.
- **Suggested fix:** Capture the dead stack's legend string while alive (with side prefix) before dealing damage, or assert exact surviving legend-line count.

### TST-9 — `TeardownFromNonIoThread` has the harness's only unbounded waits — **(new, test robustness)**
- **Location:** `test/server/battles/BattleMirrorSessionTest.cpp:319-321`
- **Severity:** Could · **Confidence:** Medium · **bug_fix:** false
- **Claim:** `readFuture.get()` and `ioThread.join()` have no deadline (every other wait in the harness is bounded). Correct code cannot hang; a regression making `closeAllImpl` a no-op with the acceptor armed would hang until the CI job timeout instead of failing fast (a timeout is still a red job, just slow).
- **Suggested fix:** `readFuture.wait_for(30s)` + FAIL on timeout, then `io.stop()` before joining.

### TST-10 — Unguarded substring EXPECTs after a predicate-satisfied read — **(new, theoretical flake)**
- **Location:** `test/server/battles/BattleMirrorSessionTest.cpp:161-162`
- **Severity:** Could · **Confidence:** Low · **bug_fix:** false
- **Claim:** `readUntil` returns when the greeting predicate holds; the follow-up `find("battle #0")`/`find("00|")` assume the snapshot bytes arrived in the same read chunk. Holds deterministically for a single ≤2 KB loopback write in practice, but TCP does not guarantee it.
- **Suggested fix:** Fold `"battle #0"` into the `readUntil` predicate.

### REN-4 / REN-5 / REN-6 — carried forward unchanged from iteration 0 (agreed non-blocking)
- **REN-4 (Could/High):** siege render omits drawbridge + indestructible wall segments (`BattleTextViewRenderer.cpp:95`) — unchanged.
- **REN-5 (Could/Medium):** `substr(0, 3)` byte-wise truncation splits UTF-8 on localized names (`:31`) — unchanged.
- **REN-6 (Could/Medium):** `K`/`T` markers ignore wall state, destroyed keep still prints `K` (`:105-110`) — unchanged (and the new tests deliberately do not pin it in a way that would block the fix).

## Won't

None.

## Design-contract compliance matrix (updated)

| Contract item | Status | Evidence |
|---|---|---|
| Strictly read-only | ✅ Pass | No state writes; all const access incl. the new footprint `dynamic_cast<const ...>` (renderer worker, fresh pass) |
| Zero overhead when disabled | ✅ Pass | Null `unique_ptr`, one branch (`CVCMIServer.cpp:1294`); fast path now also skips *rendering* when enabled-but-viewerless |
| No serialization/networkPacks/savegame/protocol changes | ✅ Pass | Diff file list — no `lib/` files touched |
| Hook post-apply | ✅ Pass | `CVCMIServer.cpp:1289-1305` |
| Terminal frames from pack fields only | ✅ Pass + now *tested against a genuinely erased battle* (TST-5 fix) |
| Single-io-thread invariant | ✅ Pass (was ⚠️) | Hook + completions io-thread; teardown marshalled via post (3/3 consensus); wording gap NET-8 remains |
| Read-drain / IAC survival | ✅ Pass + tested | `BattleMirrorSessionTest.cpp:193-219` |
| Depth-1 coalescing | ✅ Pass + tested | `BurstCoalescesToLatestFrame` provably distinguishes depth-1 from unbounded/depth-2/over-drop |
| closeAll before networkHandler->stop() | ✅ Pass (was ⚠️) | Ordering kept (`CVCMIServer.cpp:194-198`); calling thread now safe (post) |
| Exceptions never propagate into server flow | ✅ Pass (was ⚠️) | Hook, ctor, restart, and accept→start paths all wrapped |
| Invariant documented in-code + doc | ✅ Pass (was ⚠️ doc gap) | `Battle_Telnet_Mirror.md:29`, header `:67-71`; residuals NET-8 / DOC-1R are wording precision, not absence |

## Observations (no severity, no fix demanded)

1. **Engine-wide, settings schema validation is warn-only** (return ignored, `const` node, no normalize — 3/3 consensus). The min/max added for NET-6 gives operators a startup warning, while the ctor clamp does the actual enforcing. Worth remembering for any future schema constraint that assumes rejection semantics.
2. **Pre-existing, out-of-branch:** `EServerState state` (`CVCMIServer.h`) is a non-atomic enum written from the client thread in embedded mode and read by io-thread handlers — a voter dissent tracing adjacent code. Present on develop; this branch neither introduces nor worsens it.
3. The `listenPort()` cross-thread nit is folded into NET-8's wording fix; it has no production caller today.
4. The harness's `TeardownFromNonIoThread` in-code TSan caveat (`BattleMirrorSessionTest.cpp:318`) is honest documentation — TST-7 addresses the gap it acknowledges.
5. NET-1's original "race window exists even with zero telnet clients" concern is fully closed by the posted teardown (the pending `async_accept` completion is now either drained serialized or abandoned uninvoked).

## Appendix — what was reviewed / process notes

- **Review set:** `git diff origin/develop...HEAD` (13 commits `3fefe2047..a3d7293b5`): `server/battles/BattleMirrorServer.{h,cpp}`, `server/battles/BattleTextViewRenderer.{h,cpp}`, `server/CVCMIServer.{h,cpp}`, `server/CMakeLists.txt`, `test/server/battles/{BattleMirrorSessionTest,BattleMirrorTest,BattleTextViewRendererTest}.cpp`, `test/CMakeLists.txt`, `config/schemas/settings.json`, `docs/developers/Battle_Telnet_Mirror.md`, `docs/developers/Code_Structure.md`, `docs/Readme.md`, plus the committed iteration-0 report (not itself reviewed).
- **Method:** orchestrator cross-cutting seam checks (schema↔ctor, CMake/test registration, header invariants, `reset()` threading lead) → resource-scout bundle → 4 parallel category workers, each instructed to verify iteration-0 findings **for correctness, not presence**, plus a fresh pass over the fix commits' own product → consensus rounds on the four high-stakes questions.
- **Consensus record:** (1) NET-1 fix verdict: **3/3 UPHOLD** fixed-correct-and-complete (five attack angles each: closure lifetime, post-vs-stop ordering, double/racing closeAll, in-flight-write session lifetime, dtor-repeat concurrency). (2) Schema-validation dispute (two workers had contradicted each other): **3/3 CLAIM A** — warn-only, clamp is sole enforcement, comment drifts. (3) REN-3 absolute-obstacle residual: **2/2 UPHOLD** Should. (4) REN-7 OOB escalation: **2/2 DOWNSET** to Should-hardening (reachability link fails: `SpellObstacleDescriptor.cpp:45-46` validating insert before JSON exists; packs server→client only). Two voters (one in round 3, one in round 4) failed on rate limits; in both rounds the two returned votes were unanimous and **any** third verdict would still preserve the recorded majority — verdicts are mathematically settled, noted as `2/2 returned`.
- **Degradations:** `.opencode/agent-resources/code-review/` templates/rules remain absent from this repo (same as iteration 0); workflow, schemas and consensus protocol constructed from built-in orchestrator instructions. Blocker-categories rule applied: TST-7 escalated from worker-filed Could (coverage limitation); NET-6R/NET-8/DOC-1R/DOC-6/DOC-7 held at the Should floor for comment/doc drift and gaps; REN-7 held at Should per the consensus downset rather than escalated to Must.
- **Step 7 (auto-apply) intentionally not offered** per caller instruction — the ship pipeline owns the fix loop for this branch.
- **Pure static review**: no builds or test suites were run by the review agents.
