%define version 0.9.0

%define libpath /usr/lib
%ifarch x86_64
  %define libpath /usr/lib64
%endif

Summary: Openpanel network configuration module
Name: openpanel-mod-networking
Version: %version
Release: 1
License: GPLv2
Group: Development
Source: http://packages.openpanel.com/archive/openpanel-mod-networking-%{version}.tar.gz
Patch1: openpanel-mod-networking-00-makefile
BuildRoot: /var/tmp/%{name}-buildroot
Requires: openpanel-core >= 0.8.3

%description
Openpanel network configuration module
Network management module for openpanel

%prep
%setup -q -n openpanel-mod-networking-%version
%patch1 -p0 -b .buildroot

%build
BUILD_ROOT=$RPM_BUILD_ROOT
./configure
make

%install
BUILD_ROOT=$RPM_BUILD_ROOT
rm -rf ${BUILD_ROOT}
mkdir -p ${BUILD_ROOT}/var/opencore/modules/Networking.module
cp -rf ./networkingmodule.app ${BUILD_ROOT}/var/opencore/modules/Networking.module/
ln -sf networkingmodule.app/exec ${BUILD_ROOT}/var/opencore/modules/Networking.module/action
cp module.xml techsupport.* ${BUILD_ROOT}/var/opencore/modules/Networking.module/
cp *.png *.html ${BUILD_ROOT}/var/opencore/modules/Networking.module/
install -m 755 verify ${BUILD_ROOT}/var/opencore/modules/Networking.module/verify

%post
mkdir -p /var/opencore/conf/staging/Networking
chown opencore:authd /var/opencore/conf/staging/Networking

%files
%defattr(-,root,root)
/
