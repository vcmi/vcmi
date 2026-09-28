# Code Review: Battle Telnet Mirror (`ship/battle-telnet-mirror`)

- **Date:** 2026-09-28
- **Path:** A (branch-vs-base diff review, offline report)
- **Scope:** `git diff origin/develop...HEAD` on `ship/battle-telnet-mirror` (3 commits, 14 files, +1224/−1). All changes committed.
- **Reviewers:** 4 per-category worker agents (renderer, network/lifecycle/security, tests, docs) + orchestrator cross-cutting checks + 3-voter consensus rounds on the two high-stakes findings.
- **Severity taxonomy:** MoSCoW (Must / Should / Could / Won't). No repo-specific taxonomy found; MoSCoW default used.

## Verdict

One **Must** (cross-thread teardown race), twenty **Should**, three **Could**, zero **Won't**. The overall architecture is sound and honors most of the design contract; the Must is a narrowly-scoped teardown-threading defect with a small fix. Six Shoulds were escalated from Could by the synthesis blocker-categories rule (documentation gap/drift and missing-test-coverage findings never leave at Could); those are marked **(escalated)** below with the worker-filed severity noted.

## What the code does well (verified, not politeness)

- **Single-io-thread invariant holds for the hook and all completions.** Traced end-to-end: exactly one thread runs the io_context in every deployment (standalone `serverapp/EntryPoint.cpp:324` → `CVCMIServer::run()` → `NetworkHandler::run()` single blocking `context->run()`; embedded `client/ServerRunner.cpp:52-57`). Every `applyPack` caller (packet receive, timers, lobby) resolves to callbacks on that thread. Documented in-code at `BattleMirrorServer.h:27-28, 62-66` including the "if this ever becomes a pool, wrap in asio::post" caveat.
- **Depth-1 frame coalescing is correct** (`pendingFrame` latest-wins + `writing` flag, `BattleMirrorServer.cpp:175-182, 205-224`): exactly one outstanding `async_write` per socket; error path drops the session so a stale flag cannot strand it.
- **Read-drain survives telnet IAC** (`:191-203`): bytes discarded, no parser, no echo, loop restarted on every completion.
- **Terminal frames honor the pack-fields-only contract** (`BattleMirrorServer.cpp:52-65`): `BattleEnded`/`BattleCancelled` render from `pack.battleID`/`pack.victor` with no game-state battle lookup — no UAF on the erased battle.
- **Zero overhead when disabled**: null `unique_ptr`, one not-taken branch per pack (`CVCMIServer.cpp:1277`); settings read once in the ctor.
- **Hook placement and ordering correct**: post-`gh->gs->apply(pack)` (`CVCMIServer.cpp:1276-1287`), exceptions caught at the hook; `closeAll()` before `networkHandler->stop()` in SHUTDOWN (`:188-193`, ordering — the thread defect is NET-1, not the ordering).
- **Renderer geometry is right**: 17×11 via `GameConstants` (`BattleHex.h:19-22`), index math matches `setXY` (`BattleHex.h:123`), odd-row shift direction matches the engine (`BattleHex.h:138`) — the classic mirrored-offset bug is absent and pinned by tests.
- **Strictly read-only**: no game-state writes anywhere in the slice; all renderer access const.
- **No serialization / networkPacks / savegame / protocol changes** (diff file list confirmed).
- **CMake + test registration correct and CI-linked** (both new sources in `vcmiservercommon`, which `vcmitest` links); tests deterministic (no sockets, no sleeps), honest (drive the real visitor dispatch via `BattleTestFixture`), with robust substring/geometry assertions rather than fragile full-frame compares.
- **Docs**: example commands, config snippet keys, defaults, and all cross-links verified against code and schema; no stale references, no hardcoded issue numbers; in-code comments are WHY-oriented.

---

## Must

### NET-1 — `closeAll()` runs on the client thread in embedded mode, racing the io thread (UB) — **consensus 3/3 UPHOLD (Must, High)**
- **Location:** `server/CVCMIServer.cpp:188-193` / `server/battles/BattleMirrorServer.cpp:268-277`
- **Severity:** Must · **Confidence:** High · **bug_fix: true**
- **Claim:** In embedded-server mode, `setState(EServerState::SHUTDOWN)` is invoked from the client thread; the new SHUTDOWN branch then executes `battleMirror->closeAll()` — `acceptor.close()`, per-session `socket.close()`, `sessions.clear()`, `setInterested(false)` — **before** `networkHandler->stop()`, i.e. while the io thread is still inside `io_context::run()` executing mirror handlers that `insert`/`erase`/iterate the same lock-free `std::set` and touch the same asio objects. `std::set::clear()` racing `insert`/`erase` is a data race; cross-thread `close()` on sockets/acceptors with outstanding async ops violates asio's shared-object safety rules.
- **Basis (voter-traced):** `ServerThreadRunner::shutdown()` (`client/ServerRunner.cpp:64-67`) is called by `CServerHandler::~CServerHandler()` (`client/CServerHandler.cpp:74-84`, before `wait()`/join) and `CServerHandler::setState(CONNECTION_CANCELLED)` (`:357-363`) — both client threads. The io thread is pinned in `context->run()` by a work guard (`NetworkHandler.cpp:72-76`) and only joins later. `closeAll` has no mutex/strand/post marshalling (grep-verified across `server/`). Concurrent io-thread state access is reachable with zero telnet clients (the permanently-pending `async_accept` → `onAccepted` → `sessions.insert` at `BattleMirrorServer.cpp:284-306`; `closeAll`'s own `close()` calls enqueue `operation_aborted` completions that the still-running io thread executes against the cleared container). All other `setState(SHUTDOWN)` callers (`PlayerMessageProcessor.cpp:83`, `TurnOrderProcessor.cpp:270`, `CGameHandler.cpp:3876/3919/3969`, `CVCMIServer.cpp:592`, `GlobalLobbyProcessor.cpp:39/95/118`) run on the io thread and are safe. Pre-diff code was safe because only thread-safe `io_context::stop()` was called. Mirror is opt-in (default off), but when enabled the race is present in **every embedded shutdown**.
- **Suggested fix:** Marshal teardown onto the io thread: `post` the `closeAll()` body onto the mirror's `NetworkContext` (keeping a synchronous path when already on the io thread; posting onto a stopped context is benign — `~BattleMirrorServer` after `ServerThreadRunner::wait()` already closes everything single-threaded), or defer mirror close until after the server-thread join in `ServerThreadRunner::shutdown`. Update the header's "every entry point runs on the single network thread" claim (`BattleMirrorServer.h:62-66`) to cover the teardown path.
- **Regression test:** add a test that drives shutdown from a non-io thread against a live mirror on a pumped io_context and asserts an orderly teardown (see TST-3's loopback harness — same harness covers this).

## Should

### REN-1 — Dead stacks are rendered on the grid as living units — **consensus 3/3 UPHOLD (Should, High)**
- **Location:** `server/battles/BattleTextViewRenderer.cpp:139-148` (grid loop) vs `:187-190` (legend)
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** The grid-fill loop places every stack in `battle.stacks` with no `alive()` check; the legend in the same function filters `alive()`. Dead stacks persist server-side for the whole battle (death is an in-place `updateUnit`; erase happens only via the REMOVE op, used for ghost clones and killed tower shooters), so the mirror shows corpses as standing units — visually indistinguishable from live ones (no count/dead marker in cells) and inconsistent with the renderer's own legend and the client (corpse layer, hidden count box). **Worse (voter finding):** last-writer-wins at `:143/:147` lets a dead (e.g. summoned) stack **mask a living stack sharing its hex**.
- **Basis:** `GameStatePackVisitor.cpp:1406-1407, 1771-1774` (death → `updateUnit` only); `BattleInfo.cpp:798` `removeUnit` reached only via REMOVE (ghostPending stacks, `BattleFlowProcessor.cpp:213-224`; tower shooters `GameStatePackVisitor.cpp:1735-1736`); `CUnitState.cpp:497-499` (`alive()` = count>0), `:608-611` (position never cleared on death); client convention `client/battle/BattleStacksController.cpp:257-262`.
- **Suggested fix:** Skip `!stack->alive()` stacks in the grid loop (`:139-148`, both `getPosition()` and `occupiedHex()` branches), matching the legend. Consider whether to deliberately render corpses with a distinct marker instead (then document it).
- **Regression test:** kill a stack in a fixture battle and assert its grid cell empties while the legend already excludes it.

### REN-2 — LF-only line endings violate the CRLF-for-telnet contract
- **Location:** `server/battles/BattleTextViewRenderer.cpp:156,158,182,185,203,206,209,220-221,231-232`
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** All frames terminate lines with bare `"\n"`; the design contract requires CRLF for telnet correctness, and the mirror's own greeting uses `"\r\n"` (`BattleMirrorServer.cpp:168`) — showing intent. Raw-socket/minimal clients (nc piped onward) staircase on LF-only.
- **Suggested fix:** Emit `"\r\n"` at all line terminations in `renderBattleTextView`/`renderBattleSummary`/`renderBattleCancelled` (single mechanical change), and update baked-in test fragments (`BattleTextViewRendererTest.cpp:183-186`) together.

### REN-3 — Multi-hex obstacles render only their anchor hex
- **Location:** `server/battles/BattleTextViewRenderer.cpp:125-137`
- **Severity:** Should · **Confidence:** Medium · **bug_fix: true**
- **Claim:** Only `obstacle->pos` is marked; engine obstacles block multiple hexes (placement iterates `getBlockedTiles()`, `lib/battle/BattleInfo.cpp:226,277`; spell-created obstacles report real footprints via `getAffectedTiles()`/`customSize`, `lib/battle/CObstacleInstance.cpp:216-219`; moat patches span columns). A 2-hex rock shows one `#`; actually-blocked hexes appear passable. Also: a bare `CObstacleInstance` with `obstacleType == MOAT` (as synthesized in `BattleTextViewRendererTest.cpp:206-209`) hits `assert(0)` in the base `getAffectedTiles()` (`CObstacleInstance.cpp:44-50`) if the fix switches to it — guard or fall back to `pos`.
- **Suggested fix:** Mark every hex of the virtual `getAffectedTiles()` per obstacle, falling back to `pos` when empty or for non-SpellCreated MOAT instances.

### NET-2 — Accept loop not restarted after transient accept errors (silent permanent listener death)
- **Location:** `server/battles/BattleMirrorServer.cpp:293-308` (`onAccepted`)
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** For any non-abort accept error (EMFILE, ENFILE, ECONNABORTED, ENOMEM) `onAccepted` returns without calling `startAccept()` — one transient error permanently disables the mirror with a single log line. Compare `lib/network/NetworkServer.cpp:33-38`.
- **Suggested fix:** On non-abort errors, log and still `startAccept()` (optionally with a short `steady_timer` backoff for EMFILE); return without restarting only on `operation_aborted`.

### NET-3 — No cap on session count: unauthenticated connections can exhaust process fds
- **Location:** `server/battles/BattleMirrorServer.cpp:302`
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** Every accepted connection enters `sessions` unbounded; a local user (or anyone, if hostname is set to `0.0.0.0`/`::`) can open enough idle telnet connections to hit EMFILE, which also starves the **main game acceptor** in the same process. Tempered by default-off + loopback default + documented trust model, but the blast radius is the game server, not just the mirror.
- **Suggested fix:** Cap concurrent sessions (e.g. 8-16); when full, send a short "mirror busy" line and close (or just close + rate-limited log).

### NET-4 — Exception in the accept→start render path escapes `io_context::run()` and kills the network thread
- **Location:** `server/battles/BattleMirrorServer.cpp:293-308` → `Session::start` (`:166-173`) → `snapshotFrame`/`renderFrame` (`:139-148`)
- **Severity:** Should · **Confidence:** Medium · **bug_fix: true**
- **Claim:** The applyPack hook wraps mirror calls in try/catch (`CVCMIServer.cpp:1279-1286`), but `onAccepted → session->start()` runs the **same renderer pipeline** unwrapped; any exception there propagates out of `NetworkHandler::run()` (`NetworkHandler.cpp:72-76`) and kills the server network thread. Read/write completions are effectively no-throw (error_code overloads); the accept path is the one unwrapped risk surface.
- **Suggested fix:** Wrap `Session::start()`'s body (or its call site in `onAccepted`) in try/catch: log via `logNetwork` and `dropSession(session)`.

### NET-5 — `controller.gameState` dangles across game restart; snapshot in that window is a UAF read
- **Location:** `server/battles/BattleMirrorServer.cpp:59` (stored at `:94-99`, consumed at `:139-148`)
- **Severity:** Should · **Confidence:** Medium · **bug_fix: true**
- **Claim:** `onPackApplied` stores `&state` each call but nothing clears it. `CVCMIServer::prepareToRestart` (`server/CVCMIServer.cpp:266-286`) sets `gh = nullptr`, dropping the old `CGameState`. A viewer connecting before the first pack of the new game — while `current` is still set — gets `snapshotFrame → renderFrame` reading `gameState->getBattle(...)` on freed memory. Narrow window, memory-unsafe consequence.
- **Suggested fix:** Reset mirror state on restart (clear `gameState`, `current`, `logRing` from `prepareToRestart` where `gh` is reset), or version/generation-check before rendering a snapshot.

### NET-6 — Mirror port wraps silently on out-of-range config (no schema min/max)
- **Location:** `server/CVCMIServer.cpp:99`; `config/schemas/settings.json` (`server.battleMirror.port`)
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** `static_cast<uint16_t>(mirrorConfig["port"].Integer())` with no schema `minimum`/`maximum`: 70000 → binds 4464, −1 → 65535, silently.
- **Suggested fix:** Add `"minimum": 0, "maximum": 65535` to the schema (and optionally clamp + warn in the ctor).

### NET-7 — `start()` failure path can leave the acceptor open (escalated from Could: real error-path defect)
- **Location:** `server/battles/BattleMirrorServer.cpp:250-266`
- **Severity:** Should (worker-filed: Could) · **Confidence:** High · **bug_fix: true**
- **Claim:** If `open()` succeeds but `bind`/`listen` throws, the catch only logs; an open-but-unlistening acceptor holds an fd until destruction while the mirror reports disabled.
- **Suggested fix:** `acceptor.close(ec)` (error_code overload) in the catch before logging.

### DOC-1 — Doc and header comment claim rendering is gated on viewer presence; code renders unconditionally
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:27`; `server/battles/BattleMirrorServer.h:36-37`
- **Severity:** Should · **Confidence:** High · **bug_fix: true**
- **Claim:** Both say "no frame is rendered" with no viewers, but `renderAndSend` (`BattleMirrorServer.cpp:132-137`) always builds the full frame string; only `sendFrame` (`:151-155`) checks `interested`. Rendering work is done and discarded per battle pack with zero viewers — either the claim or the fast path is wrong.
- **Suggested fix:** Either fix the wording in both places ("…only **sent** when at least one viewer is connected") or move the `interested` gate ahead of `renderFrame()` so code matches the documented fast path (preferred — also saves the per-pack render cost the doc promises).

### DOC-2 — Single-io-thread / no-locks invariant missing from the developer doc
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:25-27`
- **Severity:** Should · **Confidence:** Medium · **bug_fix: false**
- **Claim:** The design contract requires the invariant documented in-code **and** in the doc. In-code is satisfied (`BattleMirrorServer.h:27-28, 62-66`); the doc only says "shared network io_context (no extra threads)" and never states the lock-free design or the io-pool consequence.
- **Suggested fix:** Extend the "How it works" sentence: "every entry point (pack hook, accepts, reads, writes) runs on the single network thread, so the mirror uses no locks by design; if the io_context ever becomes a pool, all entry points must be marshalled via `boost::asio::post`."

### DOC-3 — Depth-1 frame coalescing undocumented — **(escalated from Could: documentation gap)**
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:43-47`
- **Severity:** Should (worker-filed: Could) · **Confidence:** High · **bug_fix: false**
- **Claim:** Frames in flight are replaced (latest-wins, `BattleMirrorServer.cpp:175-182`) — observable viewer behavior (bursty battles skip intermediate frames on slow viewers) that "Behavior notes" never mentions.
- **Suggested fix:** Add a bullet: "While a frame is still being written, a newer frame replaces the queued one — slow viewers see the latest state; intermediate frames may be skipped."

### DOC-4 — Shutdown-ordering and exception-containment contracts undocumented — **(escalated from Could: documentation gap)**
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:25-27`
- **Severity:** Should (worker-filed: Could) · **Confidence:** High · **bug_fix: false**
- **Claim:** The implemented contracts — closeAll before `networkHandler->stop()` (`CVCMIServer.cpp:188-192`) and mirror errors caught so they never affect server flow (`:91-107, 1277-1287`) — are absent from the doc's "How it works".
- **Suggested fix:** Add one sentence covering both (and, once NET-1 is fixed, describe the actual teardown threading).

### DOC-5 — Gate legend incomplete: `G` also covers BLOCKED — **(escalated from Could: documentation mismatch)**
- **Location:** `docs/developers/Battle_Telnet_Mirror.md:38`
- **Severity:** Should (worker-filed: Could) · **Confidence:** High · **bug_fix: false**
- **Claim:** `gateMarker` (`BattleTextViewRenderer.cpp:40-51`) returns `G` for any state other than OPENED/DESTROYED, including `EGateState::BLOCKED` (creature-held, `lib/constants/Enumerations.h:91-95`); the doc says only "closed".
- **Suggested fix:** "gate `G` closed or blocked / `g` opened / `X` destroyed".

### TST-1 — No test for double-wide creatures occupying their second hex
- **Location:** `test/server/battles/BattleTextViewRendererTest.cpp:105` (suite)
- **Severity:** Should · **Confidence:** High · **bug_fix: false**
- **Claim:** The renderer explicitly writes `occupiedHex()` (`BattleTextViewRenderer.cpp:145-147`) but every renderer test uses single-hex Pikemen; a regression dropping or mis-writing the second hex passes the suite.
- **Suggested fix:** `DoubleWideStackOccupiesBothHexes`: `addStack` a wide creature (e.g. champion), render, assert both `getPosition()` and `occupiedHex()` cells carry the tag.

### TST-2 — Controller's `BattleCancelled` terminal path untested
- **Location:** `test/server/battles/BattleMirrorTest.cpp:66` (suite)
- **Severity:** Should · **Confidence:** High · **bug_fix: false**
- **Claim:** Only `visitBattleEnded` is exercised at controller level (`:80-89`); `visitBattleCancelled` (`BattleMirrorServer.cpp:60-65`) has zero controller coverage despite being a named contract item.
- **Suggested fix:** `CancelledBattleSendsTerminalFrameFromPackFields`: interested + sink, `applyStart`, apply a `BattleCancelled` with the battle ID, assert terminal frame content, `hasBattle()==false`, and no further frames for that ID.

### TST-3 — Session/asio layer (coalescing, IAC survival, closeAll ordering) has no regression test
- **Location:** `test/server/battles/BattleMirrorTest.cpp:20` (fixture instantiates only `BattleMirrorController`)
- **Severity:** Should · **Confidence:** Medium · **bug_fix: false**
- **Claim:** Three stated contract guarantees live entirely in the untested `Session` layer: depth-1 write (`BattleMirrorServer.cpp:214`), read-drain/IAC (`:193`), closeAll ordering (`:268-277`). A regression to two concurrent `async_write`s corrupts frames in production with zero test signal. The sink-injection seam is a legitimate unit boundary, which keeps this at Should.
- **Suggested fix:** Loopback test on 127.0.0.1:0 driving a real `NetworkContext` with `poll()` (no sleeps): (1) greeting+snapshot on connect mid-battle; (2) 3 back-to-back packs before poll → latest frame arrives uncorrupted, connection survives; (3) client sends `\xFF\xFD\x2F` (IAC DO) → session alive, frames continue; (4) `closeAll()` → orderly EOF. If the private nested `Session` blocks this, hoist it to a TU-local testable class. The same harness can pin the NET-1 teardown threading.

### TST-4 — 11 of 15 battle-pack visitors never exercised
- **Location:** `test/server/battles/BattleMirrorTest.cpp:152` (suite)
- **Severity:** Should · **Confidence:** Medium · **bug_fix: false**
- **Claim:** BattleNextRound, BattleStackMoved, BattleUnitsChanged, BattleAttack, BattleSpellCast, StacksInjured, BattleObstaclesChanged, CatapultAttack, BattleSetStackProperty, BattleTriggerEffect, BattleUpdateGateState route through `followBattle` but are untested; a dropped/misnamed override compiles silently (empty `ICPackVisitor` defaults) and the mirror would freeze between log lines.
- **Suggested fix:** `BattleEventPacksEachProduceAFrame`: apply default-constructed instances of the 11 packs with `battleID` set; assert `sinkFrames.size()` increments by exactly 1 per pack.

### TST-5 — Terminal-frame test never reproduces its stated precondition ("battle erased during apply")
- **Location:** `test/server/battles/BattleMirrorTest.cpp:42-45`
- **Severity:** Should · **Confidence:** Medium · **bug_fix: false**
- **Claim:** The fixture's `apply()` feeds only the controller, so when `BattleEnded` is applied the battle is still alive in `gameState` — a regression that dereferences `gameState->getBattle(pack.battleID)` in `visitBattleEnded` would be a production UAF yet pass this test.
- **Suggested fix:** In the lifecycle test (and the new cancelled test), first `server.applyPack(ended)` (RecordingGameServer applies to `gameState`, `BattleTestFixture.cpp:42-46`) so the battle is actually erased, then `controller.onPackApplied(...)`.

### TST-6 — Marker-state mapping only partially pinned — **(escalated from Could: missing test coverage)**
- **Location:** `test/server/battles/BattleTextViewRendererTest.cpp:216` (SiegeWalls)
- **Severity:** Should (worker-filed: Could) · **Confidence:** High · **bug_fix: false**
- **Claim:** Only DESTROYED wall / CLOSED gate / KEEP are asserted. Untested: DAMAGED `x` / INTACT `=`, gate OPENED `g` / DESTROYED `X`, towers `T`, ANSI reverse-video around the active cell (`\x1b[7m`/`\x1b[27m`, `BattleTextViewRenderer.cpp:36`), dead-stack legend exclusion (`:189-190`), stack-over-marker precedence (`:171-173`).
- **Suggested fix:** Extend SiegeWalls (DAMAGED/INTACT walls, OPENED/DESTROYED gate, towers); add assertions for the active-cell escape wrapper, a killed stack's legend disappearance, and stack-wins-over-obstacle.

## Could

### REN-4 — Siege rendering omits drawbridge and indestructible wall segments
- **Location:** `server/battles/BattleTextViewRenderer.cpp:95`
- **Severity:** Could · **Confidence:** High · **bug_fix: false**
- **Claim:** The loop covers only `EWallPart::KEEP..GATE` (0..7); `INDESTRUCTIBLE_PART_OF_GATE`/`INDESTRUCTIBLE_PART` hexes (95, 45, 62, 112, 147, 165 per `lib/battle/CBattleInfoCallback.cpp:85-99`) and drawbridge hexes (`GATE_BRIDGE`/`GATE_OUTER`, `BattleHex.h:53-55`) render blank even when the bridge is the walking path. Placement of what IS drawn is correct (engine's `wallPartToBattleHex`).
- **Suggested fix:** Mark bridge hexes when `getGateState() == OPENED` and `#` at the indestructible hexes from the canonical table.

### REN-5 — `substr(0, 3)` truncates creature names byte-wise (UTF-8 split on localized builds)
- **Location:** `server/battles/BattleTextViewRenderer.cpp:31`
- **Severity:** Could · **Confidence:** Medium · **bug_fix: false**
- **Claim:** Byte counting + fixed padding assumes 1 byte = 1 column; non-ASCII localized names emit invalid UTF-8 fragments and shift the cell grid. Harmless for English names.
- **Suggested fix:** Truncate on UTF-8 code-point boundaries (skip continuation bytes) or compute display width before padding.

### REN-6 — KEEP/tower markers ignore wall state (destroyed keep still prints `K`/`T`)
- **Location:** `server/battles/BattleTextViewRenderer.cpp:105-110`
- **Severity:** Could · **Confidence:** Medium · **bug_fix: false**
- **Claim:** Walls use `wallMarker` by state, but `K`/`T` are hardcoded; tower/keep state does degrade (`SiegeInfo::applyDamage`, `lib/battle/SiegeInfo.cpp:23`; tower destruction couples to shooter removal, `GameStatePackVisitor.cpp:1736`).
- **Suggested fix:** Render `X`/lowercase when `wallState == EWallState::DESTROYED`.

## Won't

None.

## Design-contract compliance matrix

| Contract item | Status | Evidence |
|---|---|---|
| Strictly read-only | ✅ Pass | No state writes; all const access (NET worker clean #9) |
| Zero overhead when disabled | ✅ Pass | Null `unique_ptr`, one branch (`CVCMIServer.cpp:1277`) |
| No serialization/networkPacks/savegame/protocol changes | ✅ Pass | Diff file list — no `lib/` files touched |
| Hook post-apply | ✅ Pass | After `gh->gs->apply(pack)` (`CVCMIServer.cpp:1276`) |
| Terminal frames from pack fields only | ✅ Pass | `BattleMirrorServer.cpp:52-65`, no gs lookup (REN worker clean) |
| Single-io-thread invariant | ⚠️ Split | Hook + all completions: **confirmed** (full trace). Teardown via `closeAll`: **violated** (NET-1) |
| Read-drain / IAC survival | ✅ Pass | `BattleMirrorServer.cpp:191-203` (but untested — TST-3) |
| Depth-1 coalescing | ✅ Pass | `:175-182, 205-224` (but untested — TST-3) |
| closeAll before networkHandler->stop() | ⚠️ Split | Ordering correct (`:188-193`); calling thread unsafe (NET-1) |
| Exceptions never propagate into server flow | ⚠️ Split | Hook: caught (`:1279-1286`). Accept→start render path: unwrapped (NET-4) |
| Invariant documented in-code | ✅ Pass (doc gap → DOC-2) | `BattleMirrorServer.h:27-28, 62-66` |

## Observations (no severity, no fix demanded)

1. **Settings schema `required` precedent is safe**: adding `battleMirror` to the server object's `required` matches the existing pattern (`localHostname` etc. were already required and user files omit them); all new properties carry defaults, so validation is satisfied by defaults. No action needed.
2. **CMake/test registration** is correctly ordered and both new targets compile into CI via `vcmiservercommon` → `vcmitest`.
3. **`hostname` empty → `127.0.0.1` fallback** in the ctor is a reasonable guard.
4. **No-auth/loopback trust model is documented** in `Battle_Telnet_Mirror.md:5-7`; setting hostname to `0.0.0.0` exposes battle data (player names, log) unauthenticated — acceptable for a debug tool, worth remembering if this ever ships enabled-by-default (ties into NET-3).
5. Voters confirmed the NET-1 race window exists **even with zero telnet clients** (the pending `async_accept` alone), so "no viewers connected" is not a mitigation.

## Appendix — what was reviewed / process notes

- **Review set:** `git diff origin/develop...HEAD`: `server/battles/BattleTextViewRenderer.{h,cpp}` (new), `server/battles/BattleMirrorServer.{h,cpp}` (new), `server/CVCMIServer.{h,cpp}`, `server/CMakeLists.txt`, `test/server/battles/{BattleTextViewRendererTest,BattleMirrorTest}.cpp` (new), `test/CMakeLists.txt`, `config/schemas/settings.json`, `docs/developers/Battle_Telnet_Mirror.md` (new), `docs/developers/Code_Structure.md`, `docs/Readme.md`.
- **Method:** orchestrator cross-cutting checks (hook placement, settings-key alignment, contract seams, CMake registration) + resource-scout bundle + 4 parallel category workers + 3-voter consensus on the two high-stakes findings (NET-1: 3/3 UPHOLD Must; REN-1: 3/3 UPHOLD Should). Consensus was **not** skipped or degraded.
- **Degradations:** `.opencode/agent-resources/code-review/` templates/rules were absent from this repo; the workflow, worker prompt schema, and consensus protocol were constructed from the orchestrator's built-in instructions instead. Severity escalation per the blocker-categories rule applied to NET-7, DOC-3, DOC-4, DOC-5, TST-6 (worker-filed severities preserved inline).
- **Step 7 (auto-apply) intentionally not offered** per caller instruction — the ship pipeline owns the fix loop for this branch.
- **Pure static review**: no builds or test suites were run by the review agents.
