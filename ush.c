/* CS 352 -- Micro Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *   Modified January 8, 2017
 *   
 *   JohnHenry Ward | CSCI 347
 *   Assignment 4, 5/20/20
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "defn.h"
#define DEFINE_GLOBALS
#include "globals.h"

/* Constants */ 

#define LINELEN 200000

/* Prototypes */

char** arg_parse (char *line, int *argcptr);
void printargs (int argc, char **argv);
void removeQuotes (char *src);
int countPipe (char *newline);
int hasPipe (char *line);
int pipelines (char *newline, int infd, int outfd, int flags);
void killZombies();

/* Shell main */

int
main (int margc, char **margv)
{

    /* Catch a ctrl-C */
    signal(SIGINT, catchSigInt);
    
    char   buffer [LINELEN];
    int    len;
    FILE   *f;
    
    /* Globals */
    if(margc > 1){
      mainargc = margc-1;
    }
    else{
      mainargc = 1;
    }
    mainargv = margv;
    /* Keep track of original margc */
    og_argc = margc;
    
    /* Read from file */
    if(margc > 1){
      f = fopen(mainargv[1], "r");
      if(f == NULL){
	fprintf(stderr, "Error opening file: %s\n", strerror(errno));
	exit(127);
      }
    }
    
    while (1) {

      /* Read from standard in */
      if(margc == 1){
        fprintf (stderr, "%% ");
	f = stdin;
      }

      /* Get input, make sure it's valid */
      if(fgets(buffer, LINELEN, f) != buffer){
	break;
      }
	
	/* Remove comments from buffer */
	int index = 0;
	while(buffer[index] != 0){
	  if(buffer[index] == '#' && buffer[index-1] != '$'){
	    buffer[index] = 0;
	    break;
	  }
	  index++;
	}

	/* Get rid of \n at end of buffer. */
	len = strlen(buffer);
	
	if (buffer[len-1] == '\n'){
	  buffer[len-1] = 0;
	}

	/* Run it ... */
	processline (buffer, 0, 1, WAIT | EXPAND);

    }

    if (!feof(f))
      perror ("read");

    return 0;		/* Also known as exit (0); */
}


int processline (char *line, int infd, int outfd, int flags)
{ 
    pid_t  cpid;
    
    char newline[LINELEN];

    killZombies();

    /* Expand */
    if(flags >= 2){
      int result = expand(line, newline, LINELEN);
      /* Failed to expand */
      if(result == -1){
	return -1;
      }
    }
    else{
      strcpy(newline, line);
    }

    /* Pipes */
    if(hasPipe(newline) == 1){
      int pipeCount = countPipe(newline);
      int currIndex = 0;
      int prevIn = 0;
      
      /* Runs for each command */ 
      for(int i = 0; i <= pipeCount; i++){
	int n = 0;
	/* Loop to get each command, seperated by | */ 
	while(newline[currIndex] != '|'){
	  if(newline[currIndex] == 0){
	    break;
	  }
	  if(newline[currIndex] == ' ' && (newline[currIndex+1] == '|' || newline[currIndex-1] == '|')){
	    currIndex++;
	  }
	  else{
	    newline[n] = newline[currIndex];
	    n++;
	    currIndex++;
	  }
	}
	newline[n] = 0;
	currIndex++;
	
	int fd[2];
	if(i != pipeCount){
	  /* Create pipe */
	  if(pipe(fd) < 0){
	    fprintf(stderr, "pipe error: %s\n", strerror(errno));
	    return -1;
	  }
	}
	
	/* Call proccessline with correct in and out */
	if(i == 0){
	  processline(newline, infd, fd[1], NOWAIT | NOEXPAND);
	  close(fd[1]);
	  prevIn = fd[0];
	}
	else if(i == pipeCount){
	  processline(newline, prevIn, outfd, NOEXPAND | (flags & WAIT));
	  close(prevIn);
	  killZombies();
	  /* Success */
	  return 0;
	}
	else if(i > 0){
	  processline(newline, prevIn, fd[1], NOWAIT | NOEXPAND);
	  close(prevIn);
	  close(fd[1]);
	  prevIn = fd[0];
	}
      }
    }/* End of Pipes */

    /* Argument parsing */
    int argc = 0;
    char** args = arg_parse(newline, &argc);
    /* No arguments found */
    if(argc == 0){
      return -1;
    }
    
    /* Check for Built In */
    int biResult = isBuiltIn(args[0]);
    
    /* Is a Built In */
    if(biResult != -1){
      exitVal = 0;
      if(runBuiltIn(biResult, args, argc, outfd) != 0){
	exitVal = 1;
	return -1;
      }
      free(args);
    }
    
    /* Not a Built In */
    else{
      
      /* Start a new process to do the job. */
      cpid = fork();
      if (cpid < 0) {
	/* Fork wasn't successful */
	perror ("fork");
	return -1;
      }

      /* Check for who we are! */
      if (cpid == 0) {
	/* If outfd != 1, make it 1 */
	if(outfd != 1){
	  dup2(outfd, 1);
	}
	if(infd != 0){
	  dup2(infd, 0);
	}
	/* We are the child! */
	execvp (newline, args);
	/* execlp reurned, wasn't successful */
	perror ("exec");
	fclose(stdin);  // avoid a linux stdio bug
	exit (127);
      }

      free(args);
      
      if(flags == 1 || flags == 3){
	waitForChild(cpid);
      }
      else{
	return cpid;
      }
      
    }
    return 0;
}

char** arg_parse (char *line, int *argcptr)
{
  
  int index = 0;
  int argc = 0;

  /* Count the total args */
  while(line[index] != 0){
    if(line[index] == ' '){
      index++;
    }
    else{
      argc++;
      while(line[index] != ' ' && line[index] != 0){	
	if(line[index] == '"'){
	  index++;
	  while(line[index] != '"'){
	    index++;
	    if(line[index] == 0){
	      printf("Unmatched quote\n");
	      return 0;
	    }
	  }
	}
	index++;
      }
    }
  }

  
  /* Malloc the args pointer */
  char** args = (char**) malloc(sizeof(char*) * (argc+1));
  
  if(args == NULL){
    perror("Malloc error");
    return 0;
  }

  
  /* Assign pointers and EOS */
  args[argc] = NULL; //make last index null
  
  index = 0;
  int i = 0;
  while(line[index] != 0){
    if(line[index] == ' '){
      index++;
    }
    else{
      args[i] = &line[index];
      i++; //move along the array of char*
      while(line[index] != ' ' && line[index] != 0){
	if(line[index] == '"'){
	  index++;
	  while(line[index] != '"'){
	    index++;
	  }
	}
	index++;
      }
      if(line[index] != 0){
	line[index] = 0;
	index++;
      } 
    }
  }

  /* Remove Quotes */
  for(int i = 0; i < argc; i++){
    removeQuotes(args[i]);
  }

  *argcptr = argc;
  
  return args;
}

void removeQuotes(char* str){
  int src = 0;
  int dst = 0;
  
  while(str[src] != 0){
    if(str[src] != '"'){
      str[dst] = str[src];
      src++;
      dst++;
    }
    else{
      src++;
    }
  }
  
  str[dst] = 0;
}

/* Ctrl-C was pressed, caught */
void catchSigInt(int sigInt){}

int waitForChild(int cpid){
  int status;
  /* Have the parent wait for child to complete */
  if (waitpid (cpid, &status, 0) < 0) {
    /* Wait wasn't successful */
    fprintf(stdout, "wait: %s\n", strerror(errno));
  }
  else{
    /* Catch a ctrl-C */
    signal(SIGINT, catchSigInt);
  
    if(WIFEXITED(status)){
      exitVal = WEXITSTATUS(status);
    }
    else if(WIFSIGNALED(status)){
      exitVal = 128 + WTERMSIG(status);
      if(WCOREDUMP(status)){
	fprintf(stdout, "%s (core dumped)\n", strsignal(WTERMSIG(status)));
      }
      else if(WTERMSIG(status) != SIGINT){
	fprintf(stdout, "%s\n", strsignal(WTERMSIG(status)));
      }
    }
  }
  fflush(stdout);
  return status;
}

int hasPipe(char* line){
  int i = 0;
  while(line[i] != 0){
    if(line[i] == '|'){
      return 1;
    }
    i++;
  }
  return 0;
}

int countPipe(char* cmd){
  int i = 0;
  int count = 0;
  while(cmd[i] != 0){
    if(cmd[i] == '|'){
      count++;
    }
    i++;
  }
  return count;
}

int pipelines(char* newline, int infd, int outfd, int flags){
  int pipeCount = countPipe(newline);
  int currIndex = 0;
  int prevIn = 0;
  
  /* Runs for each command */
  for(int i = 0; i <= pipeCount; i++){
    int n = 0;
    /* Loop to get each command, seperated by | */
    while(newline[currIndex] != '|'){
      if(newline[currIndex] == 0){
	break;
      }
      if(newline[currIndex] == ' ' && (newline[currIndex+1] == '|' || newline[currIndex-1] == '|')){
	currIndex++;
      }
      else{
	newline[n] = newline[currIndex];
	n++;
	currIndex++;
      }
    }
    newline[n] = 0;
    currIndex++;
    
    int fd[2];
    if(i != pipeCount){
      /* Create pipe */
      if(pipe(fd) < 0){
	fprintf(stderr, "pipe error: %s\n", strerror(errno));
	return -1;
      }
    }
    
    /* Call proccessline with correct in and out */
    if(i == 0){
      processline(newline, infd, fd[1], NOWAIT | NOEXPAND);
      close(fd[1]);
      prevIn = fd[0];
    }
    else if(i == pipeCount){
      processline(newline, prevIn, outfd, NOEXPAND | (flags & WAIT));
      close(prevIn);
      killZombies();
      /* Success */
      printf("HERE\n");
      return 0;
    }
    else if(i > 0){
      processline(newline, prevIn, fd[1], NOWAIT | NOEXPAND);
      close(prevIn);
      close(fd[1]);
      prevIn = fd[0];
    }
  }
  return 0;
}

void killZombies(){
  int status;
  waitpid(-1, &status, WNOHANG);
}
