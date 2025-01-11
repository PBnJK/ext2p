/* ext2p
 * Shell
 */

#include "util.h"
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#include <conio.h>
#else
#define clrscr() fputs("\033[1;1H\033[2J", stdout);
#endif

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "ext2.h"
#include "ext2dump.h"
#include "fault.h"
#include "inode.h"

#include "shell.h"

typedef int (*cmd_fn)(Shell *shell, int argc, char *argv[]);

#define SHELL_FN(N) static int _shell_##N(Shell *shell, int argc, char *argv[])

typedef struct _ShellCommand {
	char *name;
	cmd_fn fn;
} ShellCommand;

SHELL_FN(cat);
SHELL_FN(cd);
SHELL_FN(clear);
SHELL_FN(exit);
SHELL_FN(fsdump);
SHELL_FN(help);
SHELL_FN(ls);
SHELL_FN(stat);

static ShellCommand _shellCommands[] = {
	{ "cat", _shell_cat }, /* displays file contents */
	{ "cd", _shell_cd }, /* changes the current directory */
	{ "clear", _shell_clear }, /* clears the screen */
	{ "cls", _shell_clear }, /* clears the screen */
	{ "dir", _shell_ls }, /* lists a directory's contents */
	{ "exit", _shell_exit }, /* exits the shell */
	{ "fsdump", _shell_fsdump }, /* dumps filesystem info */
	{ "help", _shell_help }, /* prints help information */
	{ "ls", _shell_ls }, /* lists a directory's contents */
	{ "stat", _shell_stat }, /* dumps file info */
};
const int SHELL_CMD_COUNT = sizeof(_shellCommands) / sizeof(ShellCommand);

static bool _checkCommands(Shell *shell, char *buf);
static int _getArgs(char *buf, char *argv[64]);

static void _tryLevenshtein(char *buf);

static char *_humanizeSize(uint64_t bytes, char *hrbytes);

Shell *shellOpen(const char *IMGPATH) {
	Shell *shell = malloc(sizeof(*shell));

	shell->fs = ext2Open(IMGPATH);
	shell->cd = INODE_RES_ROOT_DIR;

	shell->pathLevel = 0;

	shell->err = EXIT_SUCCESS;
	shell->run = true;

	return shell;
}

void shellFree(Shell *shell) {
	ext2Free(shell->fs);

	free(shell);
}

bool shellRun(Shell *shell) {
	if( !shell ) {
		return false;
	}

	puts("== ext2 shell ==");
	puts("type 'help' for help");

	char buf[1024];

	while( shell->run ) {
		fputs("\n/", stdout);
		for( int i = 0; i < shell->pathLevel; ++i ) {
			fprintf(stdout, "%s/", shell->path[i].name);
		}

		if( shell->err == EXIT_SUCCESS ) {
			fputs("\n> ", stdout);
		} else {
			fputs("\n" ANSI_COLOR_RED "> " ANSI_COLOR_RESET, stdout);
		}

		if( fgets(buf, 1024, stdin) == NULL ) {
			break;
		}

		if( *buf == '\n' ) {
			continue;
		}

		buf[strcspn(buf, "\n")] = '\0';

		bool cmdFound = _checkCommands(shell, buf);
		if( !cmdFound ) {
			printf("no such command '%s'", buf);
			_tryLevenshtein(buf);
		}
	}

	shellFree(shell);
	return true;
}

static bool _checkCommands(Shell *shell, char *buf) {
	ShellCommand *cmd;

	char *argv[64];
	int argc = _getArgs(buf, argv);

	for( int i = 0; i < SHELL_CMD_COUNT; ++i ) {
		cmd = &_shellCommands[i];

		if( strcmp(argv[0], cmd->name) == 0 ) {
			shell->err = cmd->fn(shell, argc, argv);

			return true;
		}
	}

	return false;
}

static int _getArgs(char *buf, char *argv[64]) {
	int argc = 0;

	argv[argc++] = strtok(buf, " ");
	while( argc < 64 && (argv[argc] = strtok(NULL, " ")) != NULL ) {
		++argc;
	}

	if( argc == 64 ) {
		WARN(
			"only up to 64 arguments can be passed to a shell command "
			"(command will be "
			"truncated at arg '%s')\n",
			argv[63]
		);
	}

	return argc;
}

static void _tryLevenshtein(char *buf) {
	char *bestCmd = NULL;
	int bestDist = INT_MAX;

	ShellCommand *cmd;
	for( int i = 0; i < SHELL_CMD_COUNT; ++i ) {
		cmd = &_shellCommands[i];

		int dist = utilLevenshtein(buf, cmd->name);
		if( dist < bestDist ) {
			bestDist = dist;
			bestCmd = cmd->name;
		}
	}

	if( bestDist > 2 ) {
		putchar('\n');
		return;
	}

	printf(" (did you mean '%s'?)\n", bestCmd);
}

Dir *_findInode(Dir *root, char *filename) {
	bool found = false;

	Dir *dir = root;
	while( true ) {
		if( dir == NULL ) {
			break;
		}

		if( strcmp(dir->filename, filename) == 0 ) {
			found = true;
			break;
		}

		dir = dir->next;
	}

	if( !found ) {
		ERR("'%s' not found\n", filename);
		return NULL;
	}

	return dir;
}

SHELL_FN(cat) {
	if( argc != 2 ) {
		puts("usage: cat [file]");
		return EXIT_FAILURE;
	}

	char *filename = argv[1];

	Dir root;
	if( !ext2GetDir(shell->fs, shell->cd, &root) ) {
		return EXIT_FAILURE;
	}

	Dir *dir = _findInode(&root, filename);
	if( dir == NULL ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	if( dir->filetype != DIR_FT_FILE ) {
		printf("'%s' is not a file (is a %s)\n", filename, dirGetFiletype(dir));

		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	FP data;
	ext2ReadFile(shell->fs, dir->inode, &data);
	for( size_t i = 0; i < data.size; ++i ) {
		putchar(utilRead8(&data));
	}

	utilFreeFile(&data);
	dirFreeLinkedList(&root);

	return EXIT_SUCCESS;
}

SHELL_FN(cd) {
	if( argc != 2 ) {
		puts("usage: cd [dir]");
		return EXIT_FAILURE;
	}

	char *into = argv[1];
	if( strcmp(into, ".") == 0 ) {
		return EXIT_SUCCESS;
	}

	Dir root;
	if( !ext2GetDir(shell->fs, shell->cd, &root) ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	Dir *dir = _findInode(&root, into);
	if( dir == NULL ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	if( dir->filetype != DIR_FT_DIR ) {
		printf("'%s' is not a dir (is a %s)\n", into, dirGetFiletype(dir));

		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	shell->cd = dir->inode;

	if( strcmp(into, "..") == 0 ) {
		if( shell->pathLevel > 0 ) {
			--shell->pathLevel;
		}
	} else if( shell->pathLevel < 127 ) {
		shell->path[shell->pathLevel].inode = dir->inode;
		strncpy(
			shell->path[shell->pathLevel].name, dir->filename, dir->nameLen + 1
		);

		++shell->pathLevel;
	}

	dirFreeLinkedList(&root);
	return EXIT_SUCCESS;
}

SHELL_FN(clear) {
	UNUSED(shell);
	UNUSED(argc);
	UNUSED(argv);

	clrscr();
	return EXIT_SUCCESS;
}

SHELL_FN(exit) {
	UNUSED(argc);
	UNUSED(argv);

	shell->run = false;
	return EXIT_SUCCESS;
}

static void _fsdumpUsage(void) {
	puts("usage: fsdump [dump-format-string]");
	puts("  a               dumps all");
	puts("  b               dumps the first 32 block group descriptors");
	puts("  B               dumps all block group descriptors");
	puts("  i               dumps the first 32 inodes");
	puts("  i               dumps all inodes");
	puts("  r               dumps the root inode");
	puts("  s               dumps the Superblock");
	putchar('\n');
	puts("example:");
	puts("  fsdump a       dumps all");
	puts("  fsdump si      dumps Superblock and inodes");
}

SHELL_FN(fsdump) {
	if( argc != 2 ) {
		_fsdumpUsage();
		return EXIT_FAILURE;
	}

	int flags = 0;
	char c = '\0';
	char *fmt = argv[1];

	while( true ) {
		c = *fmt++;
		if( c == '\0' ) {
			break;
		}

		switch( c ) {
		case 'a':
			flags = DUMP_ALL;
			break;
		case 'b':
			flags |= DUMP_BGDESCRIPTOR;
			flags &= ~DUMP_ALL_BGDESCRIPTOR;
			break;
		case 'B':
			flags |= DUMP_ALL_BGDESCRIPTOR;
			flags &= ~DUMP_BGDESCRIPTOR;
			break;
		case 'i':
			flags |= DUMP_INODE;
			flags &= ~(DUMP_INODE_ALL | DUMP_INODE_ROOT);
			break;
		case 'I':
			flags |= DUMP_INODE_ALL;
			flags &= ~(DUMP_INODE | DUMP_INODE_ROOT);
			break;
		case 'r':
			flags |= DUMP_INODE_ROOT;
			flags &= ~(DUMP_INODE | DUMP_INODE_ALL);
			break;
		case 's':
			flags |= DUMP_SUPERBLOCK;
			break;
		default:
			printf("unknown character in format string '%c'\n\n", c);
			_fsdumpUsage();
			return EXIT_FAILURE;
		}
	}

	ext2Dump(shell->fs, flags);
	return EXIT_SUCCESS;
}

SHELL_FN(help) {
	UNUSED(shell);
	UNUSED(argc);
	UNUSED(argv);

	puts("== ext2 shell ==");
	putchar('\n');

	puts("about:");
	puts("  this is a small shell for interfacing with the ext2 emulator");
	putchar('\n');

	puts("commands:");
	puts("  cat              displays the contents of a file");
	puts("  cd               changes the current directory");
	puts("  clear            clears the screen");
	puts("  cls              'clear' alias -- clears the screen");
	puts("  dir              'ls' alias -- lists the contents of a directory");
	puts("  exit             exits the shell");
	puts("  fsdump           dumps information about the filesystem");
	puts("  help             display this help text");
	puts("  ls               lists the contents of a directory");
	puts("  stat             displays information about a file");
	putchar('\n');

	puts("faq:");
	puts("  how do i exit?   ctrl+c or type 'exit'");

	return EXIT_SUCCESS;
}

SHELL_FN(ls) {
	UNUSED(argc);
	UNUSED(argv);

	Dir root;
	if( !ext2GetDir(shell->fs, shell->cd, &root) ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	Dir *dir = &root;
	while( true ) {
		if( dir == NULL ) {
			break;
		}

		printf("  %-7s %s\n", dirGetFiletype(dir), dir->filename);
		dir = dir->next;
	}

	dirFreeLinkedList(&root);
	return EXIT_SUCCESS;
}

SHELL_FN(stat) {
	if( argc != 2 ) {
		puts("usage: stat [file]");
		return EXIT_FAILURE;
	}

	char *filename = argv[1];

	Dir root;
	if( !ext2GetDir(shell->fs, shell->cd, &root) ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	Dir *dir = _findInode(&root, filename);
	if( dir == NULL ) {
		dirFreeLinkedList(&root);
		return EXIT_FAILURE;
	}

	Inode inode;
	ext2GetInode(shell->fs, dir->inode, &inode);

	char humansize[BUFSIZ];
	uint64_t size = ext2GetInodeSize(shell->fs, dir->inode, &inode);
	_humanizeSize(size, humansize);

	uint32_t maxblock = inode.blocks / (2 << shell->fs->bgs->sb.logBlockSize);

	puts("data:");
	printf("  name.... %s\n", dir->filename);
	printf("  type.... %s\n", dirGetFiletype(dir));
	printf(
		"  size.... %-8s blocks... %-6" PRIu32 " fs blocks... %" PRIu32 "\n",
		humansize, inode.blocks, maxblock
	);
	printf(
		"  inode... %-8" PRIu32 " links.... %" PRIu16 "\n\n", dir->inode,
		inode.linkCount
	);

	puts("times:");

	fmttime_t date;

	utilFmtTime(inode.accessTime, date);
	printf("  access... %s\n", date);

	utilFmtTime(inode.modifyTime, date);
	printf("  modify... %s\n", date);

	utilFmtTime(inode.createTime, date);
	printf("  create... %s\n", date);

	utilFmtTime(inode.deleteTime, date);
	printf("  delete... %s\n", date);

	dirFreeLinkedList(&root);
	return EXIT_SUCCESS;
}

static const char *SUFFIX[] = { "B", "KiB", "MiB", "GiB", "TiB" };
static const uint8_t SUFFIX_LEN = sizeof(SUFFIX) / sizeof(*SUFFIX);

static char *_humanizeSize(uint64_t bytes, char *out) {
	int i;
	for( i = 0; i < SUFFIX_LEN; i++ ) {
		if( bytes < 1024 ) {
			break;
		}

		bytes >>= 10;
	}

	snprintf(out, BUFSIZ, "%lu%s", bytes, SUFFIX[i]);
	return out;
}
