# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/d/OS/AstralOS.Kernel

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/OS/AstralOS.Kernel

# Include any dependencies generated for this target.
include CMakeFiles/obj_lib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/obj_lib.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/obj_lib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/obj_lib.dir/flags.make

CMakeFiles/obj_lib.dir/src/main.cpp.o: CMakeFiles/obj_lib.dir/flags.make
CMakeFiles/obj_lib.dir/src/main.cpp.o: src/main.cpp
CMakeFiles/obj_lib.dir/src/main.cpp.o: CMakeFiles/obj_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/OS/AstralOS.Kernel/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/obj_lib.dir/src/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/obj_lib.dir/src/main.cpp.o -MF CMakeFiles/obj_lib.dir/src/main.cpp.o.d -o CMakeFiles/obj_lib.dir/src/main.cpp.o -c /mnt/d/OS/AstralOS.Kernel/src/main.cpp

CMakeFiles/obj_lib.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/obj_lib.dir/src/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/d/OS/AstralOS.Kernel/src/main.cpp > CMakeFiles/obj_lib.dir/src/main.cpp.i

CMakeFiles/obj_lib.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/obj_lib.dir/src/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/d/OS/AstralOS.Kernel/src/main.cpp -o CMakeFiles/obj_lib.dir/src/main.cpp.s

obj_lib: CMakeFiles/obj_lib.dir/src/main.cpp.o
obj_lib: CMakeFiles/obj_lib.dir/build.make
.PHONY : obj_lib

# Rule to build all files generated by this target.
CMakeFiles/obj_lib.dir/build: obj_lib
.PHONY : CMakeFiles/obj_lib.dir/build

CMakeFiles/obj_lib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/obj_lib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/obj_lib.dir/clean

CMakeFiles/obj_lib.dir/depend:
	cd /mnt/d/OS/AstralOS.Kernel && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/OS/AstralOS.Kernel /mnt/d/OS/AstralOS.Kernel /mnt/d/OS/AstralOS.Kernel /mnt/d/OS/AstralOS.Kernel /mnt/d/OS/AstralOS.Kernel/CMakeFiles/obj_lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/obj_lib.dir/depend

