#include "saturnd.h"

//GROSSE CONCLUSION IL FAUT OUVRIR UNE FOIS POUR UNE LECTURE DIRECTEMET
#define PIPESDIR "./run/pipes"
#define REQUEST_PIPE "saturnd-request-pipe"
#define REPLY_PIPE "saturnd-reply-pipe"

int taskidint = 0;

int main(int argc, char * argv[]) {

  create_fifo();

  char *descReplyPipeChar = malloc(100);
  char *descRequestPipeChar = malloc(100);
  if(descReplyPipeChar == NULL || descRequestPipeChar == NULL) goto error;

  /*  REVENIR SUR CE PROBLEME DE CHEMIN 
  if(pipes_directory == NULL){
    
    pipes_directory = malloc(100);
    if(pipes_directory==NULL) goto error;

    strcat(strcpy(pipes_directory,"/tmp/"),getenv("USER"));
    strcat(pipes_directory,"/saturnd");

    strcat(strcpy(descWchar,pipes_directory),"/saturnd-request-pipe");
    strcat(strcpy(descRchar,pipes_directory),"/saturnd-reply-pipe");
    
    free(pipes_directory);
  } else {
    */
  strcat(strcpy(descRequestPipeChar,"run/pipes"),"/saturnd-request-pipe");
  strcat(strcpy(descReplyPipeChar,"run/pipes"),"/saturnd-reply-pipe");
  //}

  if(create_tasksdir() == EXIT_FAILURE){
    write(2,"saturnd->main->create_taskdir()",30);
    goto error;
  }

  int descRequestPipe = open(descRequestPipeChar,O_RDONLY);
  int descReplyPipe = open(descReplyPipeChar,O_WRONLY);
  assert(descRequestPipe != -1);
  assert(descReplyPipe != -1);

  uint16_t opcode =  be16toh(getOPCODE(descRequestPipe));

  while(opcode != CLIENT_REQUEST_TERMINATE){ // code de terminate 0x4b49

    if(opcode ==  CLIENT_REQUEST_LIST_TASKS){ //LS
      int test = write_LS_reply(descReplyPipe);
      close(descReplyPipe);
      if(test == EXIT_FAILURE){
        printf("failure de write LS\n");
        goto error;
      }
    }
    else if(opcode == CLIENT_REQUEST_CREATE_TASK) { //CR
      int test1 = read_CR_request(descRequestPipe);
      close(descRequestPipe);
      if(test1 == EXIT_FAILURE){
        printf("failure de read CR\n");
        goto error;
      }
      int test2 = write_CR_reply(descReplyPipe);
      close(descReplyPipe);
      if(test2 == EXIT_FAILURE){
        printf("failure de write CR\n");
        goto error;
      }
    }
    else if(opcode == CLIENT_REQUEST_REMOVE_TASK){ // RM
      int exitcode = read_RM_request(descRequestPipe);
      close(descRequestPipe);
      int test = write_RM_reply(descReplyPipe,exitcode);
      close(descReplyPipe);
      if(test == EXIT_FAILURE){
        printf("failure de write RM\n");
        goto error;
      }
    }
    else if(opcode == CLIENT_REQUEST_GET_TIMES_AND_EXITCODES){ //TX

    }
    else if(opcode == CLIENT_REQUEST_GET_STDOUT){//SO

    }
    else if(opcode == CLIENT_REQUEST_GET_STDERR){ // SE

    }
    else if(opcode == CLIENT_REQUEST_TERMINATE){//TM

    } else {
      printf("on est dans le esle\n");
    }

    descRequestPipe = open(descRequestPipeChar,O_RDONLY);
    assert(descRequestPipe != -1);
    descReplyPipe = open(descReplyPipeChar,O_WRONLY);
    assert(descRequestPipe != -1);
    assert(descReplyPipe != -1);
    opcode =  be16toh(getOPCODE(descRequestPipe));
    
  }
  


  return EXIT_SUCCESS;
  
 error:
  if (errno != 0) perror("saturnd->main");
  //free(pipes_directory);
  //pipes_directory = NULL;
  close(descRequestPipe);
  close(descReplyPipe);
  free(descRequestPipeChar);
  free(descReplyPipeChar);
  return EXIT_FAILURE;

}

//TODO : FAIRE LES FOCNTION POUR CREER LES PIPES SI ILS N'EXISTENT PAS 
/*int read_STDOUT_request(int descRequestPipe){

  uint64_t taskid;
  if(read(descRequestPipe,&taskid,sizeof(uint64_t)) == -1) return EXIT_FAILURE;
  taskid = be64toh(taskid);
  int taskidint = (int) taskid;

  // TODO : FAIRE EN SORTIE DE RECCUP CE QUI A ETE AFFICHER SUR LA SORTIE STANDRARD PAR LA TACHE TASKID

}*/

/*int read_STDERR_request(int descRequestPipe){

  uint64_t taskid;
  if(read(descRequestPipe,&taskid,sizeof(uint64_t)) == -1) return EXIT_FAILURE;
  taskid = be64toh(taskid);
  int taskidint = (int) taskid;

  // TODO : FAIRE EN SORTIE DE RECCUP CE QUI A ETE AFFICHER SUR LA SORTIE D'ERREUR STANDRARD PAR LA TACHE TASKID

}*/

int read_RM_request(int descRequestPipe){

  uint64_t taskid;
  if(read(descRequestPipe,&taskid,sizeof(uint64_t)) == -1) return EXIT_FAILURE;
  taskid = be64toh(taskid);
  int taskidint = (int) taskid;

  DIR *dirp = opendir("tasks");
  struct dirent *entry;
  while((entry = readdir(dirp))){
    if(atoi(entry->d_name) == taskidint && (entry->d_name[0] != '.')){
      char *pathfile = malloc(15);
      strcat(strcpy(pathfile,"tasks/"),entry->d_name);
      printf("%s\n",pathfile);
      if(remove(pathfile) == -1) goto error; 
      free(pathfile);
      return EXIT_SUCCESS;
    }
  }
  return EXIT_FAILURE;

  error:
    if (errno != 0) perror("saturnd->read_RM_req");
    return EXIT_FAILURE;
}

int read_CR_request(int descRequestPipe){

  timing *time = malloc(sizeof(timing)); 
  if(time == NULL) goto error;

  char *timing_string = malloc(TIMING_TEXT_MIN_BUFFERSIZE); /*!!!*/
  if(timing_string == NULL) goto error;

  uint64_t minutes;
  uint32_t hours;
  uint8_t daysofweek;

  if(read(descRequestPipe,&minutes,sizeof(uint64_t)) == -1) goto error_read;
  if(read(descRequestPipe,&hours,sizeof(uint32_t)) == -1)goto error_read;
  if(read(descRequestPipe,&daysofweek,sizeof(uint8_t)) == -1)goto error_read;

  time->minutes = be64toh(minutes);
  time->hours = be32toh(hours);
  time->daysofweek = daysofweek;

  timing_string_from_timing(timing_string,time);
  
  uint32_t argc;
  if(read(descRequestPipe,&argc,sizeof(uint32_t)) == -1)goto error_read;

  char *buf = malloc(BUFSIZE); /*!!!*/
  if(buf == NULL) goto error;
  int cmp = 0;

  //lecture de la cmd line
  for(int i = 0; i < (int)be32toh(argc); i++){

    uint32_t lengthString;
    if(read(descRequestPipe,&lengthString,sizeof(uint32_t)) == -1){
      free(buf);
      goto error_read;
    }
    int lengthStringInt = (int)be32toh(lengthString);

    for(int k = 0; k < lengthStringInt; k++){
      if(read(descRequestPipe,buf+cmp,1) == -1) {
        free(buf);
        goto error;
      }
      cmp++;
    }
    buf[cmp++] = ' ';
  }
  buf[cmp] = '\0';

  if(create_file_task(timing_string,buf,taskidint) == EXIT_FAILURE) goto error_writing;

  free(timing_string);
  free(time);
  free(buf);

  return EXIT_SUCCESS;

  error_read:
    free(timing_string);
    free(time);
    if (errno != 0) perror("saturnd->read_CR_req->read");
    return EXIT_FAILURE;
  
  error:
    free(time);
    if (errno != 0) perror("saturnd->read_CR_req");
    return EXIT_FAILURE;

  error_writing:
    if (errno != 0) perror("saturnd->read_CR_req->create_file_task");
    return EXIT_FAILURE;

}

//int write_O_reply(int descRequestPipe){}

int write_RM_reply(int descReplyPipe,int exitcode){

  if(exitcode == EXIT_SUCCESS){
    uint16_t reptype = SERVER_REPLY_OK;
    reptype = htobe16(reptype);
    if(write(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_write;
    return EXIT_SUCCESS;
  }else {
    uint16_t reptype = SERVER_REPLY_ERROR;
    reptype = htobe16(reptype);
    if(write(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_write;

    uint16_t errcode = SERVER_REPLY_ERROR_NOT_FOUND;
    errcode = htobe16(errcode);
    if(write(descReplyPipe,&errcode,sizeof(uint16_t)) == -1) goto error_write;

    return EXIT_SUCCESS;
  }

  error_write:
    if (errno != 0) perror("saturnd->write_RM_rep->write");
    return EXIT_FAILURE;

}

int write_TM_reply(int descReplyPipe){

  uint16_t reptype = SERVER_REPLY_OK;
  reptype = htobe16(reptype);
  if(write(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_write;

  return EXIT_FAILURE;

  error_write:
    if (errno != 0) perror("saturnd->write_TM_rep->write");
    return EXIT_FAILURE;

}

int write_LS_replyBIS(int descReplyPipe,int argc,char *argv[],char *minutes_str,char *hours_str,char *daysofweek_str){
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
    char *cmdargv[argc];
    for(int i = 0; i < argc; i++){
      cmdargv[i] = argv[i];
    }
    char *buf = malloc(BUFSIZE);
    if(buf == NULL) goto error;

    memcpy(buf,&minutes,sizeof(uint64_t));
    memcpy(buf+8,&hours,sizeof(uint32_t));
    memcpy(buf+12,&daysofweek,sizeof(uint8_t));
    memcpy(buf+13,&cmdargc,sizeof(uint32_t));

    int i = 0;
    int cmp = 0;
    while(i < argc){

      int j = 0;

      //Longeur de l'argument :
      int taille = strlen(cmdargv[i]);
      uint32_t length = htobe32((uint32_t) taille);
      memcpy(buf+17+cmp,&length,4);
      cmp += 4;

      //Contenue de l'argument :
      while(j < taille){
        uint8_t aecrire = (uint8_t)cmdargv[i][j];
        memcpy(buf+17+cmp,&aecrire,sizeof(aecrire));// sizeof(aecrire) renvoie normalement tjr 1 car c'est un char
        j++;
        cmp++;
      }
      i++;
    }

    if(write(descReplyPipe,buf,17+cmp) < 0){
      free(buf);
      goto error;
    }

    free(buf);

    return EXIT_SUCCESS;

  }
  error:
      if (errno != 0) perror("gdfsqeq");
      return EXIT_FAILURE;

}

int write_LS_reply(int descReplyPipe){

  uint16_t reptype = SERVER_REPLY_OK;

  int nbtasksint = count_tasks("tasks");

  uint32_t nbtasks = htobe32((uint32_t) nbtasksint);

  if(write(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_write;
  if(write(descReplyPipe,&nbtasks,sizeof(uint32_t)) == -1) goto error_write;

  DIR *dirp = opendir("tasks");
  struct dirent *entry;
  while((entry = readdir(dirp))){

    char *filepath = malloc(15);
    strcat(strcpy(filepath,"tasks/"),entry->d_name);

    if(filepath[6] != '.'){
      int fd = open(filepath,O_RDONLY);
      char *buf = malloc(1024);
      read(fd,buf,1024);
      int tid = atoi(entry->d_name);
      uint64_t tidu = htobe64((uint64_t) tid);

      if(write(descReplyPipe,&tidu,sizeof(uint64_t)) <= 0) goto error_write;

      char **tab = str_split(buf,'\n');
      char **timing = str_split(*(tab+0),' ');
      char **cmdline = str_split(*(tab+1),' ');
      int argccmdline = get_argc_cmdline(cmdline);

      write_LS_replyBIS(descReplyPipe,argccmdline,cmdline,*(timing+0),*(timing+1),*(timing+2));
      
      close(fd);
      free(buf);
      free(filepath);
    }

  }

  return EXIT_SUCCESS;
  
  error_write:
    if (errno != 0) perror("saturnd->write_CR_rep->write");
    return EXIT_FAILURE;
}

int write_CR_reply(int descReplyPipe){

  uint16_t reptype = SERVER_REPLY_OK;
  uint64_t taskid = (uint64_t) taskidint;

  reptype = htobe16(reptype);
  taskid = htobe64(taskid);

  if(write(descReplyPipe,&reptype,sizeof(uint16_t)) == -1) goto error_write;
  if(write(descReplyPipe,&taskid,sizeof(uint64_t)) == -1) goto error_write;

  taskidint++;

  return EXIT_SUCCESS;

  error_write:
    if (errno != 0) perror("saturnd->write_CR_rep->write");
    return EXIT_FAILURE;

}

int get_argc_cmdline (char **cmdline){
    int cmp=0;
    for(int i=0;*(cmdline+i);i++) cmp++;
    return  cmp;
}

int create_file_task(char *time,char *cmd,int id){

  char *filename = malloc(15);
  char *taskidstring = malloc(3);
  sprintf(taskidstring,"%d",id);

  strcat(strcat(strcpy(filename,"tasks/"),taskidstring),".txt");
  char *tobewrited = malloc(100);
  strcat(strcat(strcpy(tobewrited,time),"\n"),cmd);

  int fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,00700);
  if(fd == -1) return EXIT_FAILURE;
  
  if(write(fd,tobewrited,strlen(tobewrited)) == -1) return EXIT_FAILURE;

  close(fd);
  return EXIT_SUCCESS;

}

timing *get_task_timing(char *timingline){

  timing *timing = malloc(sizeof(struct timing));
  char *minutes = malloc(20);
  char *hours = malloc(20);
  char *daysofweek = malloc(20);

  char **timing_line_splited = str_split(timingline,' ');

  free(minutes);free(hours);free(daysofweek);
  
  strcpy(minutes,*(timing_line_splited + 0));
  strcpy(hours,*(timing_line_splited + 1));
  strcpy(daysofweek,*(timing_line_splited + 2));

  assert(timing_from_strings(timing,minutes,hours,daysofweek) != -1);

  return timing;

}

uint16_t getOPCODE(int descRequestPipe){

  uint16_t opcode;
  int tmp = read(descRequestPipe,&opcode,sizeof(uint16_t));
  assert(tmp != -1);
  return opcode;

}

void create_fifo(){
  if(checkIfDirectoryExists("run/pipes/") == EXIT_FAILURE){
    int mkdirtest1 = mkdir("run",00777);
    assert(mkdirtest1 != -1);
    int mkdirtest2 = mkdir("run/pipes",00777);
    assert(mkdirtest2 != -1);
  }
  mkfifo("run/pipes/saturnd-reply-pipe",00600);
  mkfifo("run/pipes/saturnd-request-pipe",00600);
}

int create_tasksdir(){

  if(checkIfDirectoryExists("tasks") == EXIT_FAILURE){
    if(mkdir("tasks",00777) == -1) return EXIT_FAILURE; // échec
    return EXIT_SUCCESS; // réussite
  } else {
    return EXIT_SUCCESS; // réussite
  }

}

int checkIfDirectoryExists(const char* filename){
    struct stat st;
    int exist = stat(filename,&st);
    if(exist == 0){
      if(S_ISDIR(st.st_mode)){
        return EXIT_SUCCESS;//réussite
      } else {
        return EXIT_FAILURE;//échec
      }
    }
    else{
      return EXIT_FAILURE;//échec
    }
}

int count_tasks(char *path){
  int cmp = -2; // pour eviter de prendre en compte le repoertorie courant et le repertoire précedent
  DIR *dirp = opendir(path);
  while(readdir(dirp)){
    cmp++;
  }
  closedir(dirp);
  return cmp;
}

char** str_split(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc((sizeof(char*) * count));

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        idx++;
        *(result + idx) = 0;
    }

    return result;
}
