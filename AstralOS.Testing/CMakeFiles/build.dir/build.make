# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

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
CMAKE_SOURCE_DIR = /mnt/d/AstralOS/AstralOS.Testing

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/AstralOS/AstralOS.Testing

# Utility rule file for build.

# Include any custom commands dependencies for this target.
include CMakeFiles/build.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/build.dir/progress.make

build: CMakeFiles/build.dir/build.make
.PHONY : build

# Rule to build all files generated by this target.
CMakeFiles/build.dir/build: build
.PHONY : CMakeFiles/build.dir/build

CMakeFiles/build.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/build.dir/cmake_clean.cmake
.PHONY : CMakeFiles/build.dir/clean

CMakeFiles/build.dir/depend:
	cd /mnt/d/AstralOS/AstralOS.Testing && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/AstralOS/AstralOS.Testing /mnt/d/AstralOS/AstralOS.Testing /mnt/d/AstralOS/AstralOS.Testing /mnt/d/AstralOS/AstralOS.Testing /mnt/d/AstralOS/AstralOS.Testing/CMakeFiles/build.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/build.dir/depend

