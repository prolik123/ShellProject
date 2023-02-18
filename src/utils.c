#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
//#include <sys/wait.h>

#include "config.h"
#include "siparse.h"
#include "builtins.h"
#include "utils.h"

volatile int curentNumberOfFrogroundProc = 0;
volatile int currrentNumberOfBackGroundInfo = 0;

void 
printcommand(command *pcmd, int k)
{
	int flags;

	printf("\tCOMMAND %d\n",k);
	if (pcmd==NULL){
		printf("\t\t(NULL)\n");
		return;
	}

	printf("\t\targv=:");
	argseq * argseq = pcmd->args;
	do{
		printf("%s:", argseq->arg);
		argseq= argseq->next;
	}while(argseq!=pcmd->args);

	printf("\n\t\tredirections=:");
	redirseq * redirs = pcmd->redirs;
	if (redirs){
		do{	
			flags = redirs->r->flags;
			printf("(%s,%s):",redirs->r->filename,IS_RIN(flags)?"<": IS_ROUT(flags) ?">": IS_RAPPEND(flags)?">>":"??");
			redirs= redirs->next;
		} while (redirs!=pcmd->redirs);	
	}

	printf("\n");
}

void
printpipeline(pipeline * p, int k)
{
	int c;
	command ** pcmd;

	commandseq * commands= p->commands;

	printf("PIPELINE %d\n",k);
	
	if (commands==NULL){
		printf("\t(NULL)\n");
		return;
	}
	c=0;
	do{
		printcommand(commands->com,++c);
		commands= commands->next;
	}while (commands!=p->commands);

	printf("Totally %d commands in pipeline %d.\n",c,k);
	printf("Pipeline %sin background.\n", (p->flags & INBACKGROUND) ? "" : "NOT ");
}

void
printparsedline(pipelineseq * ln)
{
	int c;
	pipelineseq * ps = ln;

	if (!ln){
		printf("%s\n",SYNTAX_ERROR_STR);
		return;
	}
	c=0;

	do{
		printpipeline(ps->pipeline,++c);
		ps= ps->next;
	} while(ps!=ln);

	printf("Totally %d pipelines.",c);
	printf("\n");
}

command *
pickfirstcommand(pipelineseq * ppls)
{
	if ((ppls==NULL)
		|| (ppls->pipeline==NULL)
		|| (ppls->pipeline->commands==NULL)
		|| (ppls->pipeline->commands->com==NULL))	return NULL;
	
	return ppls->pipeline->commands->com;
}

size_t getLengthOfCommand(command * pcmd) {
	argseq * argseq = pcmd->args;
	size_t numOfArgs = 0;
	do{
		argseq= argseq->next;
		numOfArgs++;
	}while(argseq!=pcmd->args);
	return numOfArgs;
}

void printError(const char* command,const char* error) {
	fprintf(stderr,command);
	fprintf(stderr,": ");
	fprintf(stderr,error);
	fprintf(stderr,"\n");
}

int executeRawCommand(char* args[]) {
	int resultStatus = execvp(args[0],args);
	if(resultStatus != 0) {
		if(errno == ENOENT)
			printError(args[0],NO_DIR);
		else if(errno == EACCES)
			printError(args[0],PERM_DENIED);
		else
			printError(args[0],EXEC_ERR);
		return EXEC_FAILURE;
	}
	return 0;
}

int executecommand(command * pcmd) {

	size_t numOfArgs = getLengthOfCommand(pcmd);
	char* args[numOfArgs+1];
	argseq * argseq = pcmd->args;

	// Getting arguments for execution to args
	for(int k=0;k<numOfArgs;k++) { 
		args[k] = argseq->arg;
		argseq= argseq->next;
	}


	args[numOfArgs] = NULL;
	if(findAndExecIfExists(pcmd)) {
		return 1;
	}
	int isFailure = 0;
	if(pcmd->redirs != NULL && pcmd->redirs->r != NULL) {
		redirseq* currRedirSeq = pcmd->redirs;
		do{
			if(IS_RIN(currRedirSeq->r->flags))
				isFailure = (freopen(currRedirSeq->r->filename,"r",stdin) == NULL);
			else if(IS_ROUT(currRedirSeq->r->flags))
				isFailure =  (freopen(currRedirSeq->r->filename,"w",stdout) == NULL);
			else if(IS_RAPPEND(currRedirSeq->r->flags))
				isFailure= (freopen(currRedirSeq->r->filename,"a",stdout) == NULL);
			if(isFailure) {
				isFailure = 0;
				if(errno == ENOENT)
					printError(currRedirSeq->r->filename,NO_DIR);
				else if(errno == EACCES)
					printError(currRedirSeq->r->filename,PERM_DENIED);
				return 1;
			}
			currRedirSeq = currRedirSeq->next;
		}while(pcmd->redirs != currRedirSeq);
			
	}
	return executeRawCommand(args);
}


int isCharacterDeviceAsInput() {
	struct stat obj;
	fstat(0,&obj);
	//perror("");
	return S_ISCHR(obj.st_mode);
}

void printPromptSignIfTerminal(int isInAnotherDevice) {
	if(isInAnotherDevice == 0){
		for(int k=0;k<currrentNumberOfBackGroundInfo;k++) {
			printf("%s %d %s ","Background process",backgroundPIDInfo[k],"terminated.");
			if(WIFEXITED(backgroundEndStatus[k]))
				printf(" %s %d%s","exited with status", WEXITSTATUS(backgroundEndStatus[k]),")\n");
			else if(WIFSIGNALED(backgroundEndStatus[k])) {
				printf(" %s %d%s","killed by signal", WTERMSIG(backgroundEndStatus[k]),")\n");
			}
			else
				printf("\n");
		}
		currrentNumberOfBackGroundInfo = 0;
		printf(PROMPT_STR);
		fflush(stdout);
	}
	return;
}

void sigChildHandler(int signo) {
	//printf("HANDELER INVOKE\n");
	while(1) {
		int resultStatus= -1;
		int pid = waitpid(-1,&resultStatus,WNOHANG);
		if(errno == ECHILD || pid <= 0)
			return;
		int isInForground = 0;
		for(int k=0;k<curentNumberOfFrogroundProc;k++) {
			if(forgroundPID[k] == pid) {
				forgroundPID[k] = -1;
				isInForground = 1;
				break;
			}
		}
		if(currrentNumberOfBackGroundInfo < MAX_BACKGROUND_INFO && !isInForground) {
			backgroundPIDInfo[currrentNumberOfBackGroundInfo] = pid;
			backgroundEndStatus[currrentNumberOfBackGroundInfo++] = resultStatus;
		}
	}
	return;
}

int isSyntaxError(pipelineseq* ln) {
	pipelineseq* currPipeLineseq = ln;
	do{
		pipeline* currPipeLine = currPipeLineseq->pipeline;
		if(currPipeLine == NULL || currPipeLine->commands == NULL)
			return 1;

		commandseq* firstCmd = currPipeLine->commands;
		commandseq* currCmd = currPipeLine->commands;
		do{
			if(currCmd == NULL || currCmd->com == NULL || currCmd->com->args == NULL)
				return 1;
			currCmd = currCmd->next;
		}while(currCmd != firstCmd);
	}while(currPipeLineseq != ln);
	return 0;
}

int getNumberOfCommand(pipeline* pl) {
	commandseq* currCommand = pl->commands;
	int numberOfCommands = 0;
	do{
		numberOfCommands++;
		currCommand = currCommand->next;
	}while(currCommand != pl->commands);
	return numberOfCommands;
}

int howManyProccesLeft() {
	int cnt =0;
	for(int k=0;k<curentNumberOfFrogroundProc;k++) {
		//printf("%d\n",forgroundPID[k]);
		if(forgroundPID[k] != -1)
			cnt++;
	}
	return cnt;
}

void executePipeLine(pipeline* pl) {
	int numberOfCommand = getNumberOfCommand(pl);
	if(!LINBACKGROUND(pl->flags)) {
		struct sigaction act;
		sigaction(SIGCHLD,NULL,&act);
		act.sa_handler = &sigChildHandler;
		sigaction(SIGCHLD,&act,NULL);
	}
	if(numberOfCommand == 0)
		return;
	if(numberOfCommand == 1 && findAndExecIfExists(pl->commands->com)) {
		return;
	}
	else {
		int pipeTab[numberOfCommand-1][2];
		commandseq* currCmd = pl->commands;
		sigset_t myset;
		sigset_t old;
	
		sigemptyset(&myset);
		sigaddset(&myset,SIGCHLD);
		sigprocmask(SIG_BLOCK,&myset,&old);
		if(!LINBACKGROUND(pl->flags)) {
			for(int k=0;k<numberOfCommand;k++)
				forgroundPID[k] = -1;
			curentNumberOfFrogroundProc = numberOfCommand;
		}
		for(int i=0;i<numberOfCommand;i++) {
			int pipeErr = 1;
			if(i!=numberOfCommand - 1)
				pipeErr = pipe(pipeTab[i]);
			int currPid = fork();
			if(!currPid) {
				sigprocmask(SIG_UNBLOCK,&myset,NULL);
				if(LINBACKGROUND(pl->flags)) {
					setsid();
				}
				sigaction(SIGINT,&oldSIGHandler,NULL);
				if(i != numberOfCommand -1) {
					dup2(pipeTab[i][1],STDOUT_FILENO);
					close(pipeTab[i][0]);
					close(pipeTab[i][1]);
				}
				if(i != 0) {
					dup2(pipeTab[i-1][0],STDIN_FILENO);
					close(pipeTab[i-1][0]);
					close(pipeTab[i-1][1]);
				}
				int result =executecommand(currCmd->com);

				exit(result);
			}
			else {
				if(!LINBACKGROUND(pl->flags)) {
					forgroundPID[i] = currPid;
				}
				if(i != 0) {
					close(pipeTab[i-1][0]);
					close(pipeTab[i-1][1]);
				}
			}
			currCmd = currCmd->next;	
		}
		if(LINBACKGROUND(pl->flags)) {
			
			sigprocmask(SIG_UNBLOCK,&myset,NULL);
			return;
		}
		//printf("%d \n",howManyProccesLeft(curentNumberOfFrogroundProc));
		while(howManyProccesLeft() > 0) {
			//printf("WHILE!!!\n");
			sigsuspend(&old);
		}
		sigprocmask(SIG_UNBLOCK,&myset,NULL);
	}
}
