#!/bin/bash

# @abrahamgcc
# ./build.sh great,dream2,dream ++DeluxeKernel_v15.2++ N950F_G95xF_DeluxeKernel_REL.zip

KERNEL_NAME="${2}"
ZIP_NAME="${3}"
YELLOW="\e[93m"
GREEN="\e[92m"
RED="\e[91m"
NONE="\e[39m"

time_check() {
	if (( $SECONDS > 3600 )) ; then
		let "hours=SECONDS/3600"
		let "minutes=(SECONDS%3600)/60"
		let "seconds=(SECONDS%3600)%60"
		echo -e "${GREEN}${hours}h.:${minutes}mins.:${seconds}s.${NONE}"
	elif (( $SECONDS > 60 )) ; then
		let "minutes=(SECONDS%3600)/60"
		let "seconds=(SECONDS%3600)%60"
		echo -e "${GREEN}${minutes}mins.:${seconds}s.${NONE}"
	else
		echo -e "${GREEN}${SECONDS}s.${NONE}"
	fi
}

abort() {
  echo -e "\n\n ${RED}$@ ${NONE}"
  exit 1
}

clear
export CROSS_COMPILE=/home/volt/toolchain/gcc-linaro-4.9.4-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
export ARCH=arm64
export ANDROID_MAJOR_VERSION=p
export LOCALVERSION="${KERNEL_NAME}"
[ "$1" == "clear" ] && make clean && make mrproper && clear && exit 0
echo "$1" | tr ',' '\n' | while read device; do
	SECONDS=0
	[ -f arch/arm64/configs/exynos8895-${device}lte_defconfig ] || abort "exynos8895-${device}lte_defconfig DOESN'T EXIST"
	echo -e "${YELLOW} CLEANING SOURCES ..."
	make clean && make mrproper && clear
	echo " PREPARING CONFIGURATION ..."
	make exynos8895-${device}lte_defconfig && clear
	echo " BUILDING ${KERNEL_NAME} FOR ${device}lte ..."
	make -j64 && clear
	cp -rf arch/arm64/boot/dtb.img /home/volt/Downloads/greatlte/dtb_greatlte.img
	cp -rf arch/arm64/boot/Image /home/volt/Downloads/greatlte/Image
	echo -e " BUILT ${KERNEL_NAME}_${device}lte.img in $(time_check). ${NONE}"
done
[[ -z "${ZIP_NAME}" || $(ls deluxe/*.img | wc -l) != "3" ]] && abort "MISSING ZIP NAME OR KERNELS NOT FOUND [ALL VARIANTS ARE NEEDED FOR MAKE THE ZIP]"
cd deluxe
for i in great dream2 dream; do
	mv *${i}lte.img ${i}lte.img
done
echo -e " COMPRESSING kernels... ${NONE}"
tar -cJf kernel.tar.xz greatlte.img dream2lte.img dreamlte.img
mv kernel.tar.xz zip/deluxe/kernel.tar.xz
cd zip
zip -r -9 "${ZIP_NAME}".zip META-INF deluxe
rm -rf ../"${ZIP_NAME}".zip
mv "${ZIP_NAME}".zip ../"${ZIP_NAME}".zip
rm -rf deluxe/kernel.tar.xz
cd ../../
rm -rf deluxe/*.img
clear
echo -e " BUILT ${ZIP_NAME}.zip. ${NONE}"
