{ 
  pkgs ? import <nixpkgs> {},
  libsquish ? null,
}: let _libsquish = if libsquish != null then libsquish else pkgs.callPackage ./libsquish.nix {  }; in pkgs.mkShell {
    buildInputs =  with pkgs; with qt6; let
      qtEnv = env "qt-custom-${qtbase.version}" [
        qtdeclarative
        qtsvg
        qttools
      ];
    in [ qtEnv qtbase SDL2 SDL2.dev _libsquish SDL2_ttf SDL2_net SDL2_image SDL2_sound SDL2_mixer SDL2_gfx cmake clang clang-tools llvm boost doxygen pkg-config ];
    configureFlags = ["--with-sdl2"];
    nativeBuildInputs = with pkgs.buildPackages; [
    ccache samurai
    boost zlib minizip xz
    ffmpeg tbb vulkan-headers libxkbcommon
    qt6.wrapQtAppsHook luajit onnxruntime fuzzylite python3 pkg-config _libsquish
  ];
}