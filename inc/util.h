#ifndef GUARD_ELFP_UTIL_H_
#define GUARD_ELFP_UTIL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define UNUSED(A) ((void)(A))

#define UTIL_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define UTIL_MAX(A, B) (((A) > (B)) ? (A) : (B))

#define UTIL_SHF(B, N) (((B) & 0xFF) << (N))
#define UTIL_SHFR(B, N) (((B) >> (N)) & 0xFF)

/* Structure representing an open file
 * Used for passing data around
 */
typedef struct _FP {
	char *_start; /* Points to the start of the allocated memory segment */
	char *data; /* Actual data pointer that should be used */
	size_t size; /* Size of the file */
} FP;

typedef char fmttime_t[BUFSIZ];

/* Reads the file at 'FILEPATH' and returns its contents
 * Returns false on failure
 */
bool utilReadFile(const char *FILEPATH, FP *fp);

void utilCloneFile(FP *fp, FP *dest);

/* Frees a file pointer returned by 'utilReadFile' */
void utilFreeFile(FP *fp);

uint8_t utilRead8(FP *fp);
uint16_t utilRead16(bool le, FP *fp);
uint32_t utilRead32(bool le, FP *fp);
uint64_t utilRead64(bool le, FP *fp);

void utilWrite8(FILE *file, uint8_t data);
void utilWrite16(FILE *file, uint16_t data);
void utilWrite32(FILE *file, uint32_t data);
void utilWrite64(FILE *file, uint64_t data);

size_t utilFmtTime(time_t time, fmttime_t ftime);

int utilLevenshtein(const char *A, const char *B);

#endif // !GUARD_ELFP_UTIL_H_
