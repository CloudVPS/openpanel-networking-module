#!/bin/sh
if grep ^CXXFLAGS makeinclude | grep -q LINUX_REDHAT ; then
 cp rsrc/openpanel.module.networking.conf.xml.redhat rsrc/openpanel.module.networking.conf.xml
elif grep ^CXXFLAGS makeinclude | grep -q LINUX_DEBIAN ; then
 cp rsrc/openpanel.module.networking.conf.xml.debian  rsrc/openpanel.module.networking.conf.xml
else
 echo "No compatible environment found."
 exit 1
fi
