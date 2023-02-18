#ifndef _BUILTINS_H_
#define _BUILTINS_H_
#include <siparse.h>

#define BUILTIN_ERROR 2
#define MAX_PATH_LENGTH 2048

typedef struct {
	char* name;
	int (*fun)(char**); 
} builtin_pair;

extern builtin_pair builtins_table[];

int findAndExecIfExists(command* );

#endif /* !_BUILTINS_H_ */
