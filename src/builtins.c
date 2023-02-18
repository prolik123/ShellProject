/// header guard
#ifndef _BUILTINS_C_
#define _BUILTINS_C_

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#include "builtins.h"
#include "utils.h"

#define SIZE_OF_BUILD_TABLE 6

int echo(char*[]);
int undefined(char *[]);
int lexit(char *[]);
int lls(char* []);
int lcd(char* []);
int lkill(char* []);


builtin_pair builtins_table[]={
	{"exit",	&lexit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

int findAndExecIfExists(command* cmd) {

	size_t numOfArgs = getLengthOfCommand(cmd);
	char* args[numOfArgs+1];
	argseq * argseq = cmd->args;

	// Getting arguments for execution to args
	for(int k=0;k<numOfArgs;k++) { 
		args[k] = argseq->arg;
		argseq= argseq->next;
	}


	args[numOfArgs] = NULL;

	if(!args[0]) /// tak nie powinno sie stac ale casuje na wszelki wypadek
		return 1;

	for(int k=0;k<SIZE_OF_BUILD_TABLE;k++) {
		if(builtins_table[k].name == NULL)
			continue;
		if(strcmp(args[0],builtins_table[k].name) == 0) {
			if(((builtins_table[k].fun)(args)) == 1) {
				fprintf(stderr,"Builtin %s error.\n",args[0]);
			}
			return 1;
		} 
	}
	return 0;
}

int lexit(char *argv[]) {
	exit(0);
}

int 
echo( char * argv[])
{
	//printf("MY ECHO WIIII\n");
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int 
undefined(char * argv[])
{
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

int lls(char * argv[]) {

	DIR* dir;
	if(!argv[1]) {
		//char * currdir = getcwd(currdir,MAX_PATH_LENGTH);
		dir = opendir(".");
	}
	else
		dir = opendir(argv[1]);
	if(dir == NULL)
		return 1;
	struct dirent* dirinfo = readdir(dir);   
	while (dirinfo != NULL) {
		if(dirinfo->d_name[0] != '.')
			printf("%s\n", dirinfo->d_name);
		dirinfo = readdir(dir);
	}
	//printf("\n");
    closedir(dir);
	return 0;
}

int lcd(char* argv[]) {
	char * cmd;
	if(!(argv[1]))
		cmd = getenv("HOME");
	else
		cmd = argv[1];
	if(chdir(cmd) == -1 || (argv[1] && argv[2]))
		return 1;
	return 0;
}

int checkConversionError(char* arg) {
	char* end;
	const long sl = strtol(arg, &end, 10);
	if (end == arg || '\0' != *end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl > INT_MAX ||  (sl < INT_MIN)) 
		return 1;
	return 0;
}

int lkill(char* argv[]) {
	int ret = 0;
	if(argv[1] && argv[1][0] == '-' && argv[2]) {
		char* end;
		if(checkConversionError(argv[1]+1) || checkConversionError(argv[2]))
			return 1;
		int sig = (int)strtol(argv[1]+1, &end, 10);
		int pwd = (int)strtol(argv[2], &end, 10);;
		ret = kill(pwd,sig);
	}
	else if(argv[1]) {
		ret = kill(atoi(argv[1]),SIGTERM);
	}
	if(ret == -1 || !(argv[1]))
		return 1;
	return 0;
}

#endif //_BUILTINS_C_
