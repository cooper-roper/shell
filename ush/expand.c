#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h>

#include "defn.h"

#include "globals.h"


/* Prototypes */
int expand (char *orig, char *new, int newsize);


/* Methods */
int expand (char *orig, char *new, int newsize){

  pid_t cpid;
  int  p[2];
  int  i, j, k;
  int  ix = 0;
  int  brkt = 0;
  int  cmdi= -1;
  int  len = strlen(orig);
  char env[len];
  char buffer[newsize];
  char wild[len];
  int  envi = 0;
  int  arglen;
  int  parc = 0;

  
  for(i = 0; i < len; i++){


    //checking for expansion overflow error
    if(ix > newsize){
      printf("expansion overflow error\n");
      return -1;
    }
    ///////////////////////////////////////


    //if $ found...
    else if(orig[i] == '$'){
      // writes the pid ///////////////////////
      if(orig[i+1] == '$'){
	int pid = (int)getpid();
	char pidstr[10];
	sprintf(pidstr,"%d",pid);
	arglen = strlen(pidstr);
	for(j = 0; j < arglen; j++){
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  *new++ = pidstr[j];
	  ix++;
	}
	i++;
      }
      ///////////////////////////////////////

      // writes the exit status ////////////
      
      else if (orig[i+1] == '?'){
	char waitstr[10];
	sprintf(waitstr, "%d", waitstat);
	arglen = strlen(waitstr);
	for(j = 0; j < arglen; j++){
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  *new++ = waitstr[j];
	  ix++;
	}
	i++;
      }
      
      ///////////////////////////////////////
      
      // starts to read the env /////////////
      else if (orig[i+1] == '{'){
	brkt = 1;
	i++;
      }
      ///////////////////////////////////////

      // starts command processing /////////////
      else if (orig[i+1] == '('){
	if(cmdi == -1) cmdi = i+2;
	parc++;
	
      }
      ///////////////////////////////////////

      //writes the number of args
      else if (orig[i+1] == '#'){
	char argnum[3];
	int argint = argc - shift -1;
	if(argint == 0) argint = 1;
	sprintf(argnum,"%d", argint);
	arglen = strlen(argnum);
	for(j = 0; j < arglen; j++){
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  *new++ = argnum[j];
	  ix++;
	}
	i++;
      }
      ///////////////////////////////////////
      
      // returns the arguement in that position //
      else if (isdigit(orig[i+1])){
	int num = (orig[i+1] - '0') + 1;
	char *arg = "\0";
	if(argc == 1)arg = "./ush";
	else if(num + shift < argc)arg = argv[num+shift];
	arglen = strlen(arg);
	for(j = 0; j < arglen; j++){
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  *new++ = arg[j];
	  ix++;
	}
	i++;
      }
      /////////////////////////////////////////
      
      //writes the dollar sign ///////////////
      else {
	*new++ = orig[i];
	ix++;
      }
      ///////////////////////////////////////
    }

    
    //writes the environment ///////////////
    else if(orig[i] == '}' && brkt){
      env[envi++] = '\0';
      char *theEnv = getenv(env);
      if (theEnv != NULL){
	arglen = strlen(theEnv);
	for(j = 0; j < arglen; j++){
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  *new++ = theEnv[j];
	  ix++;
	}
      }
      brkt = 0;
      envi = 0;
    }
    ///////////////////////////////////////

    // starts to read the command expansion /////////////
    else if (orig[i] == ')' && parc > 0){

      parc--;

      //if found matching par
      if(parc == 0){
	orig[i] = '\0';

	//pipe
	if (pipe (p) < 0)  perror("pipe");

	//call processline to write output to pipe
	cpid = processline(&orig[cmdi], 0, p[1], NOWAIT);
	
	close(p[1]);
	//char by char until EOF
	int size = read(p[0], buffer, sizeof(buffer));
	j = 0;
	while(j < size){
	  if(buffer[j] == '\n') buffer[j] = ' ';
	  if(ix > newsize){
	    printf("expansion overflow error\n");
	    return -1;
	  }
	  ix++;
	  *new++ = buffer[j++];
	}
	close(p[0]);

	if(cpid > 0){
	  if(waitpid(cpid, &waitstat, 0) < 0)
	    perror ("wait");
	}
	
	cmdi = -1;
      }
      orig[i] = ')';
    }
     ///////////////////////////////////////
    
    //copying the env
    else if(brkt){
      env[envi++] = orig[i];
    }
    ///////////////////////////////////////

    else if(parc > 0){
      continue;
    }

    //finds files in directory////////////
    else if(orig[i] == '*' && orig[i-1] == ' '){
      char* wildptr = NULL;

      //finds certain files//
      if(orig[i+1] != ' '){
	k = 0;
	i++;
	while(orig[i] != ' '){
	  wild[k++] = orig[i];
	  i++;
	}
	wild[k] = '\0';
	wildptr = wild;
      }
      ////////////////////////
      
      DIR *stream =  opendir(".");
      struct dirent *file;
      readdir(stream); // current dir "."
      readdir(stream); // prev dir ".."
      while((file = readdir(stream)) != NULL){
	arglen = strlen(file -> d_name);
	if((wildptr == NULL) ||
	   (strstr(file -> d_name, wildptr) != NULL)){
	  for(j = 0; j < arglen; j++){
	    if(ix > newsize){
	      printf("expansion overflow error\n");
	      return -1;
	    }
	    *new++ = file -> d_name[j];
	    ix++;
	  }
	}
	*new++ = ' ';
	ix++;
      }
      closedir(stream);
    }
    ///////////////////////////////////////
    
    //writes whatever char after the '\' ////
    else if(orig[i] == '\\' && orig[i-1] == ' '){
      if(ix > newsize){
	printf("expansion overflow error\n");
	return -1;
      }
      *new++ = orig[i+1];
      ix++;
      i++;
    }
    ///////////////////////////////////////

    // do nothing /////////////////////////
    else{
      *new++ = orig[i];
      ix++;
    }
    ///////////////////////////////////////
  }

  
  *new++ = '\0';
  if(brkt){
    printf("mismatch bracket\n");
    return -1;
  }
  return 0;
}
