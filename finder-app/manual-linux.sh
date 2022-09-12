#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
    echo "Using default directory ${OUTDIR} for output"
else
    OUTDIR=$1
    echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
    echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here

    # Steps from ECEA-5305 course slide "Building the Linux Kernel"
    echo "1) \"deep\" clean the kernel built tree - removing the .config file with any existing configurations"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    echo "2) Configure for our \"virt\" arm dev board we will simulate in QEMU"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "3) Build a kernel image for booting with QEMU"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    echo "4) Build any kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    echo "5) Build the devicetree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

    # DEBUG CODE - FGREEN end of TODO

fi

echo "6) Adding the Image in outdir"
cp -rv ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}


echo "7) Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs

if ! [ -d "${OUTDIR}/rootfs" ] ; then
{
    echo "ERROR: Failed to make ${OUTDIR}/rootfs"
    exit 1
}
fi

cd ${OUTDIR}/rootfs
mkdir bin dev etc home lib proc sbin sys tmp lib64
mkdir -p usr/bin usr/lib usr/sbin var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "8) Configure Busybox"
    # Steps from ECEA-5305 course slide "Linux Root File Systems"
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
# Steps from ECEA-5305 course slide "Linux Root File Systems"
echo "10) Make and install busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ${OUTDIR}/rootfs

echo "11) Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "12) Adding library dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot -v)
sudo cp -a ${SYSROOT}/lib/* lib
sudo cp -a ${SYSROOT}/lib64 .


# TODO: Make device nodes
echo "13) Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1


# TODO: Clean and build the writer utility
echo "14) Clean and build the writer utility"
cd ${FINDER_APP_DIR}
make clean CROSS_COMPILE=${CROSS_COMPILE} all

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "14) Copy finder related scripts and executables to the \/home directory"
cd ${FINDER_APP_DIR}
cp -at ${OUTDIR}/rootfs/home writer autorun-qemu.sh finder-test.sh finder.sh
cp -art ${OUTDIR}/rootfs/home conf/
mkdir -p ${OUTDIR}/rootfs/home/conf
cp -at ${OUTDIR}/rootfs/home/conf conf/username.txt


# TODO: Chown the root directory
echo "15) chown the root directory"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
echo "16) Create initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio
