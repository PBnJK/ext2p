/* ext2p
 * Shell
 */

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

typedef bool (*cmd_fn)(Shell *shell, int argc, char *argv[]);

#define SHELL_FN(N) static bool _shell_##N(Shell *shell, int argc, char *argv[])

typedef struct _ShellCommand {
	char *name;
	cmd_fn fn;
} ShellCommand;

SHELL_FN(cd);
SHELL_FN(clear);
SHELL_FN(exit);
SHELL_FN(fsdump);
SHELL_FN(help);
SHELL_FN(ls);

static ShellCommand _shellCommands[] = {
	{ "cd", _shell_cd },	 { "clear", _shell_clear },
	{ "cls", _shell_clear }, { "dir", _shell_ls },
	{ "exit", _shell_exit }, { "fsdump", _shell_fsdump },
	{ "help", _shell_help }, { "ls", _shell_ls },
};
const int SHELL_CMD_COUNT = sizeof(_shellCommands) / sizeof(ShellCommand);

static bool _checkCommands(Shell *shell, char *buf, bool *run);
static int _getArgs(char *buf, char *argv[64]);

static void _tryLevenshtein(char *buf);

Shell *shellOpen(const char *IMGPATH) {
	Shell *shell = malloc(sizeof(*shell));

	shell->fs = ext2Open(IMGPATH);
	shell->cd = INODE_RES_ROOT_DIR;
	shell->pathLevel = 0;

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

	bool run = true;
	char buf[1024];

	while( run ) {
		fputs("\n/", stdout);
		for( int i = 0; i < shell->pathLevel; ++i ) {
			fprintf(stdout, "%s/", shell->path[i]);
		}

		fputs("\n> ", stdout);
		if( fgets(buf, 1024, stdin) == NULL ) {
			break;
		}

		if( *buf == '\n' ) {
			continue;
		}

		buf[strcspn(buf, "\n")] = '\0';

		bool cmdFound = _checkCommands(shell, buf, &run);
		if( !cmdFound ) {
			printf("no such command '%s'", buf);
			_tryLevenshtein(buf);
		}
	}

	shellFree(shell);

	puts("bye!");
	return true;
}

static bool _checkCommands(Shell *shell, char *buf, bool *run) {
	ShellCommand *cmd;

	char *argv[64];
	int argc = _getArgs(buf, argv);

	for( int i = 0; i < SHELL_CMD_COUNT; ++i ) {
		cmd = &_shellCommands[i];

		if( strcmp(argv[0], cmd->name) == 0 ) {
			*run = cmd->fn(shell, argc, argv);

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

SHELL_FN(cd) {
	if( argc != 2 ) {
		puts("usage: cd [dir]");
	}

	char *into = argv[1];
	if( strcmp(into, ".") == 0 ) {
		return true;
	}

	Dir *root = ext2GetDir(shell->fs, shell->cd);
	if( root == NULL ) {
		return true;
	}

	bool found = false;

	Dir *dir = root;
	while( true ) {
		if( dir == NULL ) {
			break;
		}

		if( strcmp(dir->filename, into) == 0 ) {
			found = true;
			break;
		}

		dir = dir->next;
	}

	if( !found ) {
		printf("couldn't cd: '%s' not found\n", into);
		goto end;
	}

	if( dir->filetype != DIR_FT_DIR ) {
		printf(
			"couldn't cd: '%s' is not a dir (is a %s)\n", into,
			dirGetFiletype(dir)
		);
		goto end;
	}

	shell->cd = dir->inode;

	if( strcmp(into, "..") == 0 ) {
		if( shell->pathLevel == 0 ) {
			goto end;
		}

		--shell->pathLevel;
	} else if( shell->pathLevel < 127 ) {
		strncpy(
			shell->path[shell->pathLevel++], dir->filename, dir->nameLen + 1
		);
	}

end:
	dirFreeLinkedList(root);
	return true;
}

SHELL_FN(clear) {
	(void)shell;
	(void)argc;
	(void)argv;

	clrscr();
	return true;
}

SHELL_FN(exit) {
	(void)shell;
	(void)argc;
	(void)argv;

	return false;
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
		return true;
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
			return true;
		}
	}

	ext2Dump(shell->fs, flags);
	return true;
}

SHELL_FN(help) {
	(void)shell;
	(void)argc;
	(void)argv;

	puts("== ext2 shell ==");
	putchar('\n');

	puts("about:");
	puts("  this is a small shell for interfacing with the ext2 emulator");
	putchar('\n');

	puts("commands:");
	puts("  cd               changes the current directory");
	puts("  clear            clears the screen");
	puts("  cls              'clear' alias -- clears the screen");
	puts("  dir              'ls' alias -- lists the contents of a directory");
	puts("  exit             exits the shell");
	puts("  fsdump           dumps information about the filesystem");
	puts("  help             display this help text");
	puts("  ls               lists the contents of a directory");
	putchar('\n');

	puts("faq:");
	puts("  how do i exit?   ctrl+c or type 'exit'");

	return true;
}

SHELL_FN(ls) {
	(void)argc;
	(void)argv;

	Dir *root = ext2GetDir(shell->fs, shell->cd);
	if( root == NULL ) {
		return true;
	}

	Dir *dir = root;
	while( true ) {
		if( dir == NULL ) {
			break;
		}

		printf("  %-7s %s\n", dirGetFiletype(dir), dir->filename);
		dir = dir->next;
	}

	dirFreeLinkedList(root);
	return true;
}
