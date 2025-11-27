/* Minimal stdio.h stub for ESBMC on Windows */
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

/* Basic types */
typedef struct FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Basic functions - stubs for ESBMC */
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int fflush(FILE *stream);
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);
int fscanf(FILE *stream, const char *format, ...);
int scanf(const char *format, ...);
int sscanf(const char *str, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
void perror(const char *s);
int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);

#endif /* _STDIO_H */

