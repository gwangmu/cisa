# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gwangmu/Projects/Riskier/cisa

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gwangmu/Projects/Riskier/cisa/build

# Include any dependencies generated for this target.
include extern/irdump/CMakeFiles/IRDumper.dir/depend.make

# Include the progress variables for this target.
include extern/irdump/CMakeFiles/IRDumper.dir/progress.make

# Include the compile flags for this target's objects.
include extern/irdump/CMakeFiles/IRDumper.dir/flags.make

extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.o: extern/irdump/CMakeFiles/IRDumper.dir/flags.make
extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.o: ../extern/irdump/dumper.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gwangmu/Projects/Riskier/cisa/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.o"
	cd /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump && /home/gwangmu/Projects/Riskier/llvm/install/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/IRDumper.dir/dumper.cpp.o -c /home/gwangmu/Projects/Riskier/cisa/extern/irdump/dumper.cpp

extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/IRDumper.dir/dumper.cpp.i"
	cd /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump && /home/gwangmu/Projects/Riskier/llvm/install/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gwangmu/Projects/Riskier/cisa/extern/irdump/dumper.cpp > CMakeFiles/IRDumper.dir/dumper.cpp.i

extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/IRDumper.dir/dumper.cpp.s"
	cd /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump && /home/gwangmu/Projects/Riskier/llvm/install/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gwangmu/Projects/Riskier/cisa/extern/irdump/dumper.cpp -o CMakeFiles/IRDumper.dir/dumper.cpp.s

# Object files for target IRDumper
IRDumper_OBJECTS = \
"CMakeFiles/IRDumper.dir/dumper.cpp.o"

# External object files for target IRDumper
IRDumper_EXTERNAL_OBJECTS =

lib/libIRDumper.so: extern/irdump/CMakeFiles/IRDumper.dir/dumper.cpp.o
lib/libIRDumper.so: extern/irdump/CMakeFiles/IRDumper.dir/build.make
lib/libIRDumper.so: extern/irdump/CMakeFiles/IRDumper.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gwangmu/Projects/Riskier/cisa/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library ../../lib/libIRDumper.so"
	cd /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/IRDumper.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
extern/irdump/CMakeFiles/IRDumper.dir/build: lib/libIRDumper.so

.PHONY : extern/irdump/CMakeFiles/IRDumper.dir/build

extern/irdump/CMakeFiles/IRDumper.dir/clean:
	cd /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump && $(CMAKE_COMMAND) -P CMakeFiles/IRDumper.dir/cmake_clean.cmake
.PHONY : extern/irdump/CMakeFiles/IRDumper.dir/clean

extern/irdump/CMakeFiles/IRDumper.dir/depend:
	cd /home/gwangmu/Projects/Riskier/cisa/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gwangmu/Projects/Riskier/cisa /home/gwangmu/Projects/Riskier/cisa/extern/irdump /home/gwangmu/Projects/Riskier/cisa/build /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump /home/gwangmu/Projects/Riskier/cisa/build/extern/irdump/CMakeFiles/IRDumper.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : extern/irdump/CMakeFiles/IRDumper.dir/depend

