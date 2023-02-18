#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

void newEmptyHandler(int signo) {
	return;
}

int
main(int argc, char *argv[])
{
	pipelineseq * ln;
	command *pcmd;
	size_t len = -1;
	int lastBuff = 0;
	int isInAnotherDevice = 0;
	char buf[2*MAX_LINE_LENGTH+1];
	buf[2*MAX_LINE_LENGTH] = '\0';
	for(int k=0;k<2*MAX_LINE_LENGTH;k++)
		buf[k] = ' ';
	int syntaxErr = 0;
	sigemptyset(&newSIGHandler.sa_mask);
	newSIGHandler.sa_handler = &newEmptyHandler; 
	sigaction(SIGINT,&newSIGHandler,&oldSIGHandler);

	if(isCharacterDeviceAsInput() == 0) {
		isInAnotherDevice = 1;
	}
	for(int k=0;k<MAX_NUMBER_FORGROUNDPROCESS;k++) {
		forgroundPID[k] = -1;
	}
	printPromptSignIfTerminal(isInAnotherDevice);
	while(len == -1)
		len = read(0,buf,MAX_LINE_LENGTH);

	while (len + lastBuff > 0) {	
		char* lastout = buf;
		char* out = strchr(buf,'\n');
		if(len+lastBuff >= MAX_LINE_LENGTH && out == NULL && syntaxErr == 0) {
			fprintf(stderr,SYNTAX_ERROR_STR);
			fprintf(stderr,"\n");
			syntaxErr = 1;
			printPromptSignIfTerminal(isInAnotherDevice);
			lastBuff = 0;
			len = read(0,buf,MAX_LINE_LENGTH);
			while(len == -1)
				len = read(0,buf,MAX_LINE_LENGTH);
			continue;
		}
		while(out != NULL && out - buf < len + lastBuff && out < buf + len + lastBuff) {
			if(syntaxErr)	{
				syntaxErr = 0;

				lastout = out+1;
				out = strchr(lastout,'\n');
				continue;
			}
			*(out) = '\0';
			if(*lastout != '\n' && *lastout !='#' && lastout != out) {
				ln = parseline(lastout);
				if(out -lastout > MAX_LINE_LENGTH || ln == NULL) { 
					fprintf(stderr,SYNTAX_ERROR_STR);
					fprintf(stderr,"\n");
				}
				else {
					pipelineseq* currPipeLineseq = ln;
					if(syntaxErr == 0 && pickfirstcommand(ln) != NULL) {

						if(isSyntaxError(ln)) {
							fprintf(stderr,SYNTAX_ERROR_STR);
							fprintf(stderr,"\n");
						}
						else {
							do{
								pipeline* currPipeLine = currPipeLineseq->pipeline;
								executePipeLine(currPipeLine);
								currPipeLineseq = currPipeLineseq->next;
							}
							while(currPipeLineseq != ln);
						}
					}
					syntaxErr = 0;
				}
				fflush(stderr);
				fflush(stdout);
			}
			lastout = out+1;
			out = strchr(lastout,'\n');
		}
		int tempBuff = 0;
		for(int k=0;lastout-buf < len+lastBuff;k++) {
			buf[k] = *lastout;
			lastout++;
			tempBuff++;
		}
		lastBuff = tempBuff > MAX_LINE_LENGTH?MAX_LINE_LENGTH:tempBuff;
		printPromptSignIfTerminal(isInAnotherDevice);
		len = read(0,buf + lastBuff, MAX_LINE_LENGTH);
		
		while(len == -1)
			len = read(0,buf + lastBuff,MAX_LINE_LENGTH);
	}
	/// all error checked
	return 0;
}

