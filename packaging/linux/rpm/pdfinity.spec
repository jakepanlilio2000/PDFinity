Name:           pdfinity
Version:        0.1.0
Release:        1%{?dist}
Summary:        Professional, modular, offline-first PDF productivity suite

License:        MPL-2.0
URL:            https://www.stacksyncsolutions.com/
Source0:        https://github.com/jakepanlilio2000/PDFinity/archive/v%{version}-beta.tar.gz

BuildRequires:  cmake >= 3.25
BuildRequires:  ninja-build
BuildRequires:  gcc-c++ >= 11
BuildRequires:  qt6-qtbase-devel >= 6.5.0
Requires:       qt6-qtbase >= 6.5.0
Requires:       qt6-qtbase-gui >= 6.5.0

%description
PDFinity is an enterprise-grade desktop PDF application engineered in modern
C++20 and Qt 6 by StackSync Solutions with Jake Ashley Panlilio as Lead Programmer.
It provides high-speed rasterization, lossless page assembly, annotation studio,
AcroForms filler, content editing, PKCS#7 digital signing, irreversible redaction,
offline OCR, format conversion, and headless batch CLI automation, operating 100%
offline with zero telemetry.

%prep
%autosetup -n PDFinity-%{version}-beta

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

%check
%ctest

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/scalable/apps
mkdir -p %{buildroot}%{_datadir}/licenses/%{name}
mkdir -p %{buildroot}%{_datadir}/doc/%{name}

install -m 755 %{_vpath_builddir}/bin/PDFinity %{buildroot}%{_bindir}/pdfinity
install -m 644 packaging/linux/pdfinity.desktop %{buildroot}%{_datadir}/applications/pdfinity.desktop
install -m 644 packaging/resources/pdfinity.svg %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/pdfinity.svg
install -m 644 LICENSE %{buildroot}%{_datadir}/licenses/%{name}/LICENSE
install -m 644 README.md %{buildroot}%{_datadir}/doc/%{name}/README.md
install -m 644 THIRDPARTY_NOTICES.txt %{buildroot}%{_datadir}/doc/%{name}/THIRDPARTY_NOTICES.txt

%files
%license LICENSE
%doc README.md THIRDPARTY_NOTICES.txt
%{_bindir}/pdfinity
%{_datadir}/applications/pdfinity.desktop
%{_datadir}/icons/hicolor/scalable/apps/pdfinity.svg

%changelog
* Sat Nov 14 2026 Jake Ashley Panlilio <jakepanlilio2000@users.noreply.github.com> - 0.1.0-1
- Initial public beta release under MPL-2.0.
