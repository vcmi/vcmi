# Building VCMI for Linux

- Current baseline requirement for building is Ubuntu 20.04
- Supported C++ compilers for UNIX-like systems are GCC 9+ and Clang 13+

Older distributions and compilers might work, but they aren't tested by Github CI (Actions)

## Installing dependencies

### Prerequisites

To compile, the following packages (and their development counterparts) are needed to build:

-   CMake
-   SDL2 with devel packages: mixer, image, ttf
-   zlib and zlib-devel
-   Boost C++ libraries v1.48+: program-options, filesystem, system, thread, locale
-   Recommended, if you want to build launcher or map editor: Qt 5, widget and network modules
-   Recommended, FFmpeg libraries, if you want to watch in-game videos: libavformat and libswscale. Their name could be libavformat-devel and libswscale-devel, or ffmpeg-libs-devel or similar names. 
-   Optional:
    - if you want to build scripting modules: LuaJIT
    - to speed up recompilation: Ccache

### On Debian-based systems (e.g. Ubuntu)

For Ubuntu and Debian you need to install this list of packages:

`sudo apt-get install cmake g++ clang libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev zlib1g-dev libavformat-dev libswscale-dev libboost-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-locale-dev libboost-iostreams-dev qtbase5-dev libtbb-dev libluajit-5.1-dev liblzma-dev libsqlite3-dev qttools5-dev ninja-build ccache`

Alternatively if you have VCMI installed from repository or PPA you can use:

`sudo apt-get build-dep vcmi`

### On RPM-based distributions (e.g. Fedora)

`sudo yum install cmake gcc-c++ SDL2-devel SDL2_image-devel SDL2_ttf-devel SDL2_mixer-devel boost boost-devel boost-filesystem boost-system boost-thread boost-program-options boost-locale boost-iostreams zlib-devel ffmpeg-devel ffmpeg-libs qt5-qtbase-devel tbb-devel luajit-devel liblzma-devel libsqlite3-devel fuzzylite-devel ccache`

NOTE: `fuzzylite-devel` package is no longer available in recent version of Fedora, for example Fedora 38. It's not a blocker because VCMI bundles fuzzylite lib in its source code.

### On Arch-based distributions

On Arch-based distributions, there is a development package available for VCMI on the AUR.

It can be found at https://aur.archlinux.org/packages/vcmi-git/

Information about building packages from the Arch User Repository (AUR) can be found at the Arch wiki.

### On NixOS or Nix

On NixOS or any system with nix available, [it is recommended](https://nixos.wiki/wiki/C) to use nix-shell. Create a shell.nix file with the following content:

```nix
with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "build";
  nativeBuildInputs = [ cmake ];
  buildInputs = [
    cmake clang clang-tools llvm ccache ninja
    boost zlib minizip xz
    SDL2 SDL2_ttf SDL2_net SDL2_image SDL2_sound SDL2_mixer SDL2_gfx
    ffmpeg tbb vulkan-headers libxkbcommon
    qt6.full luajit
  ];
}
```

And put it into build directory. Then run `nix-shell` before running any build commands.

## Getting the sources

We recommend the following directory structure:

```
.
├── vcmi -> contains sources and is under git control
└── build -> contains build output, makefiles, object files,...
```

You can get the latest source code with:

`git clone -b develop --recursive https://github.com/vcmi/vcmi.git`

## Compilation

### Configuring Makefiles

```sh
mkdir build
cd build
cmake -S ../vcmi
```

> [!NOTE]
> The `../vcmi` is not a typo, it will place Makefiles into the build dir as the build dir is your working dir when calling CMake.

See [CMake](CMake.md) for a list of options

### Trigger build

```
cmake --build . -j8
```

(-j8 = compile with 8 threads, you can specify any value. )

This will generate `vcmiclient`, `vcmiserver`, `vcmilauncher` as well as .so libraries in the `build/bin/` directory.

## Packaging

### RPM package

The first step is to prepare a RPM build environment. On Fedora systems you can follow this guide: http://fedoraproject.org/wiki/How_to_create_an_RPM_package#SPEC_file_overview

0. Enable RPMFusion free repo to access to ffmpeg libs:

```sh
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
```

> [!NOTE]
> The stock ffmpeg from Fedora repo is no good as it lacks a lots of codecs

1. Perform a git clone from a tagged branch for the right Fedora version from https://github.com/rpmfusion/vcmi; for example for Fedora 38: <pre>git clone -b f38 --single-branch https://github.com/rpmfusion/vcmi.git</pre>

2. Copy all files to ~/rpmbuild/SPECS with command: <pre>cp vcmi/*  ~/rpmbuild/SPECS</pre>

3. Fetch all sources by using spectool:

```sh
sudo dnf install rpmdevtools
spectool -g -R ~/rpmbuild/SPECS/vcmi.spec
```

4. Fetch all dependencies required to build the RPM:

```sh
sudo dnf install dnf-plugins-core
sudo dnf builddep ~/rpmbuild/SPECS/vcmi.spec
```

4. Go to ~/rpmbuild/SPECS and open terminal in this folder and type: 
```sh
rpmbuild -ba ~/rpmbuild/SPECS/vcmi.spec
```

5. Generated RPM is in folder ~/rpmbuild/RPMS

If you want to package the generated RPM above for different processor architectures and operating systems you can use the tool mock.
Moreover, it is necessary to install mock-rpmfusion_free due to the packages ffmpeg-devel and ffmpeg-libs which aren't available in the standard RPM repositories(at least for Fedora). Go to ~/rpmbuild/SRPMS in terminal and type: 

```sh
mock -r fedora-38-aarch64-rpmfusion_free path_to_source_RPM
mock -r fedora-38-x86_64-rpmfusion_free path_to_source_RPM
```

For other distributions that uses RPM, chances are there might be a spec file for VCMI. If there isn't, you can adapt the RPMFusion's file

Available root environments and their names are listed in /etc/mock.

### Debian/Ubuntu

1. Install debhelper and devscripts packages

2. Run dpkg-buildpackage command from vcmi source directory

```sh
sudo apt-get install debhelper devscripts
cd /path/to/source
dpkg-buildpackage
```

To generate packages for different architectures see "-a" flag of dpkg-buildpackage command
