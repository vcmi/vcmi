Summary:			VCMI is an open-source project aiming to reimplement HMM3:WoG game engine, giving it new and extended possibilities.
Name:				vcmi
Version:			0.9.5
Release:			1%{?dist}
License:			GPLv2+
Group:				Amusements/Games

# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export -r HEAD https://svn.code.sf.net/p/vcmi/code/tags/0.95 vcmi-0.9.5-1
#  tar -cJf vcmi-0.9.5-1.tar.xz vcmi-0.9.5-1
Source:				vcmi-0.9.5-1.tar.xz

URL:				http://forum.vcmi.eu/portal.php
BuildRequires:		cmake
BuildRequires:		gcc-c++ >= 4.7.2
BuildRequires:		SDL-devel
BuildRequires:		SDL_image-devel
BuildRequires:		SDL_ttf-devel
BuildRequires:		SDL_mixer-devel >= 1.2.8
BuildRequires:		boost >= 1.51
BuildRequires:		boost-devel >= 1.51
BuildRequires:		boost-filesystem >= 1.51
BuildRequires:		boost-iostreams >= 1.51
BuildRequires:		boost-system >= 1.51
BuildRequires:		boost-thread >= 1.51
BuildRequires:		boost-program-options >= 1.51
BuildRequires:		boost-locale >= 1.51
BuildRequires:		zlib-devel
BuildRequires:		ffmpeg-devel
BuildRequires:		ffmpeg-libs
BuildRequires:		qt5-qtbase-devel

%description
VCMI is an open-source project aiming to reimplement HMM3:WoG game engine, giving it new and extended possibilities. 

%prep
%setup -q -n %{name}-%{version}-1

%build
cmake -DCMAKE_INSTALL_PREFIX=/usr ./ -DENABLE_LAUNCHER=ON -DENABLE_PCH=ON
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%files
%doc README README.linux COPYING AUTHORS ChangeLog
%{_bindir}/vcmiclient
%{_bindir}/vcmiserver
%{_bindir}/vcmibuilder
%{_bindir}/vcmilauncher
%{_libdir}/%{name}/*

%{_datadir}/%{name}/*
%{_datadir}/applications/*
%{_datadir}/icons/*

%changelog
* Sat Mar 01 2014 VCMI - 0.9.5-1
- New upstream release

* Wed Oct 02 2013 VCMI - 0.9.4-1
- New upstream release

* Sun Jun 02 2013 VCMI - 0.9.3-1
- New upstream release

* Wed Mar 06 2013 VCMI - 0.9.2-1
- New upstream release

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
