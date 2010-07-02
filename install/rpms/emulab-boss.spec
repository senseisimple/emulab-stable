%define name emulab-boss
%define version 2.0
%define elab_url http://www.emulab.net

%define release_num 1
%define release %{release_num}.emulab%{?date:.%{date}}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	A metapackage with dependencies for an Emulab BOSS server.

Group:		System Environment/Daemons
License:	AGPL
URL:		%{elab_url}
#Source0:	
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:	
Requires:	make, sudo, rsync, nfs-utils, mhash
Requires:	mysql, mysql-server, mysql-devel
Requires:	perl, perl-BSD-Resource, perl-DBD-MySQL, perl-DBI, perl-XML-Parser, perl-XML-Simple, perl-CGI-Session, perl-GDGraph, perl-HTML-Parser, perl-TimeDate, perl-RPC-XML, perl-IO-Tty, perl-MD5, perl-SNMP-Info, perl-SNMP_Session
Requires:	otcl, tcl
Requires:	wget, curl
Requires:	python, MySQL-python, m2crypto
Requires:	httpd, mod_auth_mysql, mod_ssl
Requires:	php, php-adodb
Requires:	graphviz
Requires:	fping, netpbm, vcg
Requires:	tftpd
Requires:	dhcp
Requires:	boost, metis
Requires:	swig
Requires:	bind, bind-utils
# Emulab-ish stuff
Requires:	pubsub, ulsshxmlrpcpp


%description
This is the BOSS "metapackage", which is essentially a big dependency container
of all the packages that need to be installed on an Emulab BOSS server.

%prep

%build

%install
echo "Nothing to install for emulab-boss!"

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc

%changelog

 * Mon Sep 09 2008 David Johnson <johnsond@cs.utah.edu> 
   - initial version.
