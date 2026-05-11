# Loads the same environment as `nix develop` / the flake `devShells.default`.
# Use from the repo root:
#   nix-shell
#   nix-shell ./shell.nix
# Requires Nix with flakes (getFlake). Prefer `nix develop` when available.
let
  flake = builtins.getFlake (toString ./.);
  system = builtins.currentSystem;
in
flake.devShells.${system}.default
