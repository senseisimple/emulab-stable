%define name metis
%define version 4.0.1
%define src_version 4.0
%define url http://glaros.dtc.umn.edu/gkhome/views/metis

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

%define debug_package %{nil}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	METIS is a set of serial programs for partitioning graphs, partitioning finite element meshes, and producing fill reducing orderings for sparse matrices.

Group:		Programming
License:	MIT
URL:		%{url}
Source:	      	%{name}-%{src_version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:	make, gcc
#Requires:	

Provides:	metis

%description
METIS is a set of serial programs for partitioning graphs, partitioning finite
element meshes, and producing fill reducing orderings for sparse matrices. The
algorithms implemented in METIS are based on the multilevel
recursive-bisection, multilevel k-way, and multi-constraint partitioning
schemes developed in our lab.

%prep
%setup -n %{name}-%{src_version}

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p  $RPM_BUILD_ROOT
mkdir -p  $RPM_BUILD_ROOT/usr/bin
mkdir -p  $RPM_BUILD_ROOT/usr/lib
mkdir -p  $RPM_BUILD_ROOT/usr/include

install graphchk $RPM_BUILD_ROOT/usr/bin
install kmetis $RPM_BUILD_ROOT/usr/bin
install mesh2dual $RPM_BUILD_ROOT/usr/bin
install mesh2nodal $RPM_BUILD_ROOT/usr/bin
install oemetis $RPM_BUILD_ROOT/usr/bin
install onmetis $RPM_BUILD_ROOT/usr/bin
install partdmesh $RPM_BUILD_ROOT/usr/bin
install partnmesh $RPM_BUILD_ROOT/usr/bin
install pmetis $RPM_BUILD_ROOT/usr/bin
install libmetis.a $RPM_BUILD_ROOT/usr/lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc Doc/manual.ps INSTALL CHANGES FILES VERSION
/usr/bin/*
/usr/lib/*

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
