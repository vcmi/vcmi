{
  description = "VCMI development shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              python3
              qt6.wrapQtAppsHook
            ];

            buildInputs = with pkgs; [
              SDL2
              SDL2_image
              SDL2_mixer
              SDL2_ttf
              boost
              ffmpeg
              fuzzylite
              libsquish
              luajit
              minizip
              onetbb
              onnxruntime
              qt6.qtbase
              qt6.qttools
              xz
              zlib
            ];
          };
        });
    };
}
