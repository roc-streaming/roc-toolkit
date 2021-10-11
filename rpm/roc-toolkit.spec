%define name roc-toolkit
%define reponame roc-toolkit
%define version 0.1.5
%define build_timestamp %{lua: print(os.date("%Y%m%d"))}

Summary: Roc Toolkit provides real-time audio streaming
Name: %{name}
Version: %{version}
Release: %{build_timestamp}
Source0: https://github.com/roc-streaming/roc-toolkit/archive/refs/heads/master.zip#/%{name}-%{version}-%{release}.zip
License: MPL-2.0
BuildRoot: %{_tmppath}/%{name}-buildroot
BuildRequires: gcc-c++
BuildRequires: scons
BuildRequires: ragel
BuildRequires: gengetopt
BuildRequires: libuv-devel
BuildRequires: libunwind-devel
BuildRequires: pulseaudio-libs-devel
BuildRequires: sox-devel
BuildRequires: cmake
BuildRequires: git
Requires: libuv
Requires: libunwind
Requires: pulseaudio-libs
Requires: sox

Url: https://roc-streaming.org/

%description

Roc is a toolkit for real-time audio streaming over the network.

Basically, Roc is a network transport, highly specialized for the real-time
streaming use case. The user writes the stream to the one end and reads it from
another end, and Roc deals with all the complexity of the task of delivering
data in time and with no loss. Encoding, decoding, adjusting rates, restoring
losses - all these are performed transparently under the hood.

The project is conceived as a swiss army knife for real-time streaming. It is
designed to support a variety of network protocols, encodings, FEC schemes, and
related features. The user can build custom configurations dedicated for
specific use cases and choose an appropriate compromise between the quality,
robustness, bandwidth, and compatibility issues.

%prep
%autosetup -n %{reponame}-master

%install
rm -rf ${buildroot}
scons -Q --build-3rdparty=openfec,cpputest
scons -Q --build-3rdparty=openfec,cpputest --prefix=%{buildroot}/usr install

%clean
rm -rf %{buildroot}

%files
/usr/include/roc/*
/usr/lib/*
/usr/bin/*

%changelog

* Mon Oct 11 2021 Michael Kuryshev <me@mk9.name>
- Initial spec file
