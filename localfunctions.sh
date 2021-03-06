_getinterfaces() {
    ifaces=$(coreval --loop Networking Networking:Interface)
    echo "${ifaces}"
}

_getroutes() {
    routes=$(coreval --loop Networking Networking:Route)
    echo "${routes}"
}

_getrouteprefix() {
    route=$1
    prefix=$(coreval Networking Networking:Route ${route} prefix)
    echo "${prefix}"
}

_getroutegateway() {
    route=$1
    gateway=$(coreval Networking Networking:Route ${route} gateway)
    echo "${gateway}"
}

_getinterfaceaddresses() {
    iface=$1
    addresses=$(coreval --loop Networking Networking:Interface ${iface} Networking:Address)
    echo "${addresses}"
}

_getinterfaceenabled() {
    iface=$1
    enabled=$(coreval Networking Networking:Interface ${iface} dev_enabled)
    echo "${enabled}"
}

_getinterfacedhcp() {
    iface=$1
    dhcp=$(coreval Networking Networking:Interface ${iface} dhcp_enabled)
    echo "${dhcp}"
}

_getaddressdescription() {
    iface=$1
    address=$2
    desc=$(coreval Networking Networking:Interface ${iface} Networking:Address ${address} description)
    echo "${desc}"
}

_getaddress() {
    iface=$1
    address=$2
    addr=$(coreval Networking Networking:Interface ${iface} Networking:Address ${address} address)
    echo "${addr}"
}

_getaddressnetmask() {
    iface=$1
    address=$2
    nmsk=$(coreval Networking Networking:Interface ${iface} Networking:Address ${address} netmask)
    echo "${nmsk}"
}

_getinterfacedescription() {
    iface=$1
    desc=$(coreval Networking Networking:Interface ${iface} dev_description)
    echo "${desc}"
}

_getinterfacename() {
    iface=$1
    name=$(coreval Networking Networking:Interface ${iface} dev_name)
    echo "${name}"
}
