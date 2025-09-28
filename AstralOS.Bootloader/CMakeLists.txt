cmake_minimum_required(VERSION 3.10)
project(AstralOSBootloader)

# Set up directories
set(BUILD_DIR "gnu-efi")
set(SRC_DIR "src")
set(BIN_DIR "bin")
set(BOOTLOADER_DIR "${BUILD_DIR}/bootloader")

# Check directory rule
execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.Bootloader\" ]]; then exit 0; else echo 'Please go inside AstralOS.Bootloader'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.Bootloader")
endif()

add_custom_target(init
    COMMAND sudo apt update
    COMMAND sudo apt -y install build-essential mtools git gcc make
    COMMAND git submodule init
    COMMAND git submodule update
    COMMAND cd ${BUILD_DIR} && cd bootloader && rm -rf main.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Bootloader"
)

add_custom_target(build
    COMMAND cd ${BUILD_DIR} && find . -mindepth 1 ! -name 'Makefile' -exec rm -rf {} +
    COMMAND cp -r ${SRC_DIR}/* ${BOOTLOADER_DIR}
    COMMAND cd ${BUILD_DIR} && make
    COMMAND cd ${BUILD_DIR} && make bootloader
    COMMAND cd ${BUILD_DIR} && objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi
    COMMAND mkdir -p ${BIN_DIR} && mkdir -p ${BIN_DIR}/EFI && mkdir -p ${BIN_DIR}/EFI/BOOT && mkdir -p ${BIN_DIR}/EFI/AstralBoot && cp ${BUILD_DIR}/main.efi ${BIN_DIR}/EFI/BOOT/BOOTX64.EFI && cp EFIDRIVERS/* ${BIN_DIR}/EFI/AstralBoot/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Bootloader"
)

add_custom_target(_clean
    COMMAND rm -rf ${BIN_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding Bootloader"
)
