# tinywl

Minimal Wayland compositor using **wlroots 0.19** and **scenefx**, with an external **Mango-style** config file (`binds=` / `axisbind=`), **float** and **scroller** layouts, and a **FIFO** for simple automation.

## Requirements

- C11 compiler, GNU **make**, **pkg-config**
- **wlroots** 0.19, **scenefx** 0.4, **wayland-server**, **wayland-protocols**, **wayland-scanner**, **libxkbcommon**, **GLESv2** (or compatible GL stack), **pixman**, Mesa/DRM as required by wlroots

`pkg-config` must see at least: `wlroots-0.19`, `scenefx-0.4`, `wayland-server`, `wayland-protocols`, `xkbcommon`, `glesv2`, `pixman-1`.

---

## NixOS and Nix (flake)

This repo is a **flake**. From the repository root:

| Goal | Command |
|------|---------|
| Development shell (same as `devShells.default`) | `nix develop` |
| Classic `nix-shell` entry | `nix-shell` or `nix-shell ./shell.nix` |
| Build installable package | `nix build` → `result/bin/tinywl`, `result/share/tinywl/config.conf` |
| Install to profile (example) | `nix profile install .` (or add the flake as a flake input and expose `packages.${pkgs.system}.default`) |

`shell.nix` reuses the flake dev shell via `builtins.getFlake`, so you need **flakes enabled** (default on current NixOS). The dev shell is defined only for **x86_64-linux** and **aarch64-linux** (see `flake.nix`).

**Declarative NixOS:** add this repository as a flake input, then install `packages.${pkgs.system}.default` into `environment.systemPackages` or your user profile.

---

## Other Linux distributions (no Nix)

Install the **development** packages that provide the pkg-config names above. Names differ by distro; examples (may need version bumps or extra repos):

| Distro | Hints (not exhaustive) |
|--------|-------------------------|
| **Debian / Ubuntu** | `build-essential`, `pkg-config`, `libwlroots-0.19-dev` (if available), `wayland-protocols`, `libwayland-dev`, `libxkbcommon-dev`, `libpixman-1-dev`, `libgl-dev` / `libgles2-mesa-dev`, plus **scenefx** if packaged or built from source |
| **Fedora** | `gcc`, `make`, `pkgconf-pkg-config`, `wlroots-devel`, `wayland-devel`, `wayland-protocols-devel`, `libxkbcommon-devel`, `mesa-libGLES-devel`, `pixman-devel`, **scenefx** from COPR/source if missing |
| **Arch Linux** | `base-devel`, `wlroots`, `scenefx`, `wayland`, `wayland-protocols`, `libxkbcommon`, `pixman`, `libglvnd` |

Many stable distros ship **wlroots &lt; 0.19** or no **scenefx**; you may need **AUR**, **PPA**, **COPR**, or to build those libraries from upstream before building tinywl.

Then:

```bash
make
sudo make install              # PREFIX defaults to /usr/local
# packaging:
make install DESTDIR=/tmp/stage PREFIX=/usr
```

---

## Build (quick reference)

```bash
make
```

The Makefile runs `wayland-scanner` to generate `protocol/xdg-shell-protocol.h` from `wayland-protocols` (path from `pkg-config --variable=pkgdatadir wayland-protocols`).

---

## Run

From another Wayland session (nested compositor):

```bash
./tinywl
```

From a VT with DRM/KMS (depends on your setup):

```bash
./tinywl
```

Useful **wlroots** environment variables still apply, for example `WLR_BACKENDS=wayland` to force the Wayland backend when nesting.

### Command-line options

| Option | Meaning |
|--------|---------|
| `-c path` | Use this config file instead of the default search path |
| `-o path` | After startup, write the `WAYLAND_DISPLAY` socket name to this file |
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

Requires an existing Wayland session and **Kitty** on your `PATH`.

- If **`nix`** is available and **`flake.nix`** exists, the script runs **`nix develop -c make`** (same lockfile as CI).
- Otherwise it runs plain **`make`**, after checking **`pkg-config`** for `wlroots-0.19` and `scenefx-0.4`.
- Set **`TINYWL_TEST_USE_NIX=0`** to force system `make` even when Nix is installed.

```bash
make test-nested
# or:
./scripts/nested-layout-test.sh
```

## Uninstall (Makefile)

```bash
sudo make uninstall
# or with the same PREFIX / DESTDIR used for install
```
