# SURE: Secure Unikernels Make Serverless Computing Rapid and Efficient

## Building and running

All code blocks suppose to be executed from a common working directory.

### Building SURE apps

Install pre-requirements:
```bash
sudo apt install -y python3-pip meson libglib2.0-dev libssl-dev libnuma-dev libncurses-dev flex bison libibverbs-dev

pip3 install pyelftools
```

Clone SURE repo:
```bash
git clone https://github.com/FedeParola/sure.git
cd sure
git submodule update --init --recursive
```

Patch, build and install QEMU:
```bash
git clone https://github.com/qemu/qemu.git
cd qemu
git checkout v8.2.0
git apply ../sure/ivshmem_patch.diff
mkdir build
cd build
../configure --target-list=x86_64-softmmu
make -j
sudo make install
```

Build and install Z-stack:
```bash
git clone https://github.com/FedeParola/f-stack.git

cd f-stack/dpdk
meson build
ninja -C build
sudo ninja -C build install

cd ../lib
make -j
sudo make install
```

Bind a network interface to DPDK driver (**not needed for Mellanox NICs**):
```bash
sudo modprobe uio
sudo insmod f-stack/dpdk/build/kernel/linux/igb_uio/igb_uio.ko
sudo ip link set <ifname> down
sudo dpdk-devbind.py -b igb_uio <ifname>
```

Build Unimsg manager and gateway:
```bash
git clone https://github.com/FedeParola/unimsg.git
cd unimsg/manager/
make
cd ../gateway
make
```

Build the SURE application (e.g., rr-latency):
```bash
cd sure/apps/rr-latency/sure
make menuconfig
# Configure the application
make -j
```

### Running SURE apps

Allocate 2M hugepages:
```bash
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo umount /dev/hugepages
sudo mount -t hugetlbfs -o pagesize=2M none /dev/hugepages
```

Run the manager:
```bash
cd unimsg/manager
sudo ./unimsg_manager
```

Run the gateway:
```bash
cd unimsg/gateway
sudo ./unimsg_gateway
```

Run the SURE VM (e.g., rr-latency app).
`<id>` is an incremental id of the VM starting from 1.
The address of the VM will be computed as `10.0.0.<id>`.
```bash
cd sure/apps/rr-latency/sure
sudo ./run.sh <id> <args>
```

## Tuning the nodes

To prevent CPU C-states and P-states form affecting measurements, the following tuning can be applied.
Following instructions were tested on a CloudLab `sm110p` node, but should apply to all recent Intel CPUs.

### Disable P-states (frequency scaling)
Edit `/etc/default/grub` and append `intel_pstate=passive` to the `GRUB_CMDLINE_LINUX` option, then apply and reboot the machine:
```bash
sudo update-grub2
sudo reboot
```
Disable turbo-boost:
```bash
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```
Set all cores to max frequency:
```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Disable C-states (idle states)
```bash
echo 1 | sudo tee /sys/devices/system/cpu/cpu*/cpuidle/state*/disable
```
