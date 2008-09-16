%define realname pubsub
%define version 20080916
%define url http://www.emulab.net

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

%define debug_package %{nil}

%define elvincompat    %{?_with_elvin:       1} %{?!_with_elvin:       0}

%define elvinstr %{nil}
%if %{?elvincompat}
%define elvinstr -elvincompat
%endif

Name:		%{realname}%{elvinstr}
Version:	%{version}
Release:	%{release}
Summary:	The Emulab publish-subscribe event system.

Group:		Language
License:	MIT
URL:		%{url}
Source:	      	%{realname}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	make, gcc
Requires:	glibc
Provides:	pubsub

%description
pusub is a publish-subscribe event system designed for use in
the Emulab network testbed.  It is similar to Elvin, a closed-
source event system, and provides a very similar API.

%prep
%setup -n %{realname}

%build
%if %{elvincompat}
rm -f .noelvin
%else
touch .noelvin
%endif
make 

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make INSTALLPREFIX=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
/usr/local/lib/*
/usr/local/include/pubsub/*
/usr/local/libexec/*
/etc/init.d/*
/etc/rc*.d/*
/etc/ld.so.conf.d/pubsub.conf

%post
echo -n "Running ldconfig... "
/sbin/ldconfig
echo "done"

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
