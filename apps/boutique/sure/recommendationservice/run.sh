#!/bin/bash

if [ -z $1 ]; then
	echo "usage: $0 <sidecar_id> <app_options>"
	exit 1
fi

id=$1
shift

eval qemu-system-x86_64 \
	-nographic \
	-vga none \
	-net none \
	-kernel "$(dirname $0)/build/recommendationservice_qemu-x86_64" \
	-enable-kvm \
	-cpu host,migratable=no \
	-m 256M \
	-device ivshmem-doorbell,vectors=1,chardev=id \
	-chardev socket,path=/tmp/ivshmem_socket,id=id \
	-object memory-backend-file,size=4K,share=true,mem-path=/dev/shm/unimsg_sidecar_$id,id=sidecar_mem \
	-device ivshmem-plain,memdev=sidecar_mem \
        -append \""$@"\"
