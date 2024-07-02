#!/bin/bash

eval qemu-system-x86_64 \
	-m 8M \
	-nographic \
	-vga none \
	-net none \
	-kernel "$(dirname $0)/build/sure_qemu-x86_64" \
	-enable-kvm \
	-cpu host,migratable=no \
	-device ivshmem-doorbell,vectors=1,chardev=id \
	-chardev socket,path=/tmp/ivshmem_socket,id=id \
	-object memory-backend-file,size=4K,share=true,mem-path=/dev/shm/unimsg_sidecar_0,id=sidecar_mem \
	-device ivshmem-plain,memdev=sidecar_mem \
        -append \""$@"\"
