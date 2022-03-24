 /* CS 352 -- Micro Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Jan 7, 2022. Cooper Roper
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *   Modified January 8, 2017
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "defn.h"

#define DEFINE_GLOBALS
#include "globals.h"


/* Constants */ 

#define LINELEN 200000

/* Prototypes */

int processline (char *line, int infd, int outfd, int flags);
char ** arg_parse (char *line, int *argcptr);


/* Shell main */

int main (int mainargc, char **mainargv)
{
    char    buffer [LINELEN];
    FILE    *file = NULL;
    char    *fptr = mainargv[1];
    int     fread = 0;
    size_t  len = 0;

    argc = mainargc;
    argv = mainargv;

    if (fptr){
      if((file = fopen(fptr, "r")) == NULL){
	exit(127);
      }
      fread = 1;
    }

    while (1) {
      
      /* prompt and get line */
      if(fread){
	if(fgets(buffer, LINELEN, file) == NULL){
	    fread = 0;
	    fclose(file);
	    exit(0);
	  }
      }
      
      else{
	fprintf (stderr, "%% ");
	if (fgets (buffer, LINELEN, stdin) != buffer)
	  break;
      }
      
      /* Get rid of \n at end of buffer. */
      len = strlen(buffer);
      if (buffer[len-1] == '\n')
	buffer[len-1] = 0;
      
      if((buffer[0] == '#') || buffer[0] == '\0') continue;
      else
	for(int i = 1; i < len; i++){
	  if(buffer[i] == '#' && buffer[i-1] != '$') buffer[i] = 0;
	}
	
      /* Run it ... */
      processline (buffer, 0, 1, WAIT);

    }

    if (!feof(stdin))
      perror ("read");

    return 0;		/* Also known as exit (0); */
}

char ** arg_parse (char *line, int *argcptr)
{
	/* args array with 64 chars */
	int argnum = 0, 
	ready = 0, 
	qswitch = 0,
	index = 0,
	len = strlen(line);
	char **args = (char **)malloc(64 * sizeof(char *));	
	
	/* loop through each char in line*/
	for(int i = 0; i < len; i++){
		
		char arg = line[i];
		
		if(arg == '"'){
			qswitch = 1 - qswitch;
			continue;
                }
		
		line[index++] = arg;
		
		/* if char is blank and prev char was not, add end string to line */
		if((arg == ' ' || arg == '\n' || arg == '\t') && !qswitch){	
			if(ready) line[index-1] = '\0';
			ready = 0;
		}

		/* if char is not blank and prev char was blank or invalid, add to arg*/ 
                else if(!ready){
                        args[argnum++] = &line[index-1];
                        ready = 1;
                }
	}
	/* add null to end of args and write number of args*/
	if(qswitch) {
		fprintf(stderr, "Mismatch Quotes\n");
		argnum = 0;
	}
	line[index] = '\0';
	args[argnum] = NULL;
	*argcptr = argnum;
	return args;
}


int processline (char *line, int infd, int outfd, int flags)
{
    char   new[LINELEN];
    pid_t  cpid;
    char   **args;
    int    argsc;
    int    ex = 0;
    int    check = 1;
    int    p[2];
    int    i; 
    int    rv = 0;
    char   *cmd = NULL;
    int    count = 0;

    signal(SIGINT, SIG_IGN);

    /* Start a new process to do the job. */
    if(flags != NOEXPAND) ex = expand(line, new, LINELEN);
    else strcpy(new, line);


    int len = strlen(new);
    for(i = 0; i < len; i++){
      //find '|'
      if(new[i] == '|'){
        count++;
      }
    }

    if(count > 0){
      char *arr[count+1];
      int  c = 0;
      for(i = 0; i < len; i++){
	//find '|'
	if(cmd  == NULL){
	  cmd = &new[i];
	  arr[c++] = cmd;
	}
	if(new[i] == '|'){
	  new[i] = '\0';
	  cmd = NULL;
	}
      }

      flags = NOEXPAND;
      for(i = 0; i < count + 1; i++){
	if(i == count){
	  flags = WAIT;
	  rv = processline(arr[i], infd, outfd, flags);
	  return rv;
	}
	if (pipe (p) < 0) perror("pipe");
	rv = processline(arr[i], infd, p[1], flags);
	close(p[1]);
	if(infd != 0) close(infd);
	infd = p[0];

	if(waitpid(rv, &waitstat,0) < 0)
	  perror ("wait");
	
      }
    }
    

    if(ex == -1) return -1;
    args = arg_parse(new, &argsc);

    if(args > 0) check = check_bi(args, argsc, outfd);
    else return -1;

    if(!check) return -1;
    
    cpid = fork();
    if (cpid < 0) {
      /* Fork wasn't successful */
      perror ("fork");
      return -1;
     }
    
    /* Check for who we are! */
    if (cpid == 0) {
      /* We are the child! */

      dup2 (infd, 0);
      
      dup2 (outfd, 1);
      
      execvp (args[0], args);

      /* execvp returned, wasn't successful */
      perror ("exec");
      fclose(stdin);  // avoid a linux stdio bug
      exit (127);
    }

    free(args);
    
    /* Have the parent wait for child to complete */
    
    if (flags == WAIT) {
      /* Wait wasn't successful */

      if(waitpid(cpid, &waitstat,0) < 0)
	perror ("wait");
      
      if(WIFEXITED(waitstat)){
	WEXITSTATUS(waitstat);
      }
      
      else if(WIFSIGNALED(waitstat)){
	waitstat = WTERMSIG(waitstat) + 128;
	if(WTERMSIG(waitstat) !=  SIGINT) strsignal(WTERMSIG(waitstat));
	if(WCOREDUMP(waitstat)) printf("coredump\n");
      }
      
      return 0;
    }

    return cpid;
}

