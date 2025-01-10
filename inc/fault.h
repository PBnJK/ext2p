#ifndef GUARD_ELFP_FAULT_H_
#define GUARD_ELFP_FAULT_H_

/* Fault-handling utilities */

#include <stdio.h>
#include <stdlib.h>

#define ANSI_ESCAPE "\033["
#define ANSI_COLOR_RED ANSI_ESCAPE "31m"
#define ANSI_COLOR_RED_FG ANSI_ESCAPE "41m"
#define ANSI_COLOR_GREEN ANSI_ESCAPE "32m"
#define ANSI_COLOR_YELLOW ANSI_ESCAPE "33m"
#define ANSI_COLOR_RESET ANSI_ESCAPE "0m"

#define LOG_MSG ANSI_COLOR_GREEN "log" ANSI_COLOR_RESET ": "
#define WARNING_MSG ANSI_COLOR_YELLOW "warning" ANSI_COLOR_RESET ": "
#define ERR_MSG ANSI_COLOR_RED "error" ANSI_COLOR_RESET ": "
#define FATAL_MSG ANSI_COLOR_RED_FG "fatal" ANSI_COLOR_RESET ": "

#ifndef NDEBUG
#define LOC() fprintf(stderr, "(%s:%d)\n", __FILE__, __LINE__)
#else
#define LOC()
#endif

#define LOG(...) printf(LOG_MSG __VA_ARGS__);
#define WARN(...) printf(WARNING_MSG __VA_ARGS__);

#define ERR(...)                                                               \
	do {                                                                       \
		LOC();                                                                 \
		fprintf(stderr, ERR_MSG __VA_ARGS__);                                  \
	} while( 0 );

#define FATAL(...)                                                             \
	do {                                                                       \
		LOC();                                                                 \
		fprintf(stderr, FATAL_MSG __VA_ARGS__);                                \
		exit(EXIT_FAILURE);                                                    \
	} while( 0 );

#endif // !GUARD_ELFP_FAULT_H_
