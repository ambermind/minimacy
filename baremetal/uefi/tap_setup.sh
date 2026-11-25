#!/bin/sh
# Networking for QEMU: tap interface + bridge + NAT to the Internet + DHCP (dnsmasq)
#
# Usage: sudo ./qemu-net.sh up       create the network
#        sudo ./qemu-net.sh down     remove it
#        ./qemu-net.sh status        show state and DHCP leases
#
# Requires: iproute2, iptables, dnsmasq (Debian/Ubuntu: apt install dnsmasq-base)
#
# QEMU side:
#   -netdev tap,id=n0,ifname=tap0,script=no,downscript=no -device virtio-net-pci,netdev=n0

set -e

TAP=tap0
BR=br0
NET=192.168.2                 # VM subnet (/24)
GW=$NET.1                     # host address on the bridge: gateway and DNS for the VMs
DHCP_FIRST=$NET.10
DHCP_LAST=$NET.100
GROUP=1000                    # group allowed to use tap0 without root
LEASES=/tmp/dnsmasq-$BR.leases
PIDFILE=/tmp/dnsmasq-$BR.pid

# Uplink interface to the Internet: the one carrying the default route (eth0, wlan0...)
WAN=$(ip route show default | awk '{print $5; exit}')

# Append an iptables rule only if it is not already present: ipt <table> <rule...>
ipt()  { t=$1; shift; iptables -t "$t" -C "$@" 2>/dev/null || iptables -t "$t" -A "$@"; }
# Delete an iptables rule if it exists: iptd <table> <rule...>
iptd() { t=$1; shift; iptables -t "$t" -D "$@" 2>/dev/null || true; }

rules() {   # $1 = ipt (add) or iptd (delete)
    # NAT: VMs reach the Internet using the host's address
    $1 nat    POSTROUTING -s $NET.0/24 -o "$WAN" -j MASQUERADE
    # Forwarding: VMs may open connections, the Internet may only reply
    $1 filter FORWARD -i $BR -o "$WAN" -j ACCEPT
    $1 filter FORWARD -i "$WAN" -o $BR -m state --state RELATED,ESTABLISHED -j ACCEPT
    # Allow DHCP and DNS to dnsmasq, in case a firewall drops incoming traffic
    $1 filter INPUT -i $BR -p udp --dport 67 -j ACCEPT
    $1 filter INPUT -i $BR -p udp --dport 53 -j ACCEPT
    $1 filter INPUT -i $BR -p tcp --dport 53 -j ACCEPT
}

up() {
    if [ -z "$WAN" ]; then echo "No default route: cannot find the Internet interface"; exit 1; fi
    if ! command -v dnsmasq >/dev/null 2>&1; then echo "dnsmasq not found (apt install dnsmasq-base)"; exit 1; fi
    if ip link show $BR >/dev/null 2>&1; then echo "$BR already exists (run first: $0 down)"; exit 1; fi

    # Tap interface usable by group $GROUP
    ip tuntap add dev $TAP mode tap group $GROUP
    ip link set $TAP up

    # Bridge the tap is attached to; the host gets address $GW on it
    ip link add $BR type bridge
    echo 0 > /sys/class/net/$BR/bridge/stp_state
    ip link set $TAP master $BR
    ip addr add $GW/24 dev $BR
    ip link set $BR up

    # Routing and firewall
    sysctl -q net.ipv4.ip_forward=1
    rules ipt

    # DHCP server + DNS forwarder, on the bridge only
    dnsmasq --interface=$BR --bind-interfaces --except-interface=lo \
            --dhcp-range=$DHCP_FIRST,$DHCP_LAST,255.255.255.0,12h \
            --dhcp-option=option:router,$GW \
            --dhcp-option=option:dns-server,$GW \
            --dhcp-leasefile=$LEASES \
            --pid-file=$PIDFILE

    echo "Network ready: $TAP on $BR ($GW/24), uplink $WAN, DHCP $DHCP_FIRST-$DHCP_LAST"
}

down() {
    [ -f $PIDFILE ] && kill "$(cat $PIDFILE)" 2>/dev/null || true
    rm -f $PIDFILE $LEASES
    [ -n "$WAN" ] && rules iptd
    ip link del $BR 2>/dev/null || true
    ip tuntap del dev $TAP mode tap 2>/dev/null || true
    # ip_forward is left at 1: other services (Docker...) may depend on it
    echo "Network removed"
}

status() {
    ip -br addr show $BR 2>/dev/null || echo "$BR missing"
    ip -br link show $TAP 2>/dev/null || echo "$TAP missing"
    if [ -f $PIDFILE ] && kill -0 "$(cat $PIDFILE)" 2>/dev/null; then
        echo "dnsmasq running (pid $(cat $PIDFILE))"
    else
        echo "dnsmasq stopped"
    fi
    echo "DHCP leases:"
    cat $LEASES 2>/dev/null || echo "  (none)"
}

case "$1" in
    up)     up ;;
    down)   down ;;
    status) status ;;
    *)      echo "Usage: $0 up|down|status"; exit 1 ;;
esac