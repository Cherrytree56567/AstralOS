cmake_minimum_required(VERSION 3.10)
project(AstralOSBootloader)

# Set up directories
set(BUILD_DIR "gnu-efi")
set(SRC_DIR "src")
set(BIN_DIR "bin")
set(BOOTLOADER_DIR "${BUILD_DIR}/src")

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
    COMMAND cd ${BUILD_DIR} && make && mkdir -p src && cd src && mkdir -p out && cd ../../
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Bootloader"
)

add_custom_target(build
    COMMAND cp ${SRC_DIR}/* ${BOOTLOADER_DIR}
    COMMAND cd ${BUILD_DIR} && gcc -Iinc -Iinc/x86_64 -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c src/main.c -o src/out/main.o
    COMMAND cd ${BUILD_DIR} && ld -shared -Bsymbolic -Lx86_64/lib -Lx86_64/gnuefi -Tgnuefi/elf_x86_64_efi.lds x86_64/gnuefi/crt0-efi-x86_64.o src/out/main.o -o main.so -lgnuefi -lefi
    COMMAND cd ${BUILD_DIR} && objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi
    COMMAND mkdir -p ${BIN_DIR} && mkdir -p ${BIN_DIR}/EFI && mkdir -p ${BIN_DIR}/EFI/BOOT && mkdir -p ${BIN_DIR}/EFI/AstralBoot && cp ${BUILD_DIR}/main.efi ${BIN_DIR}/EFI/BOOT/BOOTX64.EFI && cp EFIDRIVERS/* ${BIN_DIR}/EFI/AstralBoot/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding Bootloader"
)

add_custom_target(_clean
    COMMAND rm -rf ${BIN_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding Bootloader"
)
