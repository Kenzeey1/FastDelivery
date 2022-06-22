#include "cassini.h"

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

int main(int argc, char * argv[]) {
  errno = 0;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;

  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;

  int opt;
  char * strtoull_endp;
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }

  // --------
  // | TODO |
  // --------
  char *descRequestPipeChar = malloc(100);
  char *descReplyPipeChar = malloc(100);
  if(descRequestPipeChar == NULL || descReplyPipeChar == NULL) goto error;

  if(pipes_directory == NULL){
    pipes_directory = malloc(100);
    /*
    pipes_directory = malloc(100);
    if(pipes_directory==NULL) goto error;

    strcat(strcpy(pipes_directory,"/tmp/"),getenv("USER"));
    strcat(pipes_directory,"/saturnd");

    strcat(strcpy(descRequestPipeChar,pipes_directory),"/saturnd-request-pipe");
    strcat(strcpy(descReplyPipeChar,pipes_directory),"/saturnd-reply-pipe");
    
    free(pipes_directory);
    */
    strcat(strcpy(descRequestPipeChar,"run/pipes"),"/saturnd-request-pipe");
    strcat(strcpy(descReplyPipeChar,"run/pipes"),"/saturnd-reply-pipe");
  } else {

    strcat(strcpy(descRequestPipeChar,"run/pipes"),"/saturnd-request-pipe");
    strcat(strcpy(descReplyPipeChar,"run/pipes"),"/saturnd-reply-pipe");

  }
  
  if(operation == CLIENT_REQUEST_LIST_TASKS){

    if(write_LS_req(descRequestPipeChar,operation) == EXIT_FAILURE) goto error;
    if(read_LS_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  } else if(operation == CLIENT_REQUEST_CREATE_TASK){ 
    argc -= optind;
    argv += optind;

    if(write_CR_req(descRequestPipeChar,argc,argv,operation,minutes_str,hours_str,daysofweek_str) == EXIT_FAILURE) goto error;
    if(read_CR_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  } else if(operation == CLIENT_REQUEST_REMOVE_TASK){

    if(write_RM_req(descRequestPipeChar,operation,taskid) == EXIT_FAILURE) goto error;
    if(read_RM_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  }else if(operation == CLIENT_REQUEST_GET_TIMES_AND_EXITCODES){

    if(write_TX_req(descRequestPipeChar,operation,taskid) == EXIT_FAILURE) goto error;
    if (read_TX_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  }else if (operation == CLIENT_REQUEST_GET_STDOUT || operation == CLIENT_REQUEST_GET_STDERR){

    if(write_O_req(descRequestPipeChar,operation,taskid) == EXIT_FAILURE) goto error;
    if(read_O_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  }else if (operation == CLIENT_REQUEST_TERMINATE){

    if(write_EXIT_req(descRequestPipeChar,operation) == EXIT_FAILURE) goto error;
    if(read_EXIT_reply(descReplyPipeChar) == EXIT_FAILURE) goto error;

  }

  free(descRequestPipeChar);
  free(descReplyPipeChar);

  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  free(descRequestPipeChar);
  free(descReplyPipeChar);
  return EXIT_FAILURE;
}

int write_O_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid){
  
  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t tx = htobe16(operation);
  taskid = htobe64(taskid);

  char *buf = malloc(sizeof(uint16_t) + sizeof(uint64_t));

  memcpy(buf,&tx,sizeof(uint16_t));
  memcpy(buf+2,&taskid,sizeof(uint64_t));

  if(write(descRequestPipe,buf,sizeof(uint16_t)+sizeof(uint64_t)) == -1){
    free(buf);
    goto error;
  }
  close(descRequestPipe);
  free(buf);
  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->write_O_req->open");
    return EXIT_FAILURE;

  error:
    close(descRequestPipe);
    if (errno != 0) perror("main->write_O_req");
    return EXIT_FAILURE;
}

int write_TX_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid){

  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t tx = htobe16(operation);
  taskid = htobe64(taskid);

  char *buf = malloc(sizeof(uint16_t) + sizeof(uint64_t));

  memcpy(buf,&tx,sizeof(uint16_t));
  memcpy(buf+2,&taskid,sizeof(uint64_t));

  if(write(descRequestPipe,buf,sizeof(uint16_t)+sizeof(uint64_t)) == -1){
    free(buf);
    goto error;
  }
  close(descRequestPipe);
  free(buf);
  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->write_TX_req->open");
    return EXIT_FAILURE;

  error:
    close(descRequestPipe);
    if (errno != 0) perror("main->write_TX_req");
    return EXIT_FAILURE;

}

int write_RM_req(char *descRequestPipeChar,uint16_t operation,uint64_t taskid){

  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t rm = htobe16(operation);
  taskid = htobe64(taskid);

  char *buf = malloc(sizeof(uint16_t) + sizeof(uint64_t));

  memcpy(buf,&rm,sizeof(uint16_t));
  memcpy(buf+2,&taskid,sizeof(uint64_t));

  if(write(descRequestPipe,buf,sizeof(uint16_t)+sizeof(uint64_t)) == -1) goto error;
  close(descRequestPipe);

  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->write_RM_req->open");
    return EXIT_FAILURE;

  error:
    close(descRequestPipe);
    if (errno != 0) perror("main->write_RM_req");
    return EXIT_FAILURE;

}

int write_LS_req(char *descRequestPipeChar,uint16_t operation){

  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t ls = htobe16(operation);

  if(write(descRequestPipe,&ls,sizeof(ls)) == -1) goto error;
  close(descRequestPipe);

  return EXIT_SUCCESS;

  error:
    close(descRequestPipe);
    if (errno != 0) perror("main->write_LS_req");
    return EXIT_FAILURE;

  error_open:
    if (errno != 0) perror("main->write_LS_req->open");
    return EXIT_FAILURE;

}

int write_CR_req(char *descRequestPipeChar,int argc,char *argv[],uint16_t operation,
                        char *minutes_str,char *hours_str,char *daysofweek_str){

  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t cr = htobe16(operation);
  timing *time = malloc(sizeof(timing));
  if(time == NULL) goto error;

  if(timing_from_strings(time,minutes_str,hours_str,daysofweek_str) == -1){
    
    free(time);
    goto error;

  }else{

    uint64_t minutes = htobe64(time->minutes);
    uint32_t hours = htobe32(time->hours);
    uint8_t daysofweek = time->daysofweek;
    free(time);

    uint32_t cmdargc = htobe32((uint32_t)argc);
    char *cmdargv[argc]; // maybe changer en malloc( argc*sizeof(char*) )
    for(int i = 0; i < argc; i++){
      cmdargv[i] = argv[i];
    }

    char *buf = malloc(BUFSIZE); // 1024 == BUFSIZE
    if(buf == NULL) goto error;
    memcpy(buf,&cr,sizeof(uint16_t));
    memcpy(buf+2,&minutes,sizeof(uint64_t));
    memcpy(buf+10,&hours,sizeof(uint32_t));
    memcpy(buf+14,&daysofweek,sizeof(uint8_t));
    memcpy(buf+15,&cmdargc,sizeof(uint32_t));

    //ecriture bit par bit des argument de la commande et de leurs longeur;
    int i = 0;
    int cmp = 0;
    while(i < argc){

      int j = 0;

      //Longeur de l'argument :
      int taille = strlen(cmdargv[i]);
      uint32_t length = htobe32((uint32_t) taille);
      memcpy(buf+19+cmp,&length,4);
      cmp += 4;

      //Contenue de l'argument :
      while(j < taille){
        uint8_t aecrire = (uint8_t)cmdargv[i][j];
        memcpy(buf+19+cmp,&aecrire,sizeof(aecrire));// sizeof(aecrire) renvoie normalement tjr 1 car c'est un char
        j++;
        cmp++;
      }
      i++;
    }
    if(write(descRequestPipe,buf,19+cmp) < 0){
      free(buf);
      goto error;
    }
    //printf("%s\n",buf);
    free(buf);
  }

  close(descRequestPipe);

  return EXIT_SUCCESS;

  error:
    close(descRequestPipe);
    if (errno != 0) perror("main->write_CR_req");
    return EXIT_FAILURE;

   error_open:
    if (errno != 0) perror("main->write_CR_req->open");
    return EXIT_FAILURE;
}

int write_EXIT_req(char *descRequestPipeChar,uint16_t operation){

  int descRequestPipe = open(descRequestPipeChar,O_WRONLY);
  if(descRequestPipe == -1) goto error_open;

  uint16_t exit = htobe16(operation);

  if(write(descRequestPipe,&exit,sizeof(uint16_t)) == -1) goto error_write;

  close(descRequestPipe);

  return EXIT_SUCCESS;

  error_open:
    perror("main->write_EXIT_req->open");
    return EXIT_FAILURE;

  error_write:
    close(descRequestPipe);
    perror("main->write_EXIT_req->write");
    return EXIT_FAILURE;

}

char* int_en_char(int x){
  char *month = malloc(3);
  switch(x){
    case 0:
      month = "00";
      break;
    case 1:
      month = "01";
      break;
    case 2:
      month = "02";
      break;
    case 3:
      month = "03";
      break;
    case 4:
      month = "04";
      break;
    case 5:
      month = "05";
      break;
    case 6:
      month = "06";
      break;
    case 7:
      month = "07";
      break;
    case 8:
      month = "08";
      break;
    case 9:
      month = "09";
      break;
    default :
      sprintf(month,"%d",x);
  }
  return month;
}

int read_CR_reply(char *descReplyPipeChar){

  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;

  uint16_t reptype;
  uint64_t taskid;

  if(read(descReplyPipe,&reptype,2) == -1) goto error_read;
  if(read(descReplyPipe,&taskid,8) == -1) goto error_read;

  //if(taskid > 50) taskid = 0; // REGLER CE SOUCIS
  taskid = be64toh(taskid);
  printf("%ld\n",taskid);

  close(descReplyPipe);
  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->read_CR_answr->open");
    return EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_CR_answr->read");
    return EXIT_FAILURE;

}

int read_LS_reply(char *descReplyPipeChar){

  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;

  uint16_t reptype;
  uint32_t nbtasks;
  if(read(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_read;
  if(read(descReplyPipe,&nbtasks,sizeof(uint32_t)) == -1) goto error_read;
  nbtasks = be32toh(nbtasks);

  for(int i = 0; i < (int)nbtasks ; i++){

    //TO BE PRINTED
    uint64_t taskid;
    char *timingString = malloc(TIMING_TEXT_MIN_BUFFERSIZE);

    //SECONDARY
    uint64_t minutes;
    uint32_t hours;
    uint8_t daysofweek;
    timing *time = malloc(sizeof(timing));
    uint32_t argcCommandline;

    if(read(descReplyPipe,&taskid,sizeof(uint64_t)) == -1){
      free(timingString);
      goto error_read;
    }
    if(read(descReplyPipe,&minutes,sizeof(uint64_t)) == -1) {
      free(timingString);
      goto error_read;
    }
    if(read(descReplyPipe,&hours,sizeof(uint32_t)) == -1){
      free(timingString);
      goto error_read;
    }
    if(read(descReplyPipe,&daysofweek,sizeof(uint8_t)) == -1){
      free(timingString);
      goto error_read;
    }

    taskid = be64toh(taskid);
    time->minutes = be64toh(minutes);
    time->hours = be32toh(hours);
    time->daysofweek = daysofweek;

    if(timing_string_from_timing(timingString,time) == -1){
      free(time);
      free(timingString);
      goto error;
    }

    free(time);

    //ICI RECCUP LA COMMANDELINE IZAN
    if(read(descReplyPipe,&argcCommandline,sizeof(uint32_t)) == -1){
      free(timingString);
      goto error_read;
    }
    argcCommandline = be32toh(argcCommandline);

    char *argv = malloc(1024); // TO BO PRINTED TOO
    int cmp = 0;

    for(int j = 0; j < (int)argcCommandline; j++){

      uint32_t sizeargvuint;
      if(read(descReplyPipe,&sizeargvuint,sizeof(uint32_t)) == -1){
        free(timingString);
        free(argv);
        goto error_read;
      }
      int sizeargvint = (int)be32toh(sizeargvuint);

      for(int k = 0; k < sizeargvint; k++){
        if(read(descReplyPipe,argv+cmp,1) == -1){
          free(timingString);
          free(argv);
          goto error;
        }
        cmp++;
      }
      argv[cmp++] = ' ';

    }

    printf("%ld: %s %s\n",taskid,timingString,argv);
    free(timingString);

  }

  close(descReplyPipe);
  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->read_LS_answr->open");
    return  EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_LS_answr->read");
    return EXIT_FAILURE;

  error:
    close(descReplyPipe);
    return EXIT_FAILURE;
}

int read_TX_reply(char* descReplyPipeChar){
  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;
  uint16_t reptype;

  if(read(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_read;
  reptype = be16toh(reptype);
  if (reptype ==SERVER_REPLY_OK){
    uint32_t nbruns;
    if(read(descReplyPipe,&nbruns,sizeof(uint32_t)) == -1) goto error_read;
    nbruns = be32toh(nbruns);

    for (int i =0; i <nbruns;i++){
      //printf("%d\n",i);
      time_t temps;
      uint16_t exitcode;
      if(read(descReplyPipe,&temps,sizeof(int64_t)) == -1) goto error_read;
      if(read(descReplyPipe,&exitcode,sizeof(uint16_t)) == -1) goto error_read;
      exitcode = be32toh(exitcode);
      temps = be64toh(temps);
      struct tm tempstm;
      localtime_r(&temps,&tempstm);
      //printf("%d",tempsstr->tm_year);
      char* moisstr = int_en_char(tempstm.tm_mon +1);
      char * jourstr = int_en_char(tempstm.tm_mday);
      char * heurestr = int_en_char(tempstm.tm_hour);
      char* minstr = int_en_char(tempstm.tm_min);
      char * secstr = int_en_char(tempstm.tm_sec);
      printf("%d-%s-%s %s:%s:%s %0d\n" ,tempstm.tm_year +1900,moisstr,jourstr,heurestr,minstr,secstr,exitcode);

    }
  }
  else{
    uint16_t errcode;
    if(read(descReplyPipe,&errcode,sizeof(uint16_t)) == -1) goto error_read;
    errcode = be16toh(errcode);
    //Pour ce test je suis pas tout à fait sur de ce qu'il faut renvoyer
    //j'ai écrit errcode sur la sortie d'ereur standard
    dprintf(2,"NF\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->read_TX_answr->open");
    return  EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_TX_answr->read");
    return EXIT_FAILURE;

}

int read_O_reply(char* descReplyPipeChar){

  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;
  uint16_t reptype;
  if(read(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_read;
  reptype = be16toh(reptype);
  if (reptype == SERVER_REPLY_OK){
    uint32_t taille;
    if(read(descReplyPipe,&taille,sizeof(uint32_t)) == -1) goto error_read;
    taille = be32toh(taille);
    char * output = malloc(taille);
    if (output == NULL) goto error;
    if(read(descReplyPipe,output,taille) == -1){
      free(output);
      goto error_read;
    }
    printf("%s\n",output);
    free(output);
  }
  else{
    uint16_t errcode;
    if(read(descReplyPipe,&errcode,sizeof(uint16_t)) == -1) goto error_read;
    errcode = htobe16(errcode);
    if (errcode == SERVER_REPLY_ERROR_NOT_FOUND)
      dprintf(2,"NF\n");
    else
      dprintf(2,"NR\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

  error :
    close(descReplyPipe);
    return EXIT_FAILURE;

  error_open:
    if (errno != 0) perror("main->read_O_answr->open");
    return  EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_O_answr->read");
    return EXIT_FAILURE;

}

int read_RM_reply(char *descReplyPipeChar){

  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;

  uint16_t reptype;  
  if(read(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_read;
  reptype = be16toh(reptype);
  if (reptype == SERVER_REPLY_ERROR){
    dprintf(2,"NF\n");
  }

  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->read_RM_answr->open");
    return  EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_RM_answr->read");
    return EXIT_FAILURE;

}

int read_EXIT_reply(char *descReplyPipeChar){

  int descReplyPipe = open(descReplyPipeChar,O_RDONLY);
  if(descReplyPipe == -1) goto error_open;

  uint16_t reptype;  
  if(read(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_read;

  return EXIT_SUCCESS;

  error_open:
    if (errno != 0) perror("main->read_EXIT_answr->open");
    return  EXIT_FAILURE;

  error_read:
    close(descReplyPipe);
    if (errno != 0) perror("main->read_EXIT_answr->read");
    return EXIT_FAILURE;

}
