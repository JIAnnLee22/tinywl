# tinywl

Minimal Wayland compositor using **wlroots 0.19** and **scenefx**, with an external **Mango-style** config file (`binds=` / `axisbind=`), **float** and **scroller** layouts, and a **FIFO** for simple automation.

## Requirements

- C11 compiler, GNU make, `pkg-config`
- **wlroots** 0.19, **scenefx** 0.4, **wayland-server**, **wayland-protocols**, **wayland-scanner**, **libxkbcommon**, **GLESv2**, **pixman**, Mesa/GL stack as required by wlroots

On **Nix** (recommended), use the flake dev shell or package (see below).

## Build

### With Nix

```bash
nix develop   # or: nix-shell
make
```

Build the flake package (from the repository root):

```bash
nix build
./result/bin/tinywl -h
```

### Without Nix

Install the same development packages your distribution provides (names vary), then:

```bash
make
```

The Makefile runs `wayland-scanner` to generate `protocol/xdg-shell-protocol.h` from `wayland-protocols`.

## Run

From another Wayland session (nested compositor):

```bash
./tinywl
```

From a VT with DRM/KMS (typical wlroots behavior; depends on your setup):

```bash
./tinywl
```

Useful **wlroots** environment variables still apply, for example `WLR_BACKENDS=wayland` to force the Wayland backend when nesting.

### Command-line options

| Option | Meaning |
|--------|---------|
| `-c path` | Use this config file instead of the default search path |
| `-o path` | After startup, write the `WAYLAND_DISPLAY` socket name to this file (UTF-8, no newline required by clients; scripts often use `tr -d '\n'`) |
| `-p` | Parse config only, then exit (`0` if OK) |
| `-s cmd` | After the display is ready, run `cmd` via `/bin/sh -c` (double-forked) |

Default config path if `-c` is omitted: `$XDG_CONFIG_HOME/tinywl/config.conf`, or `~/.config/tinywl/config.conf`.

A sample config ships at `share/tinywl/config.conf`.

## Configuration

See `share/tinywl/config.conf`. Lines look like:

- `binds=MODIFIERS,KEYSYM,action,arg`
- `axisbind=MODIFIERS,vertical|horizontal,scroll_viewport,STEP`

Actions include `quit`, `cycle_focus`, `set_layout` with `float` or `scroller`, and axis actions such as `scroll_viewport`.

## Automation (`TINYWL_CMD_FIFO`)

If **`TINYWL_CMD_FIFO`** is set to a filesystem FIFO path, the compositor opens it for reading (after the Wayland socket is live) and processes newline-terminated commands:

- `quit`
- `layout float` / `layout scroller`
- `scroll N` (number; default step if omitted)
- `cycle_focus`

Empty lines and lines starting with `#` are ignored.

## Nested sessions and `TINYWL_NO_DECOR`

Running a GL client (e.g. **Kitty**) inside a nested compositor on some **Mesa / AMDGPU** stacks can crash the compositor when full scenefx decorations (shadows, backdrop blur, optimized blur sizing) are enabled.

Set **`TINYWL_NO_DECOR`** to any non-empty value other than `0` to disable those paths for a more stable nested session. The nested test script sets this automatically.

## Nested layout test

Requires an existing Wayland session, **Kitty**, and Nix with the flake dev inputs (or equivalent `nix-shell -p …` as in the script):

```bash
make test-nested
# or:
./scripts/nested-layout-test.sh
```

The script builds with `make`, starts `tinywl` with `-o` and `TINYWL_CMD_FIFO`, waits for the socket under `$XDG_RUNTIME_DIR`, then drives layout commands over the FIFO.

## Install

This repository’s `Makefile` builds the `tinywl` binary only. Install by copying `tinywl` to your `PATH` and installing `share/tinywl/config.conf` where you want the default sample (for example `/usr/local/share/tinywl/`), or use **`nix build`** and deploy the `result/` tree.