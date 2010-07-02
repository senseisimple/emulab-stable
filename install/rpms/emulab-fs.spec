%define name emulab-fs
%define version 2.0
%define elab_url http://www.emulab.net

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	A metapackage with dependencies for an Emulab FS server.

Group:		System Environment/Daemons
License:	AGPL
URL:		%{elab_url}
#Source0:	
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:	
Requires:	make, sudo, rsync, samba, nfs-utils

%description
This is the "metapackage", which is essentially a big dependency container
of all the packages that need to be installed on an Emulab FS server.

%prep

%build

%install
echo "Nothing to install for emulab-fs!"

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
