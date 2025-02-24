#!/bin/bash

set -e

if [ "$EUID" -ne 0 ]
  then echo "You cannot run this without root privileges :(("
  exit 1
fi

# Create the veth interfaces
ip link add yap0 type veth peer name yap1
ip link set yap0 up
ip link set yap1 up

# Assign them IPs
ip addr add 192.168.1.1/24 dev yap0
ip addr add 192.168.1.2/24 dev yap1

echo "yap0 and yap1 created and configured"
 