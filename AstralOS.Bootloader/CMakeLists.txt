cmake_minimum_required(VERSION 3.10)
project(AstralOSBootloader)

set(BUILD_DIR "gnu-efi")
set(SRC_DIR "src")
set(BIN_DIR "bin")
set(BOOTLOADER_DIR "${BUILD_DIR}/src")

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
    COMMAND cd ${BUILD_DIR} && make && mkdir -p src/out && cd ../../
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Bootloader"
)

add_custom_command(
    OUTPUT ${BUILD_DIR}/copied.stamp
    COMMAND mkdir -p ${BOOTLOADER_DIR}
    COMMAND rm -rf ${BOOTLOADER_DIR}/*
    COMMAND rm -rf ${BUILD_DIR}/main.so ${BUILD_DIR}/main.efi
    COMMAND mkdir -p ${BOOTLOADER_DIR}/out
    COMMAND cp -r src/* ${BOOTLOADER_DIR}/
    COMMAND touch ${BUILD_DIR}/copied.stamp
)
add_custom_target(copy_files ALL
    DEPENDS ${BUILD_DIR}/copied.stamp
)

set(C_FILES "${BOOTLOADER_DIR}/main.c;${BOOTLOADER_DIR}/paging.c;${BOOTLOADER_DIR}/elf.c;${BOOTLOADER_DIR}/Utils.c;${BOOTLOADER_DIR}/pfa.c")
set(CPP_FILES "")

set(OBJ_FILES "")

function(add_obj src_file compiler)
    get_filename_component(file_name ${src_file} NAME)
    set(obj_file "${BUILD_DIR}/src/out/${file_name}.o")
    list(APPEND OBJ_FILES ${obj_file})
    add_custom_command(
        OUTPUT ${obj_file}
        COMMAND mkdir -p ${BUILD_DIR}/src/out
        COMMAND ${compiler} -Ignu-efi/inc -Ignu-efi/inc/x86_64 -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c ${src_file} -o ${obj_file}
        DEPENDS ${src_file} ${BUILD_DIR}/copied.stamp
    )
    set(OBJ_FILES ${OBJ_FILES} PARENT_SCOPE)
endfunction()

foreach(src_file ${C_FILES})
    add_obj(${src_file} gcc)
endforeach()

foreach(src_file ${CPP_FILES})
    add_obj(${src_file} g++)
endforeach()

set(SO_FILE "${BUILD_DIR}/main.so")
add_custom_command(
    OUTPUT ${SO_FILE}
    COMMAND ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi
            -Tgnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o
            ${OBJ_FILES} -o ${SO_FILE} -lgnuefi -lefi
    DEPENDS ${OBJ_FILES}
)

set(EFI_FILE "${BUILD_DIR}/main.efi")
add_custom_command(
    OUTPUT ${EFI_FILE}
    COMMAND objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym
            -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc
            --target efi-app-x86_64 --subsystem=10 ${SO_FILE} ${EFI_FILE}
    DEPENDS ${SO_FILE}
)

add_custom_target(build ALL
    DEPENDS copy_files ${EFI_FILE}
    COMMAND mkdir -p ${BIN_DIR}/EFI/BOOT
    COMMAND mkdir -p ${BIN_DIR}/EFI/AstralBoot
    COMMAND cp ${EFI_FILE} ${BIN_DIR}/EFI/BOOT/BOOTX64.EFI
    COMMAND cp EFIDRIVERS/* ${BIN_DIR}/EFI/AstralBoot/
    COMMAND cd gnu-efi && rm -rf copied.stamp
)

add_custom_target(_clean
    COMMAND rm -rf ${BIN_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
