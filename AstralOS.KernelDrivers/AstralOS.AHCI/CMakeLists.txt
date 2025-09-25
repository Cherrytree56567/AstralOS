cmake_minimum_required(VERSION 3.10)
project(AstralOSSATAAHCI)

set(BIN_DIR "${CMAKE_SOURCE_DIR}/../bin")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(OBJ_DIR "${CMAKE_SOURCE_DIR}/obj")

set(CC "g++")
set(LD "ld")
set(CFLAGS "-ffreestanding" "-fshort-wchar" "-fno-exceptions" "-fno-rtti" "-fPIC")

set(AS "nasm")
set(ASFLAGS "-felf64")

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE ASM_FILES "${SRC_DIR}/*.asm" "${SRC_DIR}/*.s" "${SRC_DIR}/*.S")

execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.KernelDrivers/AstralOS.AHCI\" ]]; then exit 0; else echo 'Please go inside AstralOS.AHCI'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.AHCI")
endif()

set(OBJ_FILES)

foreach(ASM_FILE ${ASM_FILES})
    get_filename_component(FILE_NAME ${ASM_FILE} NAME_WE)
    set(OBJ_FILE "${OBJ_DIR}/${FILE_NAME}.o")

    list(APPEND OBJ_FILES ${OBJ_FILE})

    add_custom_command(
        OUTPUT ${OBJ_FILE}
        COMMAND ${AS} ${ASFLAGS} ${ASM_FILE} -o ${OBJ_FILE}
        DEPENDS ${ASM_FILE}
        COMMENT "Assembling ${ASM_FILE} to ${OBJ_FILE}"
    )
endforeach()

foreach(SRC_FILE ${SRC_FILES})
    get_filename_component(FILE_NAME ${SRC_FILE} NAME_WE)
    set(OBJ_FILE "${OBJ_DIR}/${FILE_NAME}.o")

    list(APPEND OBJ_FILES ${OBJ_FILE})

    add_custom_command(
        OUTPUT ${OBJ_FILE}
        COMMAND ${CC} ${CFLAGS} -c ${SRC_FILE} -o ${OBJ_FILE}  # Compile individual source files
        DEPENDS ${SRC_FILE}
        COMMENT "Compiling ${SRC_FILE} to ${OBJ_FILE}"
    )
endforeach()

add_custom_target(init
    COMMAND sudo apt update
    COMMAND sudo apt -y install build-essential mtools git gcc make nasm ninja-build
    COMMAND mkdir -p ${OBJ_DIR} && mkdir -p ${BIN_DIR}/Drivers/com.astralos.ahci
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Driver"
)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")

add_custom_target(build
    DEPENDS ${OBJ_FILES}
    COMMAND ${LD} -shared -Bsymbolic -nostdlib -T ${LINKER_SCRIPT} -o ${OBJ_DIR}/driver.elf ${OBJ_FILES}
    COMMAND cp ${OBJ_DIR}/driver.elf ${BIN_DIR}/Drivers/com.astralos.ahci/driver.elf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Driver: ${LD} ${LDFLAGS} -o ${OBJ_DIR}/driver.elf ${OBJ_FILES}"
)

add_custom_target(_clean
	COMMAND rm -rf ${BIN_DIR} && rm -rf ${OBJ_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning Driver"
)
