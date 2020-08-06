/* builtIn.c
 * 
 * 4/19/29, JohnHenry Ward
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include "defn.h"
#include "globals.h"

typedef int (*funcPtr) (char** args, int argc, int outfd);

/* Prototypes */

int bi_exit();
int bi_envset();
int bi_envunset();
int bi_cd();
int bi_shift();
int bi_unshift();
int bi_sstat();

/* Constants */

const char* builtInNames[] = {"exit", "envset", "envunset", "cd", "shift", "unshift", "sstat", NULL};
const funcPtr builtInFuncs[] = {bi_exit, bi_envset, bi_envunset, bi_cd, bi_shift, bi_unshift, bi_sstat};

/* Functions */

/* Checks is a command is a built in command */
int isBuiltIn(char* line){
  int i = 0;
  while(builtInNames[i] != NULL){
    if(strcmp(line, builtInNames[i]) == 0){
      return i;
    }
    i++;
  }

  /* Did not find builtin */
  return -1;
}

/* Calls correct built in command */
int runBuiltIn(int index, char** args, int argc, int outfd){
  return builtInFuncs[index](args, argc, outfd);
}

/* Built in: Exit */
int bi_exit(char** args, int argc, int outfd){
  if(argc > 2){
    fprintf(stderr, "exit failed: too many arguments\n");
    return -1;
  }
  else if(argc > 1){
    exitVal = 0;
    exit(atoi(args[1]));
  }
  else{
    exit(0);
  }

  /*Success */
  return 0;
}

/* Built in: set environment variable */
int bi_envset(char** args, int argc, int outfd){
  if(argc != 3){
    fprintf(stderr, "envset error: incorrent number of arguments\n");
    return -1;
  }
  else if(setenv(args[1], args[2], 1) != 0){
    fprintf(stderr, "envset error: %s\n", strerror(errno));
    return -1;
  }

  /* Success */
  return 0;
}

/* Built in: unset environment variable */
int bi_envunset(char** args, int argc, int outfd){
  if(argc != 2){
    fprintf(stderr, "envunset error: incorrect number of arguments\n");
    return -1;
  }
  else if(unsetenv(args[1]) != 0){
    fprintf(stderr, "envset error: %s\n", strerror(errno));
    return -1;
  }

  /* Success */
  return 0;
}

/* Built in: Change directory */
int bi_cd(char** args, int argc, int outfd){
  if(argc > 2){
    fprintf(stderr, "cd: too many arguments\n");
    return -1;
  }
  else if(argc == 1){
    if(chdir(getenv("HOME")) < 0){
      fprintf(stderr, "cd: HOME not set\n");
      return -1;
    }
  }
  else if(chdir(args[1]) < 0){
    perror("cd");
    return -1;
  }

  /* Success */
  return 0;
}

/* Built in: shift */
int bi_shift(char** args, int argc, int outfd){
  int n = 0;
  if(argc == 1){
    n = 1;
  }
  else{
    n = atoi(args[1]);
  }
  
  if(n >= mainargc){
    fprintf(stderr, "shift: not enough arguments for shift of %d\n", n);
    return -1;
  }

  currindex = currindex + n;

  if((mainargc - n) <= 1){
    mainargc = 1;
    }
  else{
    mainargc = mainargc - n;
  }

  /* Success */
  return 0;
}

/* Built in: unshift */
int bi_unshift(char** args, int argc, int outfd){
  int n = 0;
  if(argc != 1){
    n = atoi(args[1]);
  }
  
  if(n >= mainargc - 2 && n != 0){
    fprintf(stderr, "unshift: not enough arguments for unshift of %d\n", n);
    return -1;
  }

  if(argc == 1){
    currindex = 2;
    mainargc = og_argc - 1;
  }
  else{
    currindex = currindex - n;
    mainargc = mainargc + n;
  }

  /* Success */
  return 0;
}

/* Built in: sstat */
int bi_sstat(char** args, int argc, int outfd){
  
  if(argc == 1){
    fprintf(stderr, "stat: no file specified\n");
    return -1;
  }
  else{
    int i = 1;
    while(args[i] != 0){

      struct stat fileinfo;

      if(stat(args[i], &fileinfo) == -1){
	perror(args[i]);
	exitVal = 1;
      }
      else{      
	dprintf(outfd, "%s ", args[i]);
      
	struct passwd *pswd;
	pswd = getpwuid(fileinfo.st_uid);
	if(pswd == NULL){
	  dprintf(outfd, "%d ", fileinfo.st_uid);
	}
	else{
	  dprintf(outfd, "%s ", pswd->pw_name);
	}
     
	struct group *grp;
	grp = getgrgid(fileinfo.st_gid);
	if(grp == NULL){
	  dprintf(outfd, "%d ", fileinfo.st_gid);
	}
	else{
	  dprintf(outfd, "%s ", grp->gr_name);
	}

	char output[13];
	strmode(fileinfo.st_mode, output);      
	dprintf(outfd, "%s", output);

	dprintf(outfd, "%ld ", fileinfo.st_nlink);

	dprintf(outfd, "%ld ", fileinfo.st_size);
      
	struct tm *info = localtime(&(fileinfo.st_mtime));
	dprintf(outfd, "%s", asctime(info));
      }
      i++;
    }
    fflush(stdout);
  }

  /* Success */
  return 0;
}
