Summary:            VCMI is an open-source project aiming to reimplement the entire H3:WoG game engine, giving it new and extended possibilities
Name:               vcmi
Version:            0.9.1
Release:            2%{?dist}
License:            GPLv2+

# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export -r HEAD https://vcmi.svn.sourceforge.net/svnroot/vcmi/tags/0.91 vcmi-0.9.1-2
#  tar -cJf vcmi-0.9.1-2.tar.xz vcmi-0.9.1-2
Source:             vcmi-0.9.1-2.tar.xz

URL:                http://forum.vcmi.eu/portal.php
BuildRequires:      cmake
BuildRequires:      gcc-c++ >= 4.7.2
BuildRequires:      SDL-devel
BuildRequires:      SDL_image-devel
BuildRequires:      SDL_ttf-devel
BuildRequires:      SDL_mixer-devel >= 1.2.8
BuildRequires:      boost >= 1.44
BuildRequires:      boost-devel >= 1.44
BuildRequires:      boost-filesystem >= 1.44
BuildRequires:      boost-iostreams >= 1.44
BuildRequires:      boost-system >= 1.44
BuildRequires:      boost-thread >= 1.44
BuildRequires:      boost-program-options >= 1.44
BuildRequires:      zlib-devel
BuildRequires:      ffmpeg-devel
BuildRequires:      ffmpeg-libs

%description
The purpose of VCMI project is to rewrite the entire H3:WoG engine from scratch,
giving it new and extended possibilities.

VCMI is fan-made open-source project in progress. We already allow support for
maps of any sizes, higher resolutions, extended engine limits, modding support
for custom towns, artifacts, heroes, etc...

As yet VCMI is not a standalone program, it uses Wake of Gods files and
graphics. You need to install WoG before running VCMI.

%prep
%setup -q -n %{name}-%{version}-2

%build
cmake -DCMAKE_INSTALL_PREFIX=/usr ./
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

%files
%doc README COPYING AUTHORS ChangeLog
%{_bindir}/vcmiclient
%{_bindir}/vcmiserver
%{_bindir}/vcmibuilder
%{_libdir}/%{name}/*

%{_datadir}/%{name}/*
%{_datadir}/applications/*
%{_datadir}/icons/*

%changelog
* Fri Feb 01 2013 VCMI - 0.9.1-2
- New upstream release

* Wed Jan 30 2013 VCMI - 0.9.1-1
- Development release

* Sun Oct 21 2012 VCMI - 0.9-2
- Second release of 0.9, Fixed battles crash

* Sat Oct 06 2012 VCMI - 0.9-1
- New upstream release

* Sun Jun 08 2012 VCMI - 0.89-1
- Initial version
