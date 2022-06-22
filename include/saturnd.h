#ifndef SATURND_H
#define SATURND_H

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "client-request.h"
#include "server-reply.h"
#include "timing-text-io.h"

#define BUFSIZE 1024



uint16_t getOPCODE(int descPipe);
int create_tasksdir();
int checkIfDirectoryExists(const char* filename);
int read_CR_request(int descPipe);
int write_CR_reply(int descPipe);
int create_file_task(char *time,char *cmd,int id);
void create_fifo();
int count_tasks(char *path);
char** str_split(char* a_str, const char a_delim);
timing *get_task_timing(char *timingline);
int write_LS_reply(int descReplyPipe);
int write_LS_replyBIS(int descReplyPipe,int argc,char *argv[],char *minutes_str,char *hours_str,char *daysofweek_str);
int get_argc_cmdline (char **cmdline);
int read_RM_request(int descRequestPipe);
int write_RM_reply(int descReplyPipe,int exitcode);
int read_STDERR_request(int descRequestPipe);
int read_STDOUT_request(int descRequestPipe);
int write_O_reply(int descRequestPipe);


#endif