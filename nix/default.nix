{
  pkgs ? import <nixpkgs> {},
  libsquish ? null,
  minizip ? null,
  docs ? true,
  launcher ? true,
  client ? true,
  video ? true,
  scripting ? true,
  ninja ? null,
}: let
  _libsquish =
    if libsquish != null
    then libsquish
    else pkgs.callPackage ./libsquish.nix {};
in rec {
  pname = "vcmi";
  src = ./..;
  version = "$1.18-dev"; #TODO: Update from git?
  meta = {
    description = "Open-source engine for Heroes of Might and Magic III";
    homepage = "https://vcmi.eu";
    changelog = "https://github.com/vcmi/vcmi/blob/${version}/ChangeLog.md";
    license = with pkgs.lib.licenses; [
      gpl2Plus
      cc-by-sa-40
    ];
    mainProgram = "vcmilauncher";
  };
  buildInputs = with pkgs;
  with qt6; let
    qtEnv = env "qt-custom-${qtbase.version}" [
      qtdeclarative
      qtsvg
      qttools
    ];
  in
    (
      if launcher
      then [qtEnv qtbase]
      else []
    )
    ++ [
      SDL2
      SDL2.dev
      SDL2_ttf
      SDL2_net
      SDL2_image
      SDL2_sound
      SDL2_mixer
      SDL2_gfx
      boost
      git
      _libsquish
      cmake
      pkg-config
      (
        if ninja != null
        then ninja
        else pkgs.gnumake
      )
    ]
    ++ (
      if docs
      then [doxygen]
      else []
    )
    ++ (
      if launcher
      then [qt6.wrapQtAppsHook]
      else []
    );
  configureFlags = ["--with-sdl2"];
  postFixup = with pkgs; ''
    wrapProgram $out/bin/vcmibuilder \
      --prefix PATH : "${
      lib.makeBinPath [
        innoextract
        ffmpeg
        unshield
      ]
    }"
  '';
  cmakeFlags =
    (
      if !client
      then ["-DENABLE_CLIENT=OFF"]
      else []
    )
    ++ (
      if scripting
      then ["-DENABLE_ERM=ON" "-DENABLE_LUA=ON"]
      else []
    )
    ++ (
      if !video
      then ["-DENABLE_VIDEO=OFF"]
      else []
    )
    ++ (
      if !launcher
      then ["-DENABLE_LAUNCHER=OFF"]
      else []
    );
  nativeBuildInputs = with pkgs.buildPackages;
    [
      boost
      zlib
      xz
      tbb
      vulkan-headers
      libxkbcommon
      onnxruntime
      python3
      pkg-config
      _libsquish
    ]
    ++ [
      (
        if minizip != null
        then minizip
        else pkgs.minizip
      )
      (
        if ninja != null
        then ninja
        else gnumake
      )
    ]
    ++ (
      if (client && video)
      then [ffmpeg]
      else []
    )
    ++ (
      if launcher
      then [qt6.wrapQtAppsHook]
      else []
    )
    ++ (
      if scripting
      then [luajit]
      else []
    );
}
