# This file is part of OpenPanel - The Open Source Control Panel
# OpenPanel is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License as published by the Free 
# Software Foundation, using version 3 of the License.
#
# Please note that use of the OpenPanel trademark may be subject to additional 
# restrictions. For more information, please visit the Legal Information 
# section of the OpenPanel website on http://www.openpanel.com/

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
mkdir -p ${BUILD_ROOT}/var/openpanel/modules/Networking.module
cp -rf ./networkingmodule.app ${BUILD_ROOT}/var/openpanel/modules/Networking.module/
ln -sf networkingmodule.app/exec ${BUILD_ROOT}/var/openpanel/modules/Networking.module/action
cp module.xml techsupport.* ${BUILD_ROOT}/var/openpanel/modules/Networking.module/
cp *.png *.html ${BUILD_ROOT}/var/openpanel/modules/Networking.module/
install -m 755 verify ${BUILD_ROOT}/var/openpanel/modules/Networking.module/verify

%post
mkdir -p /var/openpanel/conf/staging/Networking
chown openpanel-core:openpanel-authd /var/openpanel/conf/staging/Networking

%files
%defattr(-,root,root)
/
