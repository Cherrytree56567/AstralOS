cmake_minimum_required(VERSION 3.10)
project(AstralOSWEB)

execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.Web\" ]]; then exit 0; else echo 'Please go inside AstralOS.Web'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.Web")
endif()

set(OBJ_FILES)

add_custom_target(init
    COMMAND sudo apt update
    COMMAND sudo apt -y install git npm nodejs
    COMMAND git clone https://github.com/emscripten-core/emsdk.git
	COMMAND cd emsdk && sudo chmod +x emsdk && sudo chmod +x emsdk_env.sh
    COMMAND cd emsdk && ./emsdk install latest && ./emsdk activate latest && ./emsdk_env.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing Emscripten SDK"
)

add_custom_target(build
    COMMAND mkdir -p tmp
    COMMAND cp -f ../AstralOS.Bin/bin/AstralOS.qcow2 tmp/
	COMMAND cd emsdk && chmod +x emsdk_env.sh
    COMMAND bash -c "source ./emsdk/emsdk_env.sh && PWD && ./emsdk/upstream/emscripten/tools/file_packager.py load-rootfs.data --lz4 --preload tmp > load-rootfs.js"
    COMMAND mv -f load-rootfs.data src/image
    COMMAND mv -f load-rootfs.js src/image
    COMMAND sed -i '114i Module.FS.mkdir("/writableh"); const varsData = Module.FS.readFile("/tmp/AstralOS.qcow2", { encoding: "binary" }); Module.FS.writeFile("/writableh/AstralOS.qcow2", varsData, { encoding: "binary" });' src/image/load-rootfs.js
	COMMAND npm install @catppuccin/tailwindcss
    COMMAND npm run build
    COMMAND cp -r dist AstralOS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Website"
)

add_custom_target(_clean
	COMMAND rm -rf tmp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning Website"
)
