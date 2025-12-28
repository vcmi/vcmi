{
  pkgs,
  src,
  libsquish ? (pkgs.callPackage ./libsquish.nix {}),
  minizip ? pkgs.minizip,
  generator ? pkgs.gnumake,
  docs ? true,
  launcher ? true,
  client ? true,
  video ? true,
  scripting ? true,
}: let
  rev = builtins.substring 0 7 (src.rev or src.dirtyRev or "HEAD");
in {
  pname = "vcmi";
  inherit src;
  version = "${rev}-dev"; #TODO: Update from git?
  meta = {
    description = "Open-source engine for Heroes of Might and Magic III";
    longDescription = ''
      VCMI is an open-source engine for Heroes of Might and Magic III, offering
      new and extended possibilities. To use VCMI, you need to own the original
      data files.
    '';
    homepage = "https://vcmi.eu";
    changelog = "https://github.com/vcmi/vcmi/blob/${rev}/ChangeLog.md";
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
      libsquish
      cmake
      pkg-config
      generator
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
  cmakeFlags = with pkgs;
    [
      (lib.cmakeFeature "CMAKE_INSTALL_RPATH" "$out/lib/vcmi")
      (lib.cmakeFeature "CMAKE_INSTALL_BINDIR" "bin")
      (lib.cmakeFeature "CMAKE_INSTALL_LIBDIR" "lib")
      (lib.cmakeFeature "CMAKE_INSTALL_DATAROOTDIR" "share")
    ]
    ++ (
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
      libsquish
      minizip
    ]
    ++ [
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
