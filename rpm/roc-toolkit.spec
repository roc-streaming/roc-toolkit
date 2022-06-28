%undefine       _disable_source_fetch
%define         build_timestamp %(date +"%Y%m%d")
%define         pkgname roc
%define         pkgname_git %{pkgname}-toolkit
%define         major 0.1
%define         minor 5
%define         branch develop
%define         arch %(uname -p)
Name:           libroc
Version:        %{major}.%{minor}~git_%{branch}
Release:        %{build_timestamp}
Group:          System/Sound Daemons
Summary:        Roc real-time audio streaming over the network
Source:         https://github.com/roc-streaming/%{pkgname_git}/archive/%{branch}.zip
URL:            https://roc-streaming.org
License:        MPL-2.0
Provides:       %{name} = %{version}
Obsoletes:      %{name} < %{version}
BuildRequires:  gcc-c++
BuildRequires:  meson
BuildRequires:  ragel
BuildRequires:  libopenfec-devel

%description
Roc is a toolkit for real-time audio streaming over the network.

Basically, Roc is a network transport, highly specialized for the real-time streaming use case. The user writes the stream to the one end and reads it from another end, and Roc deals with all the complexity of the task of delivering data in time and with no loss. Encoding, decoding, adjusting rates, restoring losses – all these are performed transparently under the hood.

%package -n %{name}-devel
Summary:        Roc real-time audio streaming over the network
Group:          Development/Libraries/C and C++
BuildArch:      noarch
Provides:       %{name}-devel = %{version}
Requires:       %{name} = %{version}
Obsoletes:      %{name}-devel < %{version}

%description -n %{name}-devel
Roc is a toolkit for real-time audio streaming over the network.

Basically, Roc is a network transport, highly specialized for the real-time streaming use case. The user writes the stream to the one end and reads it from another end, and Roc deals with all the complexity of the task of delivering data in time and with no loss. Encoding, decoding, adjusting rates, restoring losses – all these are performed transparently under the hood.

%prep
%setup -n %{pkgname_git}-%{branch}

%build
scons --compiler=gcc

%install
mkdir -p %{buildroot}/%{_bindir} \
        %{buildroot}/%{_libdir} \
        %{buildroot}/%{_includedir}/%{name}
for f in bin/%{arch}*/%{pkgname}*; do
  install ${f} %{buildroot}/%{_bindir}/
done
install bin/%{arch}*/%{name}.so.%{major} %{buildroot}/%{_libdir}/
ln -s %{_libdir}/%{name}.so.%{major} %{buildroot}/%{_libdir}/%{name}.so.0 
ln -s %{_libdir}/%{name}.so.%{major} %{buildroot}/%{_libdir}/%{name}.so
cp -pr src/public_api/include/%{pkgname} %{buildroot}/%{_includedir}/

%files -n %{name}
%defattr(755,root,root)
%attr(644,root,root) %license LICENSE
%{_bindir}/*
%{_libdir}/%{name}*

%files -n %{name}-devel
%defattr(-,root,root)
%license LICENSE
%{_includedir}/%{pkgname}

%changelog
* Tue Jun 28 2022 twojstaryzdomu - 0.1.5_git_develop
- Initial release
