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
    COMMAND cp ../AstralOS.Bin/bin/AstralOS.qcow2 AstralOS.qcow2 && sudo qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd -drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_VARS_4M.fd -drive file=AstralOS.qcow2,format=raw
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Testing"
)

add_custom_target(_clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning Testing"
)
