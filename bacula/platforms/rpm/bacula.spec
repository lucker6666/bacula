# Bacula RPM spec file
#
# Copyright (C) 2000-2010 Free Software Foundation Europe e.V.

# Platform Build Configuration

# basic defines for every build
%define _release           1
%define _version           5.0.3
%define _packager D. Scott Barninger <barninger@fairfieldcomputers.com>
%define depkgs_version 18Dec09

# postgresql defines - defaults for most platforms
# pass changes if required
# versions less than 8 are no longer supported
%define postgres_version 8
%{?postgresql_version:%define postgres_version %{postgresql_version}}
%define postgres_package postgresql
%{?postgresql_package:%define postgres_package %{postgresql_package}}
%define postgres_server_package postgresql-server
%{?postgresql_server_package:%define postgres_server_package %{postgresql_server_package}}
%define postgres_devel_package postgresql-devel
%{?postgresql_devel_package:%define postgres_devel_package %{postgresql_devel_package}}

%define single_dir 0
%{?single_dir_install:%define single_dir 1}

# Installation Directory locations
%if %{single_dir}
%define _prefix        /opt/bacula
%define _sbindir       /opt/bacula/bin
%define _bindir        /opt/bacula/bin
%define _subsysdir     /opt/bacula/working
%define sqlite_bindir  /opt/bacula/sqlite
%define _mandir        /usr/share/man
%define sysconf_dir    /opt/bacula/etc
%define script_dir     /opt/bacula/scripts
%define working_dir    /opt/bacula/working
%define pid_dir        /opt/bacula/working
%define plugin_dir     /opt/bacula/plugins
%define lib_dir        /opt/bacula/lib
%else
%define _prefix        /usr
%define _sbindir       %_prefix/sbin
%define _bindir        %_prefix/bin
%define _subsysdir     /var/lock/subsys
%define sqlite_bindir %_libdir/bacula/sqlite
%define _mandir        %_prefix/share/man
%define sysconf_dir    /etc/bacula
%define script_dir     %_libdir/bacula
%define working_dir    /var/lib/bacula
%define pid_dir        /var/run
%define plugin_dir     %_libdir/bacula/plugins
%define lib_dir        %_libdir/bacula/lib
%endif

# Daemon user:group Don't change them unless you know what you are doing
%define director_daemon_user    bacula
%define storage_daemon_user     bacula
%define file_daemon_user        root
%define daemon_group            bacula
# group that has write access to tape devices, usually disk on Linux
%define storage_daemon_group    disk

%define depkgs ../depkgs

# probems with mandriva build: 
# nothing provides libbonobo2_0-devel, nothing provides libbonoboui2_0-devel

# set Macros by opensuse_bs, see http://en.opensuse.org/Build_Service/cross_distribution_package_how_to
#openSUSE 11.1          %if 0%{?suse_version} == 1110  
#openSUSE 11.0          %if 0%{?suse_version} == 1100   
#openSUSE 10.3          %if 0%{?suse_version} == 1030   
#openSUSE 10.2          %if 0%{?suse_version} == 1020   
#SUSE Linux 10.1        %if 0%{?suse_version} == 1010   
#SUSE Linux 10.0        %if 0%{?suse_version} == 1000   
#SUSE Linux 9.3         %if 0%{?suse_version} == 930    
#SLES 9                 %if 0%{?sles_version} == 9      also set: %if 0%{?suse_version} == 910
#SLE 10                 %if 0%{?sles_version} == 10     also set: %if 0%{?suse_version} == 1010
#SLE 11                 %if 0%{?sles_version} == 11     also set: %if 0%{?suse_version} == 1110
#CentOS 5               %if 0%{?centos_version} == 501  
#RHEL 4                 %if 0%{?rhel_version} == 406    
#RHEL 5                 %if 0%{?rhel_version} == 501    
#Fedora 6 with Extras   %if 0%{?fedora_version} == 6    
#Fedora 7 with Extras   %if 0%{?fedora_version} == 7    
#Fedora 8 with Extras   %if 0%{?fedora_version} == 8    
#Fedora 9 with Extras   %if 0%{?fedora_version} == 9    
#Fedora 10 with Extras  %if 0%{?fedora_version} == 10   
#Mandriva 2006          %if 0%{?mandriva_version} == 2006       
#Mandriva 2007          %if 0%{?mandriva_version} == 2007       
#Mandriva 2008          %if 0%{?mandriva_version} == 2008       


%if 0%{?opensuse_bs}
# am I running in opensuse build service?
# TODO: seems to make problems

# Build Service: Determine Distribution

%ifarch x86_64
 %define build_x86_64 1
%endif


%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires: GConf2-devel
BuildRequires: freetype-devel
BuildRequires: libtermcap-devel
BuildRequires: shadow-utils
%endif


%if 0%{?mandriva_version} == 2007
%define build_mdv 1
%define _dist "Mandriva 2007"
%endif

%if 0%{?fedora_version} == 8
%define build_fc8 1
%define _dist "Fedora Core 8"
BuildRequires: redhat-release
%endif

%if 0%{?fedora_version} == 9
%define build_fc9 1
%define _dist "Fedora Core 9"
BuildRequires: redhat-release
%endif


%if 0%{?fedora_version} == 10
%define build_fc9 1
%define _dist "Fedora Core 10"
BuildRequires: PolicyKit-gnome
BuildRequires: fedora-release
%endif

%if 0%{?fedora_version} == 11
%define build_fc9 1
%define _dist "Fedora Core 11"
BuildRequires: PolicyKit-gnome
BuildRequires: fedora-release
%endif

%if 0%{?rhel_version} == 501
%define build_rhel5 1
%define _dist "Red Hat Enterprise Linux Server release 5"
BuildRequires: redhat-release
%endif

%if 0%{?rhel_version} == 406
%define build_rhel4 1
%define _dist "Red Hat Enterprise Linux Server release 4"
BuildRequires: redhat-release
%endif


%if 0%{?centos_version} == 501
%define build_centos5 1
%define _dist "CentOS 5"
BuildRequires: redhat-release
%endif


%if 0%{?suse_version} == 1020
%define build_su102 1
%define _dist "OpenSUSE 10.2"
BuildRequires: suse-release
%endif


%if 0%{?suse_version} == 1030
%define build_su103 1
%define _dist "OpenSUSE 10.3"
BuildRequires: suse-release
%endif

%if 0%{?suse_version} == 1100
%define build_su110 1
%define _dist "SUSE 11"
BuildRequires: suse-release
%endif


%if 0%{?suse_version} == 1110
%define build_su111 1
%define _dist "SUSE 11"
%endif

%if 0%{?suse_version} == 1120
%define build_su112 1
%define _dist "SUSE 11"
%endif


%if 0%{?sles_version} == 9
%define build_su9 1
%define _dist "SLES 9"
%endif


%if 0%{?sles_version} == 10
%define build_su10 1
%define _dist "SLE 10"
%endif


%if 0%{?sles_version} == 11
%define build_su111 1
%define _dist "SLES 11"
%endif

%endif 
# opensuse-bs?


# any patches for this release
# be sure to check the setup section for patch macros

#--------------------------------------------------------------------------
# it should not be necessary to change anything below here for a release
# except for patch macros in the setup section
#--------------------------------------------------------------------------

%{?contrib_packager:%define _packager %{contrib_packager}}

Summary: Bacula - The Network Backup Solution
Name: bacula
Version: %{_version}
Release: %{_release}
Group: System Environment/Daemons
License: AGPLv3
BuildRoot: %{_tmppath}/%{name}-root
URL: http://www.bacula.org/
Vendor: The Bacula Team
Packager: %{_packager}
Prefix: %{_prefix}

Source0: http://www.prdownloads.sourceforge.net/bacula/%{name}-%{version}.tar.gz
# opensuse build service changes the release itself
# what happens if the release is not 1? DSB
%if 0%{?opensuse_bs}
Source1: Release_Notes-%{version}-1.tar.gz
%else
Source1: Release_Notes-%{version}-%{release}.tar.gz
%endif
Source2: http://www.prdownloads.sourceforge.net/bacula/depkgs-%{depkgs_version}.tar.gz

# define the basic package description
%define blurb Bacula - The Leading Open Source Backup Solution.
%define blurb2 Bacula is a set of computer programs that permit you (or the system
%define blurb3 administrator) to manage backup, recovery, and verification of computer
%define blurb4 data across a network of computers of different kinds. In technical terms,
%define blurb5 it is a network client/server based backup program. Bacula is relatively
%define blurb6 easy to use and efficient, while offering many advanced storage management
%define blurb7 features that make it easy to find and recover lost or damaged files.
%define blurb8 Bacula source code has been released under the AGPL version 3 license.

%define user_file  /etc/passwd
%define group_file /etc/group

# program locations
%define useradd  /usr/sbin/useradd
%define groupadd /usr/sbin/groupadd
%define usermod  /usr/sbin/usermod

# platform defines - set one below or define the build_xxx on the command line
# RedHat builds
%define rh7 0
%{?build_rh7:%define rh7 1}
%define rh8 0
%{?build_rh8:%define rh8 1}
%define rh9 0
%{?build_rh9:%define rh9 1}
# Fedora Core build
%define fc1 0
%{?build_fc1:%define fc1 1}
%define fc3 0
%{?build_fc3:%define fc3 1}
%define fc4 0
%{?build_fc4:%define fc4 1}
%define fc5 0
%{?build_fc5:%define fc5 1}
%define fc6 0
%{?build_fc6:%define fc6 1}
%define fc7 0
%{?build_fc7:%define fc7 1}
%define fc8 0
%{?build_fc8:%define fc8 1}
%define fc9 0
%{?build_fc9:%define fc9 1}
%define fc10 0
%{?build_fc10:%define fc10 1}
%define fc11 0
%{?build_fc11:%define fc11 1}
%define fc12 0
%{?build_fc12:%define fc12 1}
# Whitebox Enterprise build
%define wb3 0
%{?build_wb3:%define wb3 1}
# RedHat Enterprise builds
%define rhel3 0
%{?build_rhel3:%define rhel3 1}
%{?build_rhel3:%define wb3 1}
%define rhel4 0
%{?build_rhel4:%define rhel4 1}
%{?build_rhel4:%define fc3 1}
%define rhel5 0
%{?build_rhel5:%define rhel5 1}
%{?build_rhel5:%define fc6 1}
# CentOS build
%define centos3 0
%{?build_centos3:%define centos3 1}
%{?build_centos3:%define wb3 1}
%define centos4 0
%{?build_centos4:%define centos4 1}
%{?build_centos4:%define fc3 1}
%define centos5 0
%{?build_centos5:%define centos5 1}
%{?build_centos5:%define fc6 1}
# SL build
%define sl3 0
%{?build_sl3:%define sl3 1}
%{?build_sl3:%define wb3 1}
%define sl4 0
%{?build_sl4:%define sl4 1}
%{?build_sl4:%define fc3 1}
%define sl5 0
%{?build_sl5:%define sl5 1}
%{?build_sl5:%define fc6 1}
# SuSE build
%define su9 0
%{?build_su9:%define su9 1}
%define su10 0
%{?build_su10:%define su10 1}
%define su102 0
%{?build_su102:%define su102 1}
%define su103 0
%{?build_su103:%define su103 1}
%define su110 0
%{?build_su110:%define su110 1}
%define su111 0
%{?build_su111:%define su111 1}
%define su112 0
%{?build_su112:%define su112 1}
# Mandrake builds
%define mdk 0
%{?build_mdk:%define mdk 1}
%define mdv 0
%{?build_mdv:%define mdv 1}
%{?build_mdv:%define mdk 1}

# client only build
%define client_only 0
%{?build_client_only:%define client_only 1}

# Setup some short cuts
%define rhat 0
%if %{rh7} || %{rh8} || %{rh9}
%define rhat 1
%endif
%define fed 0
%if %{fc1} || %{fc3} || %{fc4} || %{fc5} || %{fc6} || %{fc7} || %{fc8} || %{fc9} || %{fc10} || %{fc11} || %{fc12}
%define fed 1
%endif
%define suse 0
%if %{su9} || %{su10} || %{su102} || %{su103} || %{su110} || %{su111} || %{su112}
%define suse 1
%endif
%define rhel 0
%if %{rhel3} || %{rhel4} || %{rhel5} || %{centos3} || %{centos4} || %{centos5}
%define rhel 1
%endif
%define scil 0
%if %{sl3} || %{sl4} || %{sl5}
%define scil 1
%endif


# test for a platform definition
%if !%{rhat} && !%{rhel} && !%{fed} && !%{wb3} && !%{suse} && !%{mdk} && !%{scil}
%{error: You must specify a platform. Please examine the spec file.}
exit 1
%endif

# distribution-specific directory for logwatch
%if %{wb3} || %{rh7} || %{rh8} || %{rh9} || %{fc1} || %{fc3} || %{fc4}
%define logwatch_dir /etc/log.d
%else
%define logwatch_dir /etc/logwatch
%endif

# database defines
# set for database support desired or define the build_xxx on the command line
%define mysql 0
%{?build_mysql:%define mysql 1}
%define sqlite 0
%{?build_sqlite:%define sqlite 1}
%define postgresql 0
%{?build_postgresql:%define postgresql 1}

# test for a database definition
%if ! %{mysql} && ! %{sqlite} && ! %{postgresql} && ! %{client_only}
%{error: You must specify database support, by passing one of the following to rpmbuild:}
%{error:   --define build_postgresql=1}
%{error:   --define build_sqlite=1}
%{error:   --define build_mysql=1}
exit 1
%endif

%if %{mysql}
%define db_backend mysql
%endif
%if %{sqlite}
%define db_backend sqlite3
%endif
%if %{postgresql}
%define db_backend postgresql
%endif

# 64 bit support
%define x86_64 0
%{?build_x86_64:%define x86_64 1}

# check what distribution we are
%if %{rhat} || %{rhel}
%define _dist %(grep Red /etc/redhat-release)
%endif
%if %{fc1} || %{fc3} || %{fc4} || %{fc5} || %{fc7} || %{fc8} || %{fc9} || %{fc10} || %{fc11} || %{fc12}
%define _dist %(grep Fedora /etc/redhat-release)
%endif
%if %{centos5} || %{centos4} || %{centos3}
%define _dist %(grep CentOS /etc/redhat-release)
%endif
%if %{sl5} ||%{sl4} || %{sl3}
%define _dist %(grep 'Scientific Linux' /etc/redhat-release)
%endif
%if %{wb3} && ! %{rhel3} && ! %{centos3} && ! %{sl3}
%define _dist %(grep White /etc/whitebox-release)
%endif
%if %{suse}
%define _dist %(grep -i SuSE /etc/SuSE-release)
%endif
%if %{mdk}
%define _dist %(grep Mand /etc/mandrake-release)
%endif
%{?DISTNAME:%define _dist %{DISTNAME}}

# only set Disribution if not in opensuse build service, as it sets it itself
%if ! 0%{?opensuse_bs}
Distribution: %{_dist}
%endif

%if 0%{?opensuse_bs} &&  %{mysql} && %{suse}
# needed in opensuse_bs, as rpm is installed during build process
BuildRequires: libmysqlclient-devel
BuildRequires: mysql-client
BuildRequires: mysql
%endif
%if 0%{?opensuse_bs} &&  %{suse} && %{postgresql}
BuildRequires: %{postgres_package}
BuildRequires: %{postgres_server_package}
%endif
BuildRequires: openssl

%if 0%{?opensuse_bs} && %{suse}
BuildRequires: pwdutils
BuildRequires: sysconfig
%endif

# should we turn on python support
%define python 0
%{?build_python:%define python 1}

# should we enable tcp wrappers support
%define tcpwrappers 0
%{?build_tcpwrappers:%define tcpwrappers 1}

# Mandriva somehow forces the manpage file extension to bz2 rather than gz
%if %{mdk}
%define manpage_ext bz2
%else
%define manpage_ext gz
%endif

# for client only build
%if %{client_only}
%define mysql 0
%define postgresql 0
%define sqlite 0
%endif

BuildRequires: gcc, gcc-c++, make, autoconf
BuildRequires: glibc, glibc-devel
BuildRequires: ncurses-devel, perl, readline-devel
BuildRequires: libstdc++-devel, zlib-devel
BuildRequires: openssl-devel
BuildRequires: libacl-devel
BuildRequires: pkgconfig
%if ! %{rh7}
BuildRequires: libxml2-devel
%endif
%if %{python}
BuildRequires: python, python-devel
%{expand: %%define pyver %(python -c 'import sys;print(sys.version[0:3])')}
%endif

%if %{rh7}
BuildRequires: libxml-devel
%endif
%if %{mdk}
BuildRequires: libstdc++-static-devel
BuildRequires: glibc-static-devel
%endif

%if %{suse}
BuildRequires: termcap
%else
BuildRequires: libtermcap-devel
%endif

%if %{sqlite}
BuildRequires: sqlite-devel
%endif

%if %{mysql}
BuildRequires: mysql-devel
%endif

%if %{postgresql} && %{wb3}
BuildRequires: rh-postgresql-devel >= %{postgres_version}
%endif

%if %{postgresql} && ! %{wb3}
BuildRequires: %{postgres_devel_package} >= %{postgres_version}
%endif

%description
%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

%if %{mysql}
%package mysql
%endif
%if %{sqlite}
%package sqlite
%endif
%if %{postgresql}
%package postgresql
%endif

Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons
Provides: bacula-dir, bacula-sd, bacula-fd, bacula-server
Conflicts: bacula-client

Requires: ncurses, libstdc++, zlib, openssl
Requires: glibc, readline, %{name}-libs

%if %{suse}
Conflicts: bacula
Requires: termcap
%else
Requires: libtermcap
%endif

%if %{mysql}
Requires: mysql
%endif

%if %{postgresql} && %{wb3}
Requires: rh-postgresql >= 7
%endif
%if %{postgresql} && ! %{wb3}
Requires: postgresql >= 7
%endif

%if %{mysql}
%description mysql
%endif
%if %{sqlite}
%description sqlite
%endif
%if %{postgresql}
%description postgresql
%endif

%if %{python}
Requires: python >= %{pyver}
%endif

%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

%if %{mysql}
This build requires MySQL to be installed separately as the catalog database.
%endif
%if %{postgresql}
This build requires PostgreSQL to be installed separately as the catalog database.
%endif
%if %{sqlite}
This build incorporates sqlite3 as the catalog database, statically compiled.
%endif
%if %{python}
This build includes python scripting support.
%endif
%if %{tcpwrappers}
This build includes tcp-wrappers support.
%endif

%package client
Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons
Provides: bacula-fd
Conflicts: bacula-mysql
Conflicts: bacula-sqlite
Conflicts: bacula-postgresql

%if %{suse}
Provides: bacula
%endif

Requires: libstdc++, zlib, openssl
Requires: glibc, readline, %{name}-libs

%if %{suse}
Requires: termcap
%else
Requires: libtermcap
%endif

%if %{python}
Requires: python >= %{pyver}
%endif

%description client
%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

This is the File daemon (Client) only package. It includes the command line 
console program.
%if %{python}
This build includes python scripting support.
%endif
%if %{tcpwrappers}
This build includes tcp-wrappers support.
%endif

%if ! %{client_only}
%package updatedb

Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons

%description updatedb
%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

This package installs scripts for updating older versions of the bacula
database.
%endif

%package libs

Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons

%description libs
%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

This package installs the shared libraries used by many bacula programs.

# Must explicitly enable debug pkg on SuSE
# but not in opensuse_bs
%if %{suse} && ! 0%{?opensuse_bs}
%debug_package
%endif

%prep
%setup
%setup -T -D -b 1
%setup -T -D -b 2

%build

%if %{suse}
export LDFLAGS="${LDFLAGS} -L/usr/lib/termcap -L/usr/lib64/termcap"
%endif

cwd=${PWD}
%if %{sqlite}
cd %{depkgs}
make sqlite3
cd ${cwd}
%endif


%if %{sqlite}
# patches for the bundled sqlite scripts

# patch the make_sqlite_tables script for installation bindir
patch src/cats/make_sqlite3_tables.in src/cats/make_sqlite3_tables.in.patch

# patch the create_sqlite_database script for installation bindir
patch src/cats/create_sqlite3_database.in src/cats/create_sqlite3_database.in.patch

# patch the make_catalog_backup script for installation bindir
patch src/cats/make_catalog_backup.in src/cats/make_catalog_backup.in.patch

# patch the update_sqlite_tables script for installation bindir
patch src/cats/update_sqlite3_tables.in src/cats/update_sqlite3_tables.in.patch

# patch the bacula-dir init script to remove sqlite service
%if %{sqlite} && %{suse}
patch platforms/suse/bacula-dir.in platforms/suse/bacula-dir-suse-sqlite.patch
%endif
%endif

# 64 bit lib location hacks
# as of 1.39.18 it should not be necessary to enable x86_64 as configure is
# reported to be fixed to properly detect lib locations.
%if %{x86_64}
export LDFLAGS="${LDFLAGS} -L/usr/lib64"
%endif
%if %{mysql} && %{x86_64}
export LDFLAGS="${LDFLAGS} -L/usr/lib64/mysql"
%endif
%if %{python} && %{x86_64}
export LDFLAGS="${LDFLAGS} -L/usr/lib64/python%{pyver}"
%endif

# Main Bacula configuration
%configure \
        --prefix=%{_prefix} \
        --sbindir=%{_sbindir} \
        --sysconfdir=%{sysconf_dir} \
        --mandir=%{_mandir} \
        --with-scriptdir=%{script_dir} \
        --with-working-dir=%{working_dir} \
        --with-plugindir=%{script_dir} \
        --with-pid-dir=%{pid_dir} \
        --with-subsys-dir=%{_subsysdir} \
        --enable-smartalloc \
        --disable-gnome \
        --disable-bwx-console \
        --disable-tray-monitor \
        --disable-conio \
        --enable-readline \
%if %{mysql}
        --with-mysql \
%endif
%if %{sqlite}
        --with-sqlite3=${cwd}/%{depkgs}/sqlite3 \
%endif
%if %{postgresql}
        --with-postgresql \
%endif
        --disable-bat \
%if %{python}
        --with-python \
%endif
%if %{tcpwrappers}
        --with-tcp-wrappers  \
%endif
%if %{client_only}
        --enable-client-only \
%endif
%if %{rh7} || %{rh8} || %{rh9} || %{fc1} || %{fc3} || %{wb3} 
        --disable-batch-insert \
%endif
        --with-dir-user=%{director_daemon_user} \
        --with-dir-group=%{daemon_group} \
        --with-sd-user=%{storage_daemon_user} \
        --with-sd-group=%{storage_daemon_group} \
        --with-fd-user=%{file_daemon_user} \
        --with-fd-group=%{daemon_group} \
        --with-dir-password="XXX_REPLACE_WITH_DIRECTOR_PASSWORD_XXX" \
        --with-fd-password="XXX_REPLACE_WITH_CLIENT_PASSWORD_XXX" \
        --with-sd-password="XXX_REPLACE_WITH_STORAGE_PASSWORD_XXX" \
        --with-mon-dir-password="XXX_REPLACE_WITH_DIRECTOR_MONITOR_PASSWORD_XXX" \
        --with-mon-fd-password="XXX_REPLACE_WITH_CLIENT_MONITOR_PASSWORD_XXX" \
        --with-mon-sd-password="XXX_REPLACE_WITH_STORAGE_MONITOR_PASSWORD_XXX" \
        --with-openssl

make -j3

%install
 
cwd=${PWD}
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
mkdir -p $RPM_BUILD_ROOT%{logwatch_dir}/conf/logfiles
mkdir -p $RPM_BUILD_ROOT%{logwatch_dir}/conf/services
mkdir -p $RPM_BUILD_ROOT%{logwatch_dir}/scripts/services
mkdir -p $RPM_BUILD_ROOT%{logwatch_dir}/scripts/shared
mkdir -p $RPM_BUILD_ROOT%{script_dir}/updatedb

mkdir -p $RPM_BUILD_ROOT/etc/pam.d
mkdir -p $RPM_BUILD_ROOT%{_sbindir}

%if %{sqlite}
mkdir -p $RPM_BUILD_ROOT%{sqlite_bindir}
%endif

make DESTDIR=$RPM_BUILD_ROOT install

%if %{client_only}
# Program docs not installed on client
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bacula-dir.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bacula-sd.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bcopy.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bextract.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bls.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/bscan.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/btape.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man8/dbcheck.8.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man1/bsmtp.1.%{manpage_ext}
%endif
# Docs for programs that are depreciated
rm -f $RPM_BUILD_ROOT%{_mandir}/man1/bacula-bgnome-console.1.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man1/bacula-bwxconsole.1.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{_mandir}/man1/bacula-tray-monitor.1.%{manpage_ext}
rm -f $RPM_BUILD_ROOT%{script_dir}/gconsole

# fixme - make installs the mysql scripts for sqlite build
%if %{sqlite}
rm -f $RPM_BUILD_ROOT%{script_dir}/startmysql
rm -f $RPM_BUILD_ROOT%{script_dir}/stopmysql
rm -f $RPM_BUILD_ROOT%{script_dir}/grant_mysql_privileges
%endif

# fixme - make installs the mysql scripts for postgresql build
%if %{postgresql}
rm -f $RPM_BUILD_ROOT%{script_dir}/startmysql
rm -f $RPM_BUILD_ROOT%{script_dir}/stopmysql
%endif

# install the init scripts
%if %{suse}
cp -p platforms/suse/bacula-dir $RPM_BUILD_ROOT/etc/init.d/bacula-dir
cp -p platforms/suse/bacula-fd $RPM_BUILD_ROOT/etc/init.d/bacula-fd
cp -p platforms/suse/bacula-sd $RPM_BUILD_ROOT/etc/init.d/bacula-sd
%endif
%if %{mdk}
cp -p platforms/mandrake/bacula-dir $RPM_BUILD_ROOT/etc/init.d/bacula-dir
cp -p platforms/mandrake/bacula-fd $RPM_BUILD_ROOT/etc/init.d/bacula-fd
cp -p platforms/mandrake/bacula-sd $RPM_BUILD_ROOT/etc/init.d/bacula-sd
%endif
%if ! %{suse} && ! %{mdk}
cp -p platforms/redhat/bacula-dir $RPM_BUILD_ROOT/etc/init.d/bacula-dir
cp -p platforms/redhat/bacula-fd $RPM_BUILD_ROOT/etc/init.d/bacula-fd
cp -p platforms/redhat/bacula-sd $RPM_BUILD_ROOT/etc/init.d/bacula-sd
%endif
chmod 0754 $RPM_BUILD_ROOT/etc/init.d/*
%if %{client_only}
rm -f $RPM_BUILD_ROOT/etc/init.d/bacula-dir
rm -f $RPM_BUILD_ROOT/etc/init.d/bacula-sd
%endif

# install sqlite
%if %{sqlite}
cp -p %{depkgs}/sqlite3/sqlite3 $RPM_BUILD_ROOT%{sqlite_bindir}/sqlite3
cp -p %{depkgs}/sqlite3/sqlite3.h $RPM_BUILD_ROOT%{sqlite_bindir}/sqlite3.h
cp -p %{depkgs}/sqlite3/libsqlite3.a $RPM_BUILD_ROOT%{sqlite_bindir}/libsqlite3.a
%endif


# install the logrotate file
cp -p scripts/logrotate $RPM_BUILD_ROOT/etc/logrotate.d/bacula

# install the updatedb scripts
cp -p updatedb/* $RPM_BUILD_ROOT%{script_dir}/updatedb/

# install the sample-query.sql file
%if ! %{client_only}
cp -p examples/sample-query.sql $RPM_BUILD_ROOT%{script_dir}/sample-query.sql
%endif

# install the logwatch scripts
%if ! %{client_only}
cp -p scripts/logwatch/bacula $RPM_BUILD_ROOT%{logwatch_dir}/scripts/services/bacula
cp -p scripts/logwatch/applybaculadate $RPM_BUILD_ROOT%{logwatch_dir}/scripts/shared/applybaculadate
cp -p scripts/logwatch/logfile.bacula.conf $RPM_BUILD_ROOT%{logwatch_dir}/conf/logfiles/bacula.conf
cp -p scripts/logwatch/services.bacula.conf $RPM_BUILD_ROOT%{logwatch_dir}/conf/services/bacula.conf
chmod 755 $RPM_BUILD_ROOT%{logwatch_dir}/scripts/services/bacula
chmod 755 $RPM_BUILD_ROOT%{logwatch_dir}/scripts/shared/applybaculadate
chmod 644 $RPM_BUILD_ROOT%{logwatch_dir}/conf/logfiles/bacula.conf
chmod 644 $RPM_BUILD_ROOT%{logwatch_dir}/conf/services/bacula.conf
%endif

# now clean up permissions that are left broken by the install
chmod o-rwx $RPM_BUILD_ROOT%{working_dir}

# fix me - building enable-client-only installs files not included in bacula-client package
%if %{client_only}
rm -f $RPM_BUILD_ROOT%{script_dir}/bacula
rm -f $RPM_BUILD_ROOT%{script_dir}/bacula_config
rm -f $RPM_BUILD_ROOT%{script_dir}/bacula-ctl-dir
rm -f $RPM_BUILD_ROOT%{script_dir}/bacula-ctl-sd
rm -f $RPM_BUILD_ROOT%{script_dir}/disk-changer
rm -f $RPM_BUILD_ROOT%{script_dir}/dvd-handler
rm -f $RPM_BUILD_ROOT%{script_dir}/mtx-changer
rm -f $RPM_BUILD_ROOT%{script_dir}/startmysql
rm -f $RPM_BUILD_ROOT%{script_dir}/stopmysql
rm -rf $RPM_BUILD_ROOT%{script_dir}/updatedb
rm -f $RPM_BUILD_ROOT%{script_dir}/bconsole
rm -f $RPM_BUILD_ROOT%{script_dir}/bpipe-fd.so
rm -f $RPM_BUILD_ROOT%{script_dir}/mtx-changer.conf
rm -f $RPM_BUILD_ROOT%{_sbindir}/bacula
%endif

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
%if 0%{?opensuse_bs}
rm -f $RPM_BUILD_DIR/Release_Notes-%{version}-1.txt
%else
rm -f $RPM_BUILD_DIR/Release_Notes-%{version}-%{release}.txt
%endif


%if %{mysql}
# MySQL specific files
%files mysql
%defattr(-, root, root)
%attr(-, root, %{daemon_group}) %{script_dir}/create_mysql_database
%attr(-, root, %{daemon_group}) %{script_dir}/drop_mysql_database
%attr(-, root, %{daemon_group}) %{script_dir}/make_mysql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/drop_mysql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/update_mysql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/grant_mysql_privileges
%attr(-, root, %{daemon_group}) %{script_dir}/startmysql
%attr(-, root, %{daemon_group}) %{script_dir}/stopmysql
%{_libdir}/libbacsql*
%endif

%if %{sqlite}
%files sqlite
%defattr(-,root,root)
%attr(-, root, %{daemon_group}) %{script_dir}/create_sqlite3_database
%attr(-, root, %{daemon_group}) %{script_dir}/drop_sqlite3_database
%attr(-, root, %{daemon_group}) %{script_dir}/grant_sqlite3_privileges
%attr(-, root, %{daemon_group}) %{script_dir}/make_sqlite3_tables
%attr(-, root, %{daemon_group}) %{script_dir}/drop_sqlite3_tables
%attr(-, root, %{daemon_group}) %{script_dir}/update_sqlite3_tables
%{sqlite_bindir}/libsqlite3.a
%{sqlite_bindir}/sqlite3.h
%{sqlite_bindir}/sqlite3
%endif



%if %{postgresql}
%files postgresql
%defattr(-,root,root)
%attr(-, root, %{daemon_group}) %{script_dir}/create_postgresql_database
%attr(-, root, %{daemon_group}) %{script_dir}/drop_postgresql_database
%attr(-, root, %{daemon_group}) %{script_dir}/make_postgresql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/drop_postgresql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/update_postgresql_tables
%attr(-, root, %{daemon_group}) %{script_dir}/grant_postgresql_privileges
%{_libdir}/libbacsql*
%endif

# The rest is DB backend independent

%if ! %{client_only}
%attr(-, root, %{daemon_group}) %dir %{script_dir}
%attr(-, root, %{daemon_group}) %dir %{sysconf_dir}
%attr(-, root, %{daemon_group}) %{script_dir}/bacula
%attr(-, root, %{daemon_group}) %{script_dir}/bacula_config
%attr(-, root, %{daemon_group}) %{script_dir}/bconsole
%attr(-, root, %{daemon_group}) %{script_dir}/create_bacula_database
%attr(-, root, %{daemon_group}) %{script_dir}/drop_bacula_database
%attr(-, root, %{daemon_group}) %{script_dir}/grant_bacula_privileges
%attr(-, root, %{daemon_group}) %{script_dir}/make_bacula_tables
%attr(-, root, %{daemon_group}) %{script_dir}/drop_bacula_tables
%attr(-, root, %{daemon_group}) %{script_dir}/update_bacula_tables
%attr(-, root, %{daemon_group}) %{script_dir}/make_catalog_backup
%attr(-, root, %{daemon_group}) %{script_dir}/make_catalog_backup.pl
%attr(-, root, %{daemon_group}) %{script_dir}/delete_catalog_backup
%attr(-, root, %{daemon_group}) %{script_dir}/btraceback.dbx
%attr(-, root, %{daemon_group}) %{script_dir}/btraceback.gdb
%attr(-, root, %{daemon_group}) %{script_dir}/btraceback.mdb
%attr(-, root, %{daemon_group}) %{script_dir}/disk-changer
%attr(-, root, %{daemon_group}) %{script_dir}/bacula-ctl-dir
%attr(-, root, %{daemon_group}) %{script_dir}/bacula-ctl-fd
%attr(-, root, %{daemon_group}) %{script_dir}/bacula-ctl-sd
%attr(-, root, %{daemon_group}) %{script_dir}/bpipe-fd.so
%attr(-, root, %{daemon_group}) /etc/init.d/bacula-dir
%attr(-, root, %{daemon_group}) /etc/init.d/bacula-fd
%attr(-, root, %{daemon_group}) %{_sbindir}/dbcheck
%attr(-, root, %{storage_daemon_group}) %{script_dir}/dvd-handler
%attr(-, root, %{storage_daemon_group}) /etc/init.d/bacula-sd
%attr(-, root, %{storage_daemon_group}) %{script_dir}/mtx-changer
%attr(-, root, %{storage_daemon_group}) %config(noreplace) %{script_dir}/mtx-changer.conf

/etc/logrotate.d/bacula
%{logwatch_dir}/scripts/services/bacula
%{logwatch_dir}/scripts/shared/applybaculadate
%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bacula-dir.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bacula-fd.conf
%attr(-, root, %{storage_daemon_group}) %config(noreplace) %{sysconf_dir}/bacula-sd.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bconsole.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{logwatch_dir}/conf/logfiles/bacula.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{logwatch_dir}/conf/services/bacula.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{script_dir}/query.sql
%attr(-, root, %{daemon_group}) %{script_dir}/sample-query.sql

%attr(-, %{storage_daemon_user}, %{daemon_group}) %dir %{working_dir}

%{_sbindir}/bacula-dir
%{_sbindir}/bacula-fd
%{_sbindir}/bacula-sd
%{_sbindir}/bacula
%{_sbindir}/bcopy
%{_sbindir}/bextract
%{_sbindir}/bls
%{_sbindir}/bscan
%{_sbindir}/btape
%{_sbindir}/btraceback
%{_sbindir}/bconsole
%{_sbindir}/bsmtp
%{_sbindir}/bregex
%{_sbindir}/bwild
%{_mandir}/man8/bacula-fd.8.%{manpage_ext}
%{_mandir}/man8/bacula-dir.8.%{manpage_ext}
%{_mandir}/man8/bacula-sd.8.%{manpage_ext}
%{_mandir}/man8/bacula.8.%{manpage_ext}
%{_mandir}/man8/bconsole.8.%{manpage_ext}
%{_mandir}/man8/bcopy.8.%{manpage_ext}
%{_mandir}/man8/bextract.8.%{manpage_ext}
%{_mandir}/man8/bls.8.%{manpage_ext}
%{_mandir}/man8/bscan.8.%{manpage_ext}
%{_mandir}/man8/btape.8.%{manpage_ext}
%{_mandir}/man8/btraceback.8.%{manpage_ext}
%{_mandir}/man8/dbcheck.8.%{manpage_ext}
%{_mandir}/man1/bsmtp.1.%{manpage_ext}
%{_mandir}/man1/bat.1.%{manpage_ext}
%_prefix/share/doc/*

# opensuse build service changes the release itself
%if 0%{?opensuse_bs}
%doc ../Release_Notes-%{version}-1.txt
%else
%doc ../Release_Notes-%{version}-%{release}.txt
%endif
%endif

%if %{mysql}
%pre mysql
# test for bacula database older than version 12
# note: this ASSUMES no password has been set for bacula database
DB_VER=`mysql 2>/dev/null bacula -e 'select * from Version;'|tail -n 1`
%endif

%if %{sqlite}
%pre sqlite
# are we upgrading from sqlite to sqlite3?
if [ -s %{working_dir}/bacula.db ] && [ -s %{sqlite_bindir}/sqlite ];then
        echo "This version of bacula-sqlite involves an upgrade to sqlite3."
        echo "Your catalog database file is not compatible with sqlite3, thus"
        echo "you will need to dump the data, delete the old file, and re-run"
        echo "this rpm upgrade."
        echo ""
        echo "Backing up your current database..."
        echo ".dump" | %{sqlite_bindir}/sqlite %{working_dir}/bacula.db > %{working_dir}/bacula_backup.sql
        mv %{working_dir}/bacula.db %{working_dir}/bacula.db.old
        echo "Your catalog data has been saved in %{working_dir}/bacula_backup.sql and your"
        echo "catalog file has been renamed %{working_dir}/bacula.db.old."
        echo ""
        echo "Please re-run this rpm package upgrade."
        echo "After the upgrade is complete, restore your catalog"
        echo "with the following commands:"
        echo "%{script_dir}/drop_sqlite3_tables"
        echo "cd %{working_dir}"
        echo "%{sqlite_bindir}/sqlite3 $* bacula.db < bacula_backup.sql"
        echo "chown bacula.bacula bacula.db"
        exit 1
fi
# test for bacula database older than version 11 and sqlite3
if [ -s %{working_dir}/bacula.db ] && [ -s %{sqlite_bindir}/sqlite3 ];then
        DB_VER=`echo "select * from Version;" | %{sqlite_bindir}/sqlite3 2>/dev/null %{working_dir}/bacula.db | tail -n 1`
%endif

%if %{postgresql}
%pre postgresql
DB_VER=`echo 'select * from Version;' | psql bacula 2>/dev/null | tail -3 | head -1`
%endif

%if ! %{client_only}
if [ -n "$DB_VER" ] && [ "$DB_VER" -lt "11" ]; then
    echo "This bacula upgrade will update a bacula database from version 11 to 12."
    echo "You appear to be running database version $DB_VER. You must first update"
    echo "your database to version 11 and then install this upgrade. The alternative"
    echo "is to use %{script_dir}/drop_%{db_backend}_tables to delete all your your current"
    echo "catalog information, then do the upgrade. Information on updating a"
    echo "database older than version 11 can be found in the release notes."
    exit 1
fi
%endif

%if %{sqlite}
fi
%endif

%if ! %{client_only}
# check for and copy %{sysconf_dir}/console.conf to bconsole.conf
if [ -s %{sysconf_dir}/console.conf ];then
    cp -p %{sysconf_dir}/console.conf %{sysconf_dir}/bconsole.conf
fi

# create the daemon users and groups
# first create the groups if they don't exist
HAVE_BACULA=`grep %{daemon_group} %{group_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
        %{groupadd} -r %{daemon_group} > /dev/null 2>&1
        echo "The group %{daemon_group} has been added to %{group_file}."
        echo "See the manual chapter \"Running Bacula\" for details."
fi
HAVE_BACULA=`grep %{storage_daemon_group} %{group_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
        %{groupadd} -r %{storage_daemon_group} > /dev/null 2>&1
        echo "The group %{storage_daemon_group} has been added to %{group_file}."
        echo "See the manual chapter \"Running Bacula\" for details."
fi
# now create the users if they do not exist
# we do not use the -g option allowing the primary group to be set to system default
# this will be a unique group on redhat type systems or the group users on some systems
HAVE_BACULA=`grep %{storage_daemon_user} %{user_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
        %{useradd} -r -c "Bacula" -d %{working_dir} -g %{storage_daemon_group} -M -s /sbin/nologin %{storage_daemon_user} > /dev/null 2>&1
        echo "The user %{storage_daemon_user} has been added to %{user_file}."
        echo "See the manual chapter \"Running Bacula\" for details."
fi
HAVE_BACULA=`grep %{director_daemon_user} %{user_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
        %{useradd} -r -c "Bacula" -d %{working_dir} -g %{daemon_group} -M -s /sbin/nologin %{director_daemon_user} > /dev/null 2>&1
        echo "The user %{director_daemon_user} has been added to %{user_file}."
        echo "See the manual chapter \"Running Bacula\" for details."
fi
HAVE_BACULA=`grep %{file_daemon_user} %{user_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
        %{useradd} -r -c "Bacula" -d %{working_dir} -g %{daemon_group} -M -s /sbin/nologin %{file_daemon_user} > /dev/null 2>&1
        echo "The user %{file_daemon_user} has been added to %{user_file}."
        echo "See the manual chapter \"Running Bacula\" for details."
fi
# now we add the supplementary groups, this is ok to call even if the users already exist
# we only do this if the user is NOT root
IS_ROOT=%{director_daemon_user}
if [ "$IS_ROOT" != "root" ]; then
%{usermod} -G %{daemon_group} %{director_daemon_user}
fi
IS_ROOT=%{storage_daemon_user}
if [ "$IS_ROOT" != "root" ]; then
%{usermod} -G %{daemon_group},%{storage_daemon_group} %{storage_daemon_user}
fi
IS_ROOT=%{file_daemon_user}
if [ "$IS_ROOT" != "root" ]; then
%{usermod} -G %{daemon_group} %{file_daemon_user}
fi
%endif

%if %{mysql}
%post mysql
%endif
%if %{sqlite}
%post sqlite
%endif
%if %{postgresql}
%post postgresql
%endif
%if ! %{client_only}
# add our links
if [ "$1" -ge 1 ] ; then
%if %{suse} && %{mysql}
  /sbin/chkconfig --add mysql
%endif
%if %{suse} && %{postgresql}
  /sbin/chkconfig --add postgresql
%endif
  /sbin/chkconfig --add bacula-dir
  /sbin/chkconfig --add bacula-fd
  /sbin/chkconfig --add bacula-sd
fi
%endif

%if %{mysql}

#check, if mysql can be called successfully at all
if mysql 2>/dev/null bacula -e 'select * from Version;' ; then

# test for an existing database
# note: this ASSUMES no password has been set for bacula database
DB_VER=`mysql 2>/dev/null bacula -e 'select * from Version;'|tail -n 1`

# grant privileges and create tables if they do not exist
if [ -z "$DB_VER" ]; then
    echo "Hmm, it doesn't look like you have an existing database."
    echo "Granting privileges for MySQL user bacula..."
    %{script_dir}/grant_mysql_privileges
    echo "Creating MySQL bacula database..."
    %{script_dir}/create_mysql_database
    echo "Creating bacula tables..."
    %{script_dir}/make_mysql_tables

# check to see if we need to upgrade a 3.x database
elif [ "$DB_VER" -lt "12" ]; then
    echo "This release requires an upgrade to your bacula database."
    echo "Backing up your current database..."
    mysqldump -f --opt bacula | bzip2 > %{working_dir}/bacula_backup.sql.bz2
    echo "Upgrading bacula database ..."
    %{script_dir}/update_mysql_tables
    echo "If bacula works correctly you can remove the backup file %{working_dir}/bacula_backup.sql.bz2"

fi
fi
%endif

%if %{sqlite}
# test for an existing database
if [ -s %{working_dir}/bacula.db ]; then
        DB_VER=`echo "select * from Version;" | %{sqlite_bindir}/sqlite3 2>/dev/null %{working_dir}/bacula.db | tail -n 1`
        # check to see if we need to upgrade a 3.x database
        if [ "$DB_VER" -lt "12" ] && [ "$DB_VER" -ge "11" ]; then
                echo "This release requires an upgrade to your bacula database."
                echo "Backing up your current database..."
                echo ".dump" | %{sqlite_bindir}/sqlite3 %{working_dir}/bacula.db | bzip2 > %{working_dir}/bacula_backup.sql.bz2
                echo "Upgrading bacula database ..."
                %{script_dir}/update_sqlite3_tables
                echo "If bacula works correctly you can remove the backup file %{working_dir}/bacula_backup.sql.bz2"
        fi
else
        # create the database and tables
        echo "Hmm, doesn't look like you have an existing database."
        echo "Creating SQLite database..."
        %{script_dir}/create_sqlite3_database
        chown %{director_daemon_user}.%{daemon_group} %{working_dir}/bacula.db
        echo "Creating the SQLite tables..."
        %{script_dir}/make_sqlite3_tables
fi
%endif

%if %{postgresql}
# check if psql can be called successfully at all
if echo 'select * from Version;' | psql bacula 2>/dev/null; then

# test for an existing database
# note: this ASSUMES no password has been set for bacula database
DB_VER=`echo 'select * from Version;' | psql bacula 2>/dev/null | tail -3 | head -1`

# grant privileges and create tables if they do not exist
if [ -z "$DB_VER" ]; then
    echo "Hmm, doesn't look like you have an existing database."
    echo "Creating PostgreSQL bacula database..."
    %{script_dir}/create_postgresql_database
    echo "Creating bacula tables..."
    %{script_dir}/make_postgresql_tables
    echo "Granting privileges for PostgreSQL user bacula..."
    %{script_dir}/grant_postgresql_privileges

# check to see if we need to upgrade a 5.0.x database
elif [ "$DB_VER" -lt "12" ]; then
    echo "This release requires an upgrade to your bacula database."
    echo "Backing up your current database..."
    pg_dump bacula | bzip2 > %{working_dir}/bacula_backup.sql.bz2
    echo "Upgrading bacula database ..."
    %{script_dir}/update_postgresql_tables
    echo "If bacula works correctly you can remove the backup file %{working_dir}/bacula_backup.sql.bz2"
        
fi
fi
%endif

%if ! %{client_only}
if [ -d %{sysconf_dir} ]; then
   cd %{sysconf_dir}
   for string in XXX_REPLACE_WITH_DIRECTOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_PASSWORD_XXX XXX_REPLACE_WITH_DIRECTOR_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_MONITOR_PASSWORD_XXX; do
      pass=`openssl rand -base64 33`
      for file in *.conf; do
         sed "s@${string}@${pass}@g" $file > $file.new
         cp -f $file.new $file; rm -f $file.new
      done
   done
# put actual hostname in conf file
   host=`hostname`
   string="XXX_HOSTNAME_XXX"
   for file in *.conf; do
      sed "s@${string}@${host}@g" $file >$file.new
      cp -f $file.new $file; rm -f $file.new
   done
fi
%endif


%if %{mysql}
%preun mysql
%endif
%if %{sqlite}
%preun sqlite
%endif
%if %{postgresql}
%preun postgresql
%endif

%if ! %{client_only}
# delete our links
if [ $1 = 0 ]; then
  /sbin/chkconfig --del bacula-dir
  /sbin/chkconfig --del bacula-fd
  /sbin/chkconfig --del bacula-sd
fi
%endif

%files client
%defattr(-,root,root)
%attr(-, root, %{daemon_group}) %dir %{script_dir}
%attr(-, root, %{daemon_group}) %dir %{sysconf_dir}
%{script_dir}/bacula-ctl-fd
/etc/init.d/bacula-fd

/etc/logrotate.d/bacula

%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bacula-fd.conf
%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bconsole.conf
%attr(-, root, %{daemon_group}) %dir %{working_dir}

%{_sbindir}/bacula-fd
%{_sbindir}/btraceback
%attr(-, root, %{daemon_group}) %{script_dir}/btraceback.gdb
%attr(-, root, %{daemon_group}) %{script_dir}/btraceback.dbx
%{_sbindir}/bconsole
%{_mandir}/man8/bacula-fd.8.%{manpage_ext}
%{_mandir}/man8/bacula.8.%{manpage_ext}
%{_mandir}/man8/bconsole.8.%{manpage_ext}
%{_mandir}/man8/btraceback.8.%{manpage_ext}
%{_mandir}/man1/bat.1.%{manpage_ext}
%_prefix/share/doc/*

%pre client
# create the daemon group and user
HAVE_BACULA=`grep %{daemon_group} %{group_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
    %{groupadd} -r %{daemon_group} > /dev/null 2>&1
    echo "The group %{daemon_group} has been added to %{group_file}."
    echo "See the manual chapter \"Running Bacula\" for details."
fi
# we do not use the -g option allowing the primary group to be set to system default
# this will be a unique group on redhat type systems or the group users on some systems
HAVE_BACULA=`grep %{file_daemon_user} %{user_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
    %{useradd} -r -c "Bacula" -d %{working_dir} -g %{daemon_group} -M -s /sbin/nologin %{file_daemon_user} > /dev/null 2>&1
    echo "The user %{file_daemon_user} has been added to %{user_file}."
    echo "See the manual chapter \"Running Bacula\" for details."
fi
# now we add the supplementary group, this is ok to call even if the user already exists
# we only do this if the user is NOT root
IS_ROOT=%{file_daemon_user}
if [ "$IS_ROOT" != "root" ]; then
%{usermod} -G %{daemon_group} %{file_daemon_user}
fi

%post client
# add our link
if [ "$1" -ge 1 ] ; then
   /sbin/chkconfig --add bacula-fd
fi

if [ -d %{sysconf_dir} ]; then
   cd %{sysconf_dir}
   for string in XXX_REPLACE_WITH_DIRECTOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_PASSWORD_XXX XXX_REPLACE_WITH_DIRECTOR_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_MONITOR_PASSWORD_XXX; do
      pass=`openssl rand -base64 33`
      for file in *.conf; do
         sed "s@${string}@${pass}@g" $file > $file.new
         cp -f $file.new $file; rm -f $file.new
      done
   done
# put actual hostname in conf file
   host=`hostname`
   string="XXX_HOSTNAME_XXX"
   for file in *.conf; do
      sed "s@${string}@${host}@g" $file >$file.new
      cp -f $file.new $file; rm -f $file.new
   done
fi

%preun client
# delete our link
if [ $1 = 0 ]; then
   /sbin/chkconfig --del bacula-fd
fi

%files libs
%defattr(-,root,root)
%{_libdir}/libbac-*
%{_libdir}/libbac.*
%{_libdir}/libbaccfg*
%{_libdir}/libbacfind*
%{_libdir}/libbacpy*

%post libs
/sbin/ldconfig
exit 0

%postun libs
/sbin/ldconfig
exit 0

%if ! %{client_only}
%files updatedb
%defattr(-,root,%{daemon_group})
%{script_dir}/updatedb/*
#oensuse_bs: directories not owned by any package
#%{script_dir}/updatedb

%pre updatedb
# create the daemon group
HAVE_BACULA=`grep %{daemon_group} %{group_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
    %{groupadd} -r %{daemon_group} > /dev/null 2>&1
    echo "The group %{daemon_group} has been added to %{group_file}."
    echo "See the manual chapter \"Running Bacula\" for details."
fi

%post updatedb
echo "The database update scripts were installed to %{script_dir}/updatedb"
%endif

%changelog
* Mon Sep 06 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
* Mon Sep 06 2010 Kern Sibbald <kern@sibbald.com>
- 5.0.3
- change license to AGPL v3
- add defines for postgresql version and packages, require version 8 or greater
- adjusted package description
- fix bug with libbacsql files
* Sun Mar 07 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- remove --without-qwt from configure statement
* Sat Feb 27 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- move shared libraries into bacula-libs package
* Sat Feb 20 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- remove deprecated mysql4 and mysql5 build defines
- add build support for tcpwrappers
- remove bacula_config file for client only build
- remove requirements for database server packages
- enable readline support
- add fc11 and fc12
* Sat Feb 13 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- 5.0.1
- fix client only build
- clean up requirements for termcap
- query.sql now empty by default so we install sample-query.sql along side it
- remove old nobuild_mtx switch
* Sun Feb 07 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix mysql database upgrade
* Fri Feb 05 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix permissions of dbcheck
- misc. cleanup
* Sat Jan 30 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- 5.0.0
- add su112 and fc10 builds, remove doc package declaration.
- fix source path for logwatch files.
- restore static bundled build of sqlite (who took that out?) and remove use
- of system sqlite. we don't support use of sqlite2.
- upgrade database from version 11 to 12.
* Mon Aug 10 2009 Philipp Storz <philipp.storz@dass-it.de>
- changes to work with opensuse build service
* Sat Aug 1 2009 Kern Sibbald <kern@sibbald.com>
- Split docs into separate bacula-docs.spec, bacula-bat.spec and bacula-mtx.spec
- removed build of rescue, gnome console and wxconsole
* Sat Jun 20 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- Fix bat install which is now handled by make and uses shared libs
* Sat May 16 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix libxml dependency for rh7 per Pasi Kärkkäinen <pasik@iki.fi>
* Mon May 04 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- Fix post ldconfig problem in client only build
* Sun May 03 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- remove more files installed by client-only build not needed by client package
- remove libbacsql files from client package
* Sat May 02 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- 3.0.1
- update for new docs configuration
* Sat Apr 25 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add switch to pass Distribution tag
* Sun Apr 5 2009 D. Scott Barninger <barninger@fairfieldcomputers.com>
- 3.0.0 release
- database update version 10 to 11
- make now installs docs so we rm from buildroot
- add shared libs in %_libdir and other misc new files
* Wed Dec 31 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add su111 target
* Sat Nov 08 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- set paths for QT using pkgconfig
* Sat Oct 11 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- release 2.4.3 update depkgs to 11Sep08 remove file nmshack from mtx package
* Sun Sep 07 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- adjust /opt/bacula install
- add build switch to supress rescue package
* Sun Aug 24 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add support for install to /opt/bacula
* Sun Aug 17 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- remove libtermcap dependancy for FC9
* Mon Aug 04 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix bat dependencies
* Sat Jun 28 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add su110 target
* Sat May 24 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add fc9 target
* Sun Mar 30 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- FHS compatibility changes
* Sat Feb 16 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- remove fix for false buffer overflow detection with glibc >= 2.7
* Sat Feb 09 2008 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix for false buffer overflow detection with glibc >= 2.7
