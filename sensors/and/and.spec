Name:      and
Version:   1.2.0
Release:   1
Summary:   Auto nice daemon
Vendor:    Patrick Schemitz <schemitz@users.sourceforge.net>
Copyright: GPL
Group:     Daemons
Buildroot: /var/tmp/%{name}-buildroot
Source:    http://and.sourceforge.net/%{name}-%{version}.tar.gz
URL:       http://and.sourceforge.net
Prefix:    %{_prefix}
ExclusiveOS: linux

%description
The auto nice daemon renices and even kills jobs according to their CPU time,
owner, and command name. This is especially useful on production machines with
lots of concurrent CPU-intensive jobs and users that tend to forget to
nice their jobs.

%prep            
rm -rf %{buildroot}
%setup -q
make PREFIX=%{_prefix} \
     INSTALL_ETC=/etc \
     INSTALL_INITD= \
     INSTALL_SBIN=%{_sbindir} \
     INSTALL_MAN=%{_mandir}

%install
mkdir -p %{buildroot}/etc
mkdir -p %{buildroot}$initddir
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_mandir}/man8
mkdir -p %{buildroot}%{_mandir}/man5
make PREFIX=%{buildroot}%{_prefix} \
     INSTALL_ETC=%{buildroot}/etc \
     INSTALL_INITD= \
     INSTALL_SBIN=%{buildroot}%{_sbindir} \
     INSTALL_MAN=%{buildroot}%{_mandir} install 

%clean
rm -rf %{buildroot}

%pre

%post
initddir=`%{_sbindir}/and-find-init.d`
ln -sf %{_sbindir}/and.init $initddir/and
/sbin/chkconfig --add and

%preun
%{_sbindir}/and.init stop > /dev/null 2>&1 
/sbin/chkconfig --del and
initddir=`%{_sbindir}/and-find-init.d`
rm -f $initddir/and

%postun

%files
%defattr(-,root,root)
%doc README LICENSE CHANGELOG
%config(noreplace)  /etc/and.*
%{_sbindir}/*
%{_mandir}/*/*
