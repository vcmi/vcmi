# Battle Telnet Mirror

A server-side, read-only battle spectator streamed as ANSI text over a plain TCP socket. While a battle runs on the game server, `nc 127.0.0.1 3033` (or `telnet 127.0.0.1 3033`) shows a live-updating 17x11 hex-grid rendering: stacks with creature identity, counts and HP, the active stack highlighted, obstacles, siege walls/gate/keep/towers when relevant, the round number, and a tail of the battle log.

## Security

The mirror has **no authentication** and is **loopback-only by default**. It is a local debugging and spectating tool — do not expose the port to untrusted networks. If the mirror is ever enabled on a non-loopback hostname, the 16-viewer session cap is the only flood guard — keep it loopback.

## Enabling

Disabled by default; when disabled there is no listener at all (check with `ss -ltn | grep 3033`). To enable, add to the user settings file (the JSON in the user data directory, not a repo file):

```json
{
	"server" : {
		"battleMirror" : {
			"enabled" : true
		}
	}
}
```

Optional keys: `"hostname"` (default `"127.0.0.1"`) and `"port"` (default `3033`; valid range 0-65535, out-of-range values log a settings-validation warning at startup and are clamped by the server; `0` = OS-assigned, the actual port is logged on startup).

## How it works

A post-apply hook in `CVCMIServer::applyPack` feeds every applied client pack to `BattleMirrorController` (server/battles/BattleMirrorServer.cpp). The controller filters battle packs, keeps an 8-line log ring, and re-renders a frame via the pure renderer in server/battles/BattleTextViewRenderer.cpp: a frame is rendered and sent only when at least one viewer is connected — with no viewers, nothing is rendered or sent. `BattleMirrorServer` is a minimal boost.asio acceptor running on the shared network io_context (no extra threads). Viewers connect read-only: their input is drained and discarded.

Every entry point — the pack hook, accepts, reads and writes — runs on the single network thread, so no locks are needed by design; should the io_context ever become a pool, all entry points must be marshalled onto it via `boost::asio::post`. On server shutdown the mirror is closed before the network handler stops: `closeAll()` posts its body onto the io thread (the shutdown path can run on a non-io thread in client-embedded mode), and if the context was already stopped, the destructor closes the acceptor and sockets idempotently. Mirror errors anywhere in the hook are caught and logged and never affect server flow.

## Frame legend

Each frame clears the screen (ANSI escape sequences — use an ANSI terminal). Format:

- Header: `VCMI battle mirror - battle #<id>, round <N>, active: <stack>`
- Grid rows `00|`..`10|`, odd rows indented; one 4-character cell per hex:
	- `a`/`d` + 3-letter creature abbreviation, e.g. `aPik` — attacker/defender stack; wide creatures fill both hexes
	- the active stack is shown in inverse video
	- `#` usual/absolute obstacle, `%` spell-created obstacle, `~` moat
 	- siege: `K` keep, `T` tower (both `X` once destroyed), `W` indestructible wall segment, drawbridge `=` lowered while the gate stands open, gate `G` closed or blocked (creature-held) / `g` opened / `X` destroyed, walls `X` destroyed / `x` damaged / `=` intact / `H` reinforced
	- `A`/`D` attacker/defender hero position
- One legend line per living stack: `<side> <name>  count <N>  HP <left>/<max>`
- Log tail lines prefixed with `| `

## Behavior notes

- Mirrors the **most recently active** battle: concurrent battles flip the view to the latest activity; per-viewer battle selection is not supported.
- On connect the server sends a greeting and, if a battle is already running, an immediate full snapshot frame.
- Frame updates are coalesced depth-1, latest-wins: while a frame is still being written, a newer frame replaces the queued one — slow viewers see the latest state; intermediate frames may be skipped.
- After a battle ends, the final summary frame stays on screen and the connection stays open (idle) — the next battle reuses it.
- At most 16 concurrent viewers; further connections are refused with a "battle mirror busy" line; saturation is logged once per continuous saturation period.

## Limitations

- Single battle view (see above).
- ANSI terminal recommended: each frame clears the screen.
- Battle log text uses the server locale.
- Viewer input is ignored by design.
