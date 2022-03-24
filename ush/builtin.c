#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h> 
#include <pwd.h>
#include <grp.h>
#include "globals.h"
#include "strmode.c"
#define bi 7

/*prototypes*/
int check_bi(char** args, int argsc, int outfd);

void exit_bi(char** args, int argsc, int outfd);
void envset_bi(char** args, int argsc,int outfd);
void envunset_bi(char** args, int argsc,int outfd);
void cd_bi(char** args, int argsc,int outfd);
void shift_bi(char** args, int argsc,int outfd);
void unshift_bi(char** args, int argsc,int outfd);
void sstat_bi(char** args, int argsc,int outfd);

typedef void (*built_in)(char ** line, int argsc,int outfd);

struct built_in {
  char title[10];
  built_in builtin;
};

  struct built_in arr[bi] = {
    {"exit", exit_bi},
    {"envset", envset_bi},
    {"envunset",envunset_bi},
    {"cd",cd_bi},
    {"shift", shift_bi},
    {"unshift", unshift_bi},
    {"sstat", sstat_bi}
  };

int check_bi(char** args, int argsc, int outfd){

  if(argsc < 1) return 0;
  
  int i;
  for(i = 0; i < bi; i++){
      if( strcmp(args[0], arr[i].title) == 0 ){
	(* arr[i].builtin) (args, argsc, outfd);
	return 0;
      }
  }
  return 1;
}

void exit_bi(char** args, int argsc, int outfd){
  int status = 0;
  if (argsc > 1) 
    status = atoi(args[1]);
  exit(status);
}

void envset_bi(char** args, int argsc, int outfd){
  setenv(args[1], args[2], 1);
  waitstat = 0;
  return;
}

void envunset_bi(char** args, int argsc, int outfd){
  waitstat = 0;
  if (unsetenv(args[1]) == -1){
    printf("%s\n", strerror(errno));
    waitstat = 1;
  }
  return;
}

void cd_bi(char** args, int argsc, int outfd){
  waitstat = 0;
  char * path;
  if(argsc == 1) path = getenv("HOME");
  else path = args[1];
  if (chdir(path) == -1){
    printf("%s\n", strerror(errno));
      waitstat = 1;
  }
  return;
}

void shift_bi(char** args, int argsc, int outfd){
  waitstat = 0;
  if(argsc == 1)shift++;
  else if(atoi(args[1]) >= argc + shift){
    printf("illegal shift\n");
    waitstat = 1;
  }
  else shift = shift + atoi(args[1]);
  return;
}

void unshift_bi(char** args, int argsc, int outfd){
  waitstat = 0;
  if(argsc == 1) shift = shift - shift;
  else if( atoi(args[1]) >  argc - shift){
    printf("invalid shift\n");
    waitstat = 1;
  }
  else shift = shift - atoi(args[1]);
  return;
}

void sstat_bi(char** args, int argsc, int outfd){
  waitstat = 0;

  if(argsc == 1){
    printf("no file");
    waitstat = 1;
    return;
  }

  int i;
  for (i = 1; i < argsc; i++){

    char buf[12];
    struct passwd *pws;
    struct group  *grp;
    struct stat fileStat;
  
    if( stat(args[i], &fileStat) < 0){
      printf("error, file doesn't exist\n");
      waitstat = 1;
      return;
    }



    strmode_bi(fileStat.st_mode, buf);
    buf[10] = '\0';
  
    unsigned long links = fileStat.st_nlink;      
    unsigned long size = fileStat.st_size;

    char *time = ctime(&fileStat.st_mtime);
 

    dprintf(outfd,"%s ", args[i]);    //file name


	   
    if ((pws = getpwuid(fileStat.st_uid)) != NULL)
      dprintf(outfd, "%s ", pws->pw_name);
    else
      dprintf(outfd, "%d ", fileStat.st_uid);


    if ((grp = getgrgid(fileStat.st_gid)) != NULL)
      dprintf(outfd, "%s ", grp->gr_name);
    else
      dprintf(outfd, "%d ", fileStat.st_gid);


    dprintf(outfd, "%s %lu %lu %s",
	   buf,       //permissions
	   links,     //links
	   size,      //size
	   time);     //mod time

  }

  return;
  
}


