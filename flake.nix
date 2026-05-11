{
  description = "tinywl — wlroots + scenefx compositor (dev shell)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in
    {
      packages = forAllSystems (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "tinywl";
          version = "0.1.0";
          src = ./.;
          nativeBuildInputs = with pkgs; [ pkg-config wayland-scanner ];
          buildInputs = with pkgs; [
            scenefx
            wlroots_0_19
            wayland
            wayland-protocols
            libxkbcommon
            mesa
            libglvnd
            libdrm
            pixman
          ];
          buildPhase = ''
            runHook preBuild
            make tinywl
            runHook postBuild
          '';
          installPhase = ''
            runHook preInstall
            mkdir -p $out/bin $out/share/tinywl
            install -Dm755 tinywl $out/bin/tinywl
            install -Dm644 share/tinywl/config.conf $out/share/tinywl/config.conf
            runHook postInstall
          '';
        };
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            pkg-config
            wayland-scanner
            scenefx
            wlroots_0_19
            wayland
            wayland-protocols
            libxkbcommon
            mesa
            libglvnd
            clang
            gnumake
            pixman
            kitty
          ];
        };
      });
    };
}
