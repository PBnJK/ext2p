# CMake build

cmake_minimum_required(VERSION 3.28)
project(
	ext2p
	VERSION 0.1
	LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
	ext2p
	"src/bg.c"
	"src/bgdescriptor.c"
	"src/dir.c"
	"src/disk.c"
	"src/ext2.c"
	"src/ext2dump.c"
	"src/inode.c"
	"src/main.c"
	"src/shell.c"
	"src/superblock.c"
	"src/util.c"
)

target_include_directories(ext2p PRIVATE ${PROJECT_SOURCE_DIR}/inc)

target_compile_options(ext2p PRIVATE -std=c99 -Wall -Wextra -pedantic)
target_link_options(ext2p PRIVATE -fsanitize=address)
