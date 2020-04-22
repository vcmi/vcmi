Summary:            Rewrite of the Heroes of Might and Magic 3 engine
Name:               vcmi
Version:            0.99
Release:            2%{?dist}
License:            GPLv2+
Source:             https://github.com/vcmi/%{name}/archive/%{version}/%{name}-%{version}.tar.gz
URL:                http://forum.vcmi.eu/portal.php
BuildRequires:      cmake
BuildRequires:      pkgconfig(sdl2)
BuildRequires:      pkgconfig(SDL2_image)
BuildRequires:      pkgconfig(SDL2_ttf)
BuildRequires:      pkgconfig(SDL2_mixer)
BuildRequires:      boost-devel >= 1.51
BuildRequires:      pkgconfig(zlib)
BuildRequires:      pkgconfig(libavcodec)
BuildRequires:      pkgconfig(libavformat)
BuildRequires:      pkgconfig(libavdevice)
BuildRequires:      pkgconfig(libavutil)
BuildRequires:      pkgconfig(libswscale)
BuildRequires:      pkgconfig(libpostproc)
BuildRequires:      pkgconfig(minizip)
BuildRequires:      pkgconfig(Qt5Core)
BuildRequires:      pkgconfig(Qt5Gui)
BuildRequires:      pkgconfig(Qt5Widgets)
BuildRequires:      pkgconfig(Qt5Network)
BuildRequires:      desktop-file-utils

%description
The purpose of VCMI project is to rewrite entire HOMM 3: WoG engine from
scratch, giving it new and extended possibilities. It will help to support
mods and new towns already made by fans but abandoned because of game code
limitations.

In its current state it already supports maps of any sizes, higher
resolutions and extended engine limits.

%prep
%autosetup -p1

%build
%cmake -DENABLE_TEST=0 -DCMAKE_SKIP_RPATH=OFF -UCMAKE_INSTALL_LIBDIR
make %{?_smp_mflags}

%install
%if 0%{?suse_version}
%cmake_install
%else
%make_install
%endif

# drop fuzzylite artifacts
rm -fr %{buildroot}%{_includedir}
rm -f %{buildroot}%{_libdir}/*.a

%check
# Menu file is being installed when make install
# so it need only to check this allready installed file
desktop-file-validate %{buildroot}%{_datadir}/applications/vcmiclient.desktop
desktop-file-validate %{buildroot}%{_datadir}/applications/vcmilauncher.desktop

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &> /dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi
/usr/bin/update-desktop-database &> /dev/null || :

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%doc README.md README.linux license.txt AUTHORS ChangeLog
%{_bindir}/vcmiclient
%{_bindir}/vcmiserver
%{_bindir}/vcmibuilder
%{_bindir}/vcmilauncher
%{_libdir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/*/apps/*
%if 0%{?suse_version}
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/48x48
%dir %{_datadir}/icons/hicolor/48x48/apps
%dir %{_datadir}/icons/hicolor/64x64
%dir %{_datadir}/icons/hicolor/64x64/apps
%dir %{_datadir}/icons/hicolor/256x256
%dir %{_datadir}/icons/hicolor/256x256/apps
%endif

%changelog
* Fri Feb 17 2017 - 0.99-2
- Common spec for openSUSE and Fedora

* Tue Nov 01 2016 VCMI - 0.99-1
- New upstream release

* Wed Apr 01 2015 VCMI - 0.98-1
- New upstream release

* Sun Nov 02 2014 VCMI - 0.97-1
- New upstream release

* Tue Jul 01 2014 VCMI - 0.96-1
- New upstream release

* Sat Mar 01 2014 VCMI - 0.95-1
- New upstream release

* Wed Oct 02 2013 VCMI - 0.94-1
- New upstream release

* Sun Jun 02 2013 VCMI - 0.93-1
- New upstream release

* Wed Mar 06 2013 VCMI - 0.92-1
- New upstream release

* Fri Feb 01 2013 VCMI - 0.91-2
- New upstream release

* Wed Jan 30 2013 VCMI - 0.91-1
- Development release

* Sun Oct 21 2012 VCMI - 0.90-2
- Second release of 0.90, Fixed battles crash

* Sat Oct 06 2012 VCMI - 0.90-1
- New upstream release

* Fri Jun 08 2012 VCMI - 0.89-1
- Initial version
