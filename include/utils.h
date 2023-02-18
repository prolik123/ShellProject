#ifndef _UTILS_H_
#define _UTILS_H_

#include "siparse.h"
#include "config.h"
#include <signal.h>


volatile int forgroundPID[MAX_NUMBER_FORGROUNDPROCESS];
volatile int backgroundPIDInfo[MAX_BACKGROUND_INFO];
volatile int backgroundSignalInfo[MAX_BACKGROUND_INFO];
volatile int backgroundEndStatus[MAX_BACKGROUND_INFO];

struct sigaction newSIGHandler, oldSIGHandler;

void printcommand(command *, int);
void printpipeline(pipeline *, int);
void printparsedline(pipelineseq *);
void printError(const char* command,const char* error);
int executeRawCommand(char* args[]);
int executecommand(command *);

size_t getLengthOfCommand(command *);

command * pickfirstcommand(pipelineseq *);

int isCharacterDeviceAsInput();
void printPromptSignIfTerminal(int);
int isSyntaxError(pipelineseq* );
int getNumberOfCommand(pipeline* );

void executePipeLine(pipeline* );

void sigChildHandler(int);
int howManyProccesLeft();

#endif /* !_UTILS_H_ */
