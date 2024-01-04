#!/bin/bash

if [ -z $1 ]; then
	echo "usage: $0 <vm_id> <app_options>"
	exit 1
fi

id=$1
shift

eval $(dirname $0)/../../../../qemu/build/qemu-system-x86_64 \
	-nographic \
	-vga none \
	-net none \
	-kernel $(dirname $0)/build/unikraft_qemu-x86_64 \
	-enable-kvm \
	-cpu host \
	-m 1024 \
	-object memory-backend-file,id=mem,size=1024M,mem-path=/dev/hugepages,share=on \
	-mem-prealloc \
	-numa node,memdev=mem \
	-chardev socket,id=char1,path=/usr/local/var/run/openvswitch/vhost-user-$id \
	-netdev vhost-user,id=hostnet1,chardev=char1,vhostforce=on \
	-device virtio-net-pci,netdev=hostnet1,id=net1,mac=52:54:00:12:34:5$id \
	-append \"netdev.ipv4_addr=10.0.0.$id netdev.ipv4_gw_addr=10.0.0.254 netdev.ipv4_subnet_mask=255.255.255.0 -- "$@"\"