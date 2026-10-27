{
  description = "Terminal synced lyrics for MPRIS music players";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in {
      packages = forAll (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "lyrics-term";
          version = "2.1.0";

          src = ./.;

          nativeBuildInputs = [
            pkgs.meson
            pkgs.ninja
            pkgs.pkg-config
          ];

          buildInputs = [
            pkgs.dbus
            pkgs.curl
            pkgs.freetype
            pkgs.nlohmann_json
          ];

          mesonFlags = [ "-Dtests=true" ];

          doCheck = true;

          meta = with pkgs.lib; {
            description = "Terminal synced lyrics for any MPRIS-compatible music player";
            homepage = "https://github.com/vitalikr2x3/lyrics-term";
            license = licenses.mit;
            platforms = platforms.linux;
            mainProgram = "lyrics-term";
          };
        };
      });

      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.system}.default ];
          packages = [ pkgs.meson pkgs.ninja pkgs.pkg-config ];
        };
      });
    };
}
