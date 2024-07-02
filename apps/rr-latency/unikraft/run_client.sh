#!/bin/bash

eval qemu-system-x86_64 \
	-nographic \
	-vga none \
	-net none \
	-kernel $(dirname $0)/build/unikraft_qemu-x86_64 \
	-enable-kvm \
	-cpu host,migratable=no \
	-netdev tap,id=tap1,script=$(dirname $0)/qemu-ifup.sh,downscript=no,vhost=on \
        -device virtio-net-pci,netdev=tap1,mac=52:54:00:12:34:57 \
        -append \"netdev.ipv4_addr=10.0.0.2 netdev.ipv4_gw_addr=10.0.0.254 netdev.ipv4_subnet_mask=255.255.255.0 -- -c "$@"\"
