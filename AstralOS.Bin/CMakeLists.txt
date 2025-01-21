cmake_minimum_required(VERSION 3.10)
project(AstralOSBin)

# Set up directories
set(QCOW2_FILE "AstralOS.qcow2")
set(QCOW2_SIZE "10G")
set(MOUNT_POINT1 "MountEFI")
set(MOUNT_POINT2 "MountRoot")
set(EFI_SIZE "100MB")

# Check directory rule
execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.Bin\" ]]; then exit 0; else echo 'Please go inside AstralOS.Bin'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.Bin")
endif()

add_custom_target(init
    COMMAND sudo apt update
    COMMAND sudo apt install -y parted dosfstools mtools qemu-utils automake libguestfs-tools
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Binaries"
)

add_custom_target(build
    COMMAND mkdir -p bin
    COMMAND cd bin && qemu-img create -f qcow2 ${QCOW2_FILE} ${QCOW2_SIZE}
    COMMAND cd bin && sudo modprobe nbd && sudo qemu-nbd --connect=/dev/nbd0 ${QCOW2_FILE} && sleep 2
    COMMAND cd bin && sudo parted /dev/nbd0 --script mklabel gpt mkpart primary fat32 1MB ${EFI_SIZE} set 1 esp on mkpart primary ext2 ${EFI_SIZE} 100%
    COMMAND cd bin && sudo kpartx -a /dev/nbd0 && sleep 2
    COMMAND cd bin && sudo mkfs.vfat -F32 /dev/mapper/nbd0p1
    COMMAND cd bin && sudo mkfs.ext2 /dev/mapper/nbd0p2
    COMMAND cd bin && sudo mkdir -p ${MOUNT_POINT1} ${MOUNT_POINT2}
    COMMAND cd bin && sudo mount /dev/mapper/nbd0p1 ${MOUNT_POINT1} && sudo mount /dev/mapper/nbd0p2 ${MOUNT_POINT2}
    COMMAND cd bin && sudo cp -r ../../AstralOS.Bootloader/bin/* ${MOUNT_POINT1}/
    COMMAND cd bin && sudo cp -r ../../AstralOS.Kernel/bin/* ${MOUNT_POINT2}/
    COMMAND cd bin && sudo umount ${MOUNT_POINT1} && sudo umount ${MOUNT_POINT2}
    COMMAND cd bin && sudo kpartx -d /dev/nbd0 && sudo qemu-nbd --disconnect /dev/nbd0
    COMMAND cd bin && rm -R ${MOUNT_POINT1} ${MOUNT_POINT2}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Binaries"
)

add_custom_target(_clean
	COMMAND rm -R bin
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Cleaning Binaries"
)