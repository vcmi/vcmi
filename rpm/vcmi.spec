Summary:            VCMI is an open-source project aiming to reimplement HMM3:WoG game engine, giving it new and extended possibilities.
Name:               vcmi
Version:            0.89
Release:            1%{?dist}
License:            GPLv2+
Group:              Amusements/Games

# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export -r HEAD https://vcmi.svn.sourceforge.net/svnroot/vcmi/tags/0.89 vcmi-0.89-1
#  tar -cJvf vcmi-0.89-1.tar.xz vcmi-0.89-1
Source:             vcmi-0.89-1.tar.xz

URL:                http://forum.vcmi.eu/portal.php
BuildRequires:      gcc-c++ 
BuildRequires:      SDL-devel            
BuildRequires:      SDL_image-devel
BuildRequires:      SDL_ttf-devel
BuildRequires:      SDL_mixer-devel
BuildRequires:      boost
BuildRequires:      boost-devel
BuildRequires:      boost-filesystem
BuildRequires:      boost-iostreams
BuildRequires:      boost-system
BuildRequires:      boost-thread
BuildRequires:      boost-program-options
BuildRequires:      zlib-devel
BuildRequires:      ffmpeg-devel
BuildRequires:      ffmpeg-libs

%description
The purpose of VCMI project is to rewrite entire HOMM 3: WoG engine from scratch, giving it new and extended possibilities. We hope to support mods and new towns already made by fans but abandoned because of game code limitations.

VCMI is fan-made open-source project in progress. We already allow support for maps of any sizes, higher resolutions and extended engine limits. However, although working, the game is not finished. There are still many features and functionalities to add, both old and brand new.

As yet VCMI is not standalone program, it uses Wake of Gods files and graphics. You need to install WoG before running VCMI. 

%prep
%setup -q -n %{name}-%{version}-1

%build
./configure --datadir=%{_datadir} --bindir=%{_bindir} --libdir=%{_libdir}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_datadir}/%{name}/

%files
%doc README README.linux COPYING AUTHORS ChangeLog
%{_bindir}/vcmiclient
%{_bindir}/vcmiserver
%{_libdir}/%{name}/*

%dir %{_datadir}/%{name}
%{_datadir}/applications/*
%{_datadir}/icons/*

%changelog
* Sun Jun 08 2012 VCMI - 0.89-1
- Initial version
