%define name ulsshxmlrpcpp
%define version 0.1.2
%define url http://www.emulab.net

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

%define debug_package %{nil}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	The ulsshxmlrpcpp library provides the classes needed to communicate with any XML-RPC server that uses SSH as a transport.

Group:		System
License:	GPL
URL:		%{url}
Source:	      	%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	make, autoconf, gcc
#Requires:	

%description
OTcl, short for MIT Object Tcl, is an extension to Tcl/Tk for
object-oriented programming. It shouldn't be confused with the IXI
Object Tcl extension by Dean Sheenan. (Sorry, but we both like the
name and have been using it for a while.)

%prep
%setup -n %{name}-%{version}

%build
%configure --prefix=/usr
make 

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
/usr/bin/*
/usr/include/*
/usr/include/ulxmlrpcpp/*
/usr/lib/*
/usr/share/ulxmlrpcpp/httpd/*


%post
echo -n "Running ldconfig... "
/sbin/ldconfig
echo "done"

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
