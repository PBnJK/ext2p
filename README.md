# ext2p: An ext2 filesystem emulator

This is a small utility for interacting with ext2 filesystems. It allows you to
navigate and visualize folders and files, as well as dump information on the fs
itself. Soon, it will also allow modification, creation, and deletion of files!

## Usage
For now, the tool must be used in conjunction with an existing ext2 filesystem,
but an example one is provided in this repository (`example.img`.) You may also
generate one using a tool such as `mke2fs`.

You may run it as follows:
```sh
$ ext2p path/to/filesystem
```

You can then type "help" to see the available commands.

## Building
This tool uses CMake to build. You can build it as follows:
```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## References
The following references where used during the development of this tool:

- [The Second Extended File System, by Dave Poirier](https://www.nongnu.org/ext2-doc/);
- [The linux ext2.h header](https://github.com/torvalds/linux/blob/master/fs/ext2/ext2.h);
- [Ext2, by Willian Oliveira](https://willianoliveira.blog/ext2/);
- [The Ext2 Wikipedia page](https://en.wikipedia.org/wiki/Ext2);
- [ext4 Data Structures and Algorithms](https://www.kernel.org/doc/html/latest/filesystems/ext4/),
  which, although focused on the later ext4 filesystem, still contains information
  relevant to the earlier ext2 filesystem.
