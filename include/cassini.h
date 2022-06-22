#ifndef CASSINI_H
#define CASSINI_H

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>

#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "client-request.h"
#include "server-reply.h"
#include "timing-text-io.h"

#define BUFSIZE 1024

int write_CR_req(char *descRequestPipeChar,int argc,char *argv[],uint16_t operation,char *minutes_str,char *hours_str,char *daysofweek_str);
int read_CR_reply(char *descReplyPipeChar);
int write_LS_req(char *descRequestPipeChar,uint16_t operation);
int read_LS_reply(char *descReplyPipeChar);
int write_RM_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid);
int write_TX_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid);
int read_TX_reply(char* descReplyPipeChar);
int write_O_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid);
int read_O_reply(char *descReplyPipeChar);
int write_EXIT_req(char *descRequestPipeChar,uint16_t operation);
int read_RM_reply(char *descReplyPipeChar);
int read_EXIT_reply(char *descReplyPipeChar);
char* int_en_char(int x);


#endif // CASSINI
