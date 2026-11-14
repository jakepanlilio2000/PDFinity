Name:           pdfinity
Version:        1.1.0
Release:        1%{?dist}
Summary:        Professional, modular, offline-first PDF productivity suite

%global tag_version 1.1.0

License:        MPL-2.0
URL:            https://www.stacksyncsolutions.com/
Source0:        https://github.com/jakepanlilio2000/PDFinity/archive/v%{tag_version}.tar.gz

BuildRequires:  cmake >= 3.25
BuildRequires:  ninja-build
BuildRequires:  gcc-c++ >= 11
BuildRequires:  qt6-qtbase-devel >= 6.5.0
Requires:       qt6-qtbase >= 6.5.0
Requires:       qt6-qtbase-gui >= 6.5.0
Requires:       hicolor-icon-theme

%description
PDFinity is an enterprise-grade desktop PDF application engineered in modern
C++20 and Qt 6 by StackSync Solutions with Jake Ashley Panlilio as Lead Programmer.
It provides high-speed rasterization, lossless page assembly, annotation studio,
AcroForms filler, content editing, PKCS#7 digital signing, irreversible redaction,
offline OCR, format conversion, and headless batch CLI automation, operating 100%
offline with zero telemetry.

%prep
%autosetup -n PDFinity-%{tag_version}

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

%check
%ctest

%install
%cmake_install
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_datadir}/metainfo
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/scalable/apps
install -m 644 packaging/linux/com.stacksyncsolutions.pdfinity.desktop %{buildroot}%{_datadir}/applications/com.stacksyncsolutions.pdfinity.desktop
ln -sf com.stacksyncsolutions.pdfinity.desktop %{buildroot}%{_datadir}/applications/pdfinity.desktop
install -m 644 packaging/linux/com.stacksyncsolutions.pdfinity.metainfo.xml %{buildroot}%{_datadir}/metainfo/com.stacksyncsolutions.pdfinity.metainfo.xml
install -m 644 packaging/resources/pdfinity.svg %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/pdfinity.svg
install -m 644 packaging/resources/pdfinity.svg %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/com.stacksyncsolutions.pdfinity.svg

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
update-desktop-database &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi
update-desktop-database &>/dev/null || :

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%license LICENSE packaging/linux/METADATA_LICENSE
%doc README.md THIRDPARTY_NOTICES.txt THIRD_PARTY_LICENSES.md
%{_bindir}/pdfinity
%{_datadir}/applications/com.stacksyncsolutions.pdfinity.desktop
%{_datadir}/applications/pdfinity.desktop
%{_datadir}/metainfo/com.stacksyncsolutions.pdfinity.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/pdfinity.svg
%{_datadir}/icons/hicolor/scalable/apps/com.stacksyncsolutions.pdfinity.svg

%changelog
* Sat Nov 14 2026 Jake Ashley Panlilio <jakepanlilio2000@users.noreply.github.com> - 1.1.0-1
- Stable release under MPL-2.0.
