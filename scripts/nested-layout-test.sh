#!/usr/bin/env bash
# Build tinywl, run it nested in the current Wayland session, spawn several
# Kitty windows (scroller stress), then drive layout / scroll / focus via
# TINYWL_CMD_FIFO.
#
# Requires: an existing Wayland session (WAYLAND_DISPLAY set), kitty, and a
# working `make` (system packages or Nix). Build step:
#   - If flake.nix exists and `nix` is available: `nix develop -c make`
#   - Else: plain `make` (set TINYWL_TEST_USE_NIX=0 to skip Nix even when installed)
#
# Environment:
#   TINYWL_TEST_KITTY_COUNT   Number of Kitty windows (default: 5)
#   TINYWL_TEST_USE_NIX      Set to 0 to force system make (see README)
#
# Usage: ./scripts/nested-layout-test.sh
# Exit: compositor exit status (0 if quit command ran)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

FIFO="$(mktemp -u "${TMPDIR:-/tmp}/tinywl-cmd.XXXXXX")"
SOCKFILE="${TMPDIR:-/tmp}/tinywl-nested-wayland-$$"

cleanup() {
	rm -f "$FIFO" "$SOCKFILE"
	kill ${KITTY_PIDS:-} 2>/dev/null || true
	kill "${TWLPID:-}" 2>/dev/null || true
}
trap cleanup EXIT

rm -f "$SOCKFILE"

mkfifo "$FIFO"
export TINYWL_CMD_FIFO="$FIFO"
# Nested GL (e.g. kitty inside tinywl inside Sway) is fragile on some Mesa stacks;
# skip heavy scenefx toplevel decorations for this automated test.
export TINYWL_NO_DECOR=1

build_tinywl() {
	if [[ "${TINYWL_TEST_USE_NIX:-1}" != "0" ]] && command -v nix >/dev/null 2>&1 && [[ -f "$ROOT/flake.nix" ]]; then
		nix develop "$ROOT" -c make
		return
	fi
	if ! command -v pkg-config >/dev/null 2>&1; then
		echo "[test] pkg-config not found; install build deps or use Nix (see README)." >&2
		exit 1
	fi
	if ! pkg-config --exists wlroots-0.19 scenefx-0.4 wayland-server 2>/dev/null; then
		echo "[test] missing pkg-config modules (wlroots-0.19, scenefx-0.4, wayland-server)." >&2
		echo "[test] Install distro dev packages or run: nix develop -c make" >&2
		exit 1
	fi
	make
}

echo "[test] building…"
build_tinywl

echo "[test] starting compositor (nested)…"
./tinywl -c "$ROOT/share/tinywl/config.conf" -o "$SOCKFILE" &
TWLPID=$!

for _ in $(seq 1 200); do
	if [[ -s "$SOCKFILE" ]]; then
		break
	fi
	sleep 0.05
done
if [[ ! -s "$SOCKFILE" ]]; then
	echo "[test] timeout waiting for WAYLAND_DISPLAY" >&2
	exit 1
fi

NEST="$(tr -d '\n' <"$SOCKFILE")"
export WAYLAND_DISPLAY="$NEST"

echo "[test] WAYLAND_DISPLAY=$NEST"

RUNDIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
for _ in $(seq 1 200); do
	if [[ -S "$RUNDIR/$NEST" ]]; then
		break
	fi
	sleep 0.05
done

# Compositor opens the FIFO for read before wlr_backend_start finishes; open our
# writer now so we never block forever on exec 3>"$FIFO", then spawn clients.
echo "[test] opening command FIFO writer…"
exec 3>"$FIFO"

KITTY_COUNT="${TINYWL_TEST_KITTY_COUNT:-5}"
if ! [[ "$KITTY_COUNT" =~ ^[1-9][0-9]*$ ]]; then
	echo "[test] TINYWL_TEST_KITTY_COUNT must be a positive integer" >&2
	exit 1
fi

echo "[test] launching ${KITTY_COUNT} kitty windows (class tinywl-test)…"
KITTY_PIDS=
for i in $(seq 1 "$KITTY_COUNT"); do
	kitty --class tinywl-test --title "tinywl-scroll-${i}/${KITTY_COUNT}" &
	KITTY_PIDS="${KITTY_PIDS:+$KITTY_PIDS }$!"
	sleep 0.4
done

# Let surfaces map and initial configures settle before driving the FIFO.
sleep 3

send() {
	echo "[test] -> $1"
	printf '%s\n' "$1" >&3
}

# --- Scroller-focused scenario (multiple stacked toplevels) ---
send "layout scroller"
sleep 2
send "scroll 80"
sleep 1.5
send "scroll 120"
sleep 1.5
send "scroll 100"
sleep 1.5
send "scroll -150"
sleep 1.5
send "scroll -80"
sleep 2

send "layout float"
sleep 1.5
send "cycle_focus"
sleep 1.5

send "layout scroller"
sleep 2
send "scroll 200"
sleep 1.5
send "scroll -100"
sleep 2

send "layout float"
sleep 1.5
send "quit"

echo "[test] waiting for compositor…"
wait "$TWLPID"
exit $?
