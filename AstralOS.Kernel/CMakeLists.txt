cmake_minimum_required(VERSION 3.10)
project(AstralOSKernel)

set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(OBJ_DIR "${CMAKE_SOURCE_DIR}/obj")

set(CC "g++")
set(LD "ld")
set(CFLAGS "-ffreestanding" "-fshort-wchar")

file(GLOB_RECURSE  SRC_FILES "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp")

execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.Kernel\" ]]; then exit 0; else echo 'Please go inside AstralOS.Kernel'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.Kernel")
endif()

set(OBJ_FILES)

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
    COMMAND sudo apt -y install build-essential mtools git gcc make
    COMMAND mkdir -p ${BIN_DIR} && mkdir -p ${OBJ_DIR} && mkdir -p ${BIN_DIR}/AstralOS && mkdir -p ${BIN_DIR}/AstralOS/System64
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Kernel"
)

add_custom_target(build
    DEPENDS ${OBJ_FILES}
    COMMAND ${LD} -static -Bsymbolic -nostdlib -Ttext 0x1000 -o ${OBJ_DIR}/kernel.elf ${OBJ_FILES}
    COMMAND cp ${OBJ_DIR}/kernel.elf ${BIN_DIR}/AstralOS/System64/kernel.elf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Kernel: ${LD} ${LDFLAGS} -o ${OBJ_DIR}/kernel.elf ${OBJ_FILES}"
)

add_custom_target(_clean
	COMMAND rm -rf ${BIN_DIR} && rm -rf ${OBJ_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning Kernel"
)
