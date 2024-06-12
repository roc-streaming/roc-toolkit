%undefine _disable_source_fetch

Name:       roc-toolkit
Version:    0.4.0
Release:    1%{?dist}
Summary:    Real-time audio streaming over the network
License:    MPL-2.0 AND CECILL-C AND BSD
URL:        https://github.com/roc-streaming/roc-toolkit
Source:    %{URL}/archive/v%{version}/%{name}_%{version}.tar.gz
Provides:  %{name} = %{version}
Obsoletes: %{name} < %{version}

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: pkgconf-pkg-config
BuildRequires: python3-devel
BuildRequires: python3-scons
BuildRequires: ragel-devel
BuildRequires: gengetopt
BuildRequires: cmake
BuildRequires: make
BuildRequires: which
BuildRequires: libuv-devel
BuildRequires: libunwind-devel
BuildRequires: speexdsp-devel
BuildRequires: sox-devel
BuildRequires: openssl-devel
BuildRequires: pulseaudio-libs-devel

%description
Roc is a toolkit for real-time audio streaming over the network.

%package -n %{name}-devel
Summary:        Real-time audio streaming over the network
Group:          Development/Libraries/C and C++
Provides:       %{name}-devel = %{version}
Requires:       %{name} = %{version}
Obsoletes:      %{name}-devel < %{version}

%description -n %{name}-devel
Development package for roc-toolkit

%package -n %{name}-utils
Summary:        Real-time audio streaming over the network
Provides:       %{name}-utils = %{version}
Requires:       %{name} = %{version}
Obsoletes:      %{name}-utils < %{version}

%description -n %{name}-utils
Command-line tools package for roc-toolkit

%prep
%setup -n %{name}-%{version}

%build
scons --build-3rdparty=openfec \
  --prefix=/usr \
  --libdir=%{_libdir} \
  %{?_smp_mflags} \
  CFLAGS="%{build_cflags}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}"

%install
scons --build-3rdparty=openfec \
  --prefix=/usr \
  --libdir=%{_libdir} \
  %{?_smp_mflags} \
  CFLAGS="%{build_cflags}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}" \
  DESTDIR=%{buildroot} \
  install

%files
%license LICENSE
%{_libdir}/libroc.so.*

%files devel
%license LICENSE
%{_includedir}/roc
%{_libdir}/pkgconfig/roc.pc
%{_libdir}/libroc.so

%files utils
%license LICENSE
%{_bindir}/roc-*
%{_mandir}/man1/*.1.gz
