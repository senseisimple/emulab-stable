#
# Emulab clientside src rpm for Protogeni install.
#
%define debug_package %{nil}
%define _prefix       /usr/local

Summary:	Emulab Client Side for Protogeni Install.
Name:		emulab-protogeni
Version:	0.90
Release:	1
License:	LGPL
Group:		Applications/System
Source:		emulab-client-src.tar.gz

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
URL:		http://www.emulab.net
BuildRequires:  libtool
BuildRequires:	make
Provides:	emulab-protogeni
Prefix:		/usr

%description
Emulab Client side support for Emulab as Protogeni.

%prep
%setup -n %{name}-%{version}

%build
%configure
make client

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
make DESTDIR=%{buildroot} shadow-install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_libexecdir}/*
%{_libdir}/*
%{_includedir}/pubsub/*
/etc/init.d/*
/etc/rc*.d/*
%if "%{_prefix}" == "/usr/local"
/etc/ld.so.conf.d/*
%endif

