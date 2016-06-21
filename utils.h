

#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>

char *dup(const char *s);
char *dupl(const char *s, const char *end);
char *trim(char *str);
char *strrev(char *str);


#endif 

