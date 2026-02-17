cmake_minimum_required(VERSION 3.10)
project(AstralOS)

add_custom_target(init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bootloader && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Kernel && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.KernelDrivers && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bin && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Testing && cmake . && cmake --build . --target init
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing all subprojects"
)

add_custom_target(build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bootloader && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Kernel && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.KernelDrivers && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bin && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Testing && cmake . && cmake --build . --target build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building all subprojects"
)

add_custom_target(initweb
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Web && cmake . && cmake --build . --target init
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Website"
)

add_custom_target(buildweb
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Web && cmake . && cmake --build . --target build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Website"
)

add_custom_target(run
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Testing && cmake . && cmake --build . --target run
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running all subprojects"
)

add_custom_target(_clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bootloader && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Kernel && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.KernelDrivers && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Bin && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Testing && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.Web && cmake . && cmake --build . --target _clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning all subprojects"
)
