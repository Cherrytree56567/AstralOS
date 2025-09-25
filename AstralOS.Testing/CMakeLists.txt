cmake_minimum_required(VERSION 3.10)
project(AstralOSTesting)

add_custom_target(init
    COMMAND sudo apt update
    COMMAND sudo apt install -y qemu-utils qemu-system ovmf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Testing"
)

add_custom_target(build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Testing"
)

add_custom_target(run
    COMMAND cp ../AstralOS.Bin/bin/AstralOS.qcow2 AstralOS.qcow2 && sudo qemu-system-x86_64 -machine q35 -cpu qemu64 -m 4G -drive file=AstralOS.qcow2 -drive if=pflash,format=raw,unit=0,file="../OVMFbin/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="../OVMFbin/OVMF_VARS-pure-efi.fd" -net none -serial file:qemu.log
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Testing"
)

add_custom_target(_clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning Testing"
)
