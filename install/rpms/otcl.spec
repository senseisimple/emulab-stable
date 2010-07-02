%define name otcl
%define version 1.13
%define url http://otcl-tclcl.sourceforge.net/otcl/

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

%define debug_package %{nil}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	OTcl, short for MIT Object Tcl, is an extension to Tcl/Tk for object-oriented programming.

Group:		Language
License:	MIT
URL:		%{url}
Source:	      	%{name}-src-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	make, autoconf, gcc, tcl, tcl-devel, tk, tk-devel, libXt-devel
Requires:	tcl, tk, libX11, glibc, libxcb, libXau, libXdmcp

patch1:		otcl-x11-headers.patch

%description
OTcl, short for MIT Object Tcl, is an extension to Tcl/Tk for
object-oriented programming. It shouldn't be confused with the IXI
Object Tcl extension by Dean Sheenan. (Sorry, but we both like the
name and have been using it for a while.)

%prep
%setup -n %{name}-%{version}
#pwd
#find .
#cd %{name}-%{version}
%patch1 -p0
#cd ..

%build
%configure --prefix=/usr
make 
make libotcl.so

%install
#
# this bit of ugliness is essentially pulled from the Makefile,
# since I don't want to patch it with a DESTDIR and crap
#
rm -rf $RPM_BUILD_ROOT
mkdir -p  $RPM_BUILD_ROOT
mkdir -p  $RPM_BUILD_ROOT/usr/bin
mkdir -p  $RPM_BUILD_ROOT/usr/lib
mkdir -p  $RPM_BUILD_ROOT/usr/include

install owish $RPM_BUILD_ROOT/usr/bin
install otclsh $RPM_BUILD_ROOT/usr/bin
install libotcl.a $RPM_BUILD_ROOT/usr/lib
ranlib $RPM_BUILD_ROOT/usr/lib/libotcl.a
install libotcl.so $RPM_BUILD_ROOT/usr/lib
install -m 644 otcl.h $RPM_BUILD_ROOT/usr/include

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
/usr/bin/owish
/usr/bin/otclsh
/usr/lib/libotcl.a
/usr/lib/libotcl.so
/usr/include/otcl.h

%post
echo -n "Running ldconfig... "
/sbin/ldconfig
echo "done"

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
