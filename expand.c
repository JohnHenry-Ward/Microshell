/*   Expand.c
 *
 *   4/15/20, JohnHenry Ward
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include "defn.h"
#include "globals.h"

int expand (char *orig, char *new, int newsize){
  
  int origIndex = 0;
  int newIndex = 0;
  int i = 0;
  while(orig[origIndex] != 0){
    if(newIndex == newsize){
      printf("Hit end of buffer in new string\n");
      return -1;
    }    
    
    else if(orig[origIndex] == '$'){
      /* Environment Variable */
      if(orig[origIndex+1] == '{'){
	origIndex+=2;
	i = 0;
	char envVar[newsize - newIndex];
	while(orig[origIndex] != '}'){
	  if(orig[origIndex] == 0){
	    printf("Mismatch { }\n");
	    return -1;
	  }
	  envVar[i] = orig[origIndex];
	  i++;
	  origIndex++;
	}

	envVar[i] = 0;
	char* value = getenv(envVar);
	
	if(value != NULL){
	  i = 0;
	  while(value[i] != 0){
	    if(newIndex == newsize){
	      printf("Hit end of buffer in new string\n");
	      return -1;
	    }
	    new[newIndex] = value[i];
	    newIndex++;
	    i++;
	  }
	}
	origIndex++;
      }
      /* Process ID */
      else if(orig[origIndex+1] == '$'){
	char pid[newsize-newIndex];
	snprintf(pid, 50, "%d", getpid());
	i = 0;
	while(pid[i] != 0){
	  new[newIndex] = pid[i];
	  newIndex++;
	  i++;
	}
	origIndex+=2;
      }
      /* Command line arguments */
      else if(isdigit(orig[origIndex+1])){
	/* Shell/Script name */
	if(orig[origIndex+1] == '0'){
	  i = 0;
	  if(og_argc == 1){
	    while(mainargv[0][i] != 0){
	      new[newIndex] = mainargv[0][i];
	      newIndex++;
	      i++;
	    }
	  }
	  else{
	    while(mainargv[1][i] != 0){
	      new[newIndex] = mainargv[1][i];
	      newIndex++;
	      i++;
	    }
	  }
	  origIndex+=2;
	}
	/* Argument list */
	else{
	  i = 0;
	  /* Get the argument number */
	  char argnum[2];
	  int savedindex = origIndex;
	  while(isdigit(orig[origIndex+1])){
	    argnum[i] = orig[origIndex+1];
	    origIndex++;
	    i++;
	  }
	  origIndex = savedindex;
	  
	  /* Check if there is an argument there */
	  if(og_argc - 1 < (atoi(argnum) + currindex - 1)){
	    i = 0;
	    while(isdigit(argnum[i])){
	      new[newIndex] = ' ';
	      origIndex++;
	      i++;
	    }
	    origIndex++;
	  }
	  else{
	    i = 0;
	    while(mainargv[atoi(argnum) + currindex - 1][i] != 0){
	      new[newIndex] = mainargv[atoi(argnum) + currindex - 1][i];
	      newIndex++;
	      i++;
	    }
	    origIndex++;
	    i = 0;
	    while(isdigit(argnum[i])){
	      origIndex++;
	      i++;
	    }
	  }
	}
      }
      /* Number of arguments */
      else if(orig[origIndex+1] == '#'){
	char str[newsize-newIndex];
	snprintf(str, 50, "%d", mainargc);
	i = 0;
	while(str[i] != 0){
	  new[newIndex] = str[i];
	  newIndex++;
	  i++;
	}
	origIndex+=2;
      }
      /* Exit value */
      else if(orig[origIndex+1] == '?'){
        char valAsStr[4];
        snprintf(valAsStr, 4, "%d", exitVal);
        i = 0;
        while(valAsStr[i] != 0){
          new[newIndex] = valAsStr[i];
          newIndex++;
          i++;
        }
        origIndex+=2;
      }
      /* Command Expansion */
      else if(orig[origIndex+1] == '('){
	origIndex+=2;
	int multiParen = 0;
	char command[newsize - newIndex];
	i = 0;
	/* Get command encapsulated by $( ... ) */
	while(orig[origIndex] != ')' || multiParen != 0){
	  if(orig[origIndex] == 0){
	    printf("command expansion error: missing )\n");
	    return -1;
	  }
	  if(orig[origIndex] == '('){
	    multiParen++;
	  }
	  else if(orig[origIndex] == ')' && multiParen != 0){
	    multiParen--;
	  }
	  command[i] = orig[origIndex];
	  i++;
	  origIndex++;
	}
	command[i] = 0;

	/* Create a pipe */
	int fd[2];
	if(pipe(fd) < 0){
	  fprintf(stderr, "pipe error: %s\n", strerror(errno));
	  return -1;
	}

	/* Recursive call to processline */
	int cpid = processline(command, fd[0], fd[1], NOWAIT | EXPAND);
	close(fd[1]);

	/* Read loop to read into the new string, 8 bytes at a time */
	int rv;
	int savedIndex = newIndex;
	while((rv = read(fd[0], &new[newIndex], 8)) > 0){
	  if(newIndex >= newsize){
	    printf("Hit end of buffer in new string\n");
	    return -1;
	  }
	  if(rv == -1){
	    fprintf(stderr, "read error: %s\n", strerror(errno));
	    return -1;
	  }
	  newIndex+=rv;
	}

	/* Remove newlines */
	if(new[newIndex - 1] == '\n'){
	  newIndex = newIndex-1;
	}
	
	i = savedIndex;
	while(new[i] != 0){
	  if(new[i] == '\n'){
	    new[i] = ' ';
	  }
	  i++;
	}
       
	close(fd[0]);
	
	/* Have the parent wait for child to complete */
	if(cpid > 0){
	  waitForChild(cpid);
        }
	
	origIndex++;
      }
      
      else{
	new[newIndex] = orig[origIndex];
	origIndex++;
	newIndex++;
      }
    }
    /* End of $ */

    
    /* Wildcard */
    else if(orig[origIndex] == '*' && (orig[origIndex-1] == '\\' || orig[origIndex-1] == ' ' || orig[origIndex-1] == '"')){
      /* Print all files in current directory */
      if(orig[origIndex-1] == ' ' && (orig[origIndex+1] == ' ' || orig[origIndex+1] == 0)){
	DIR *pdir = opendir(".");
	struct dirent *dirinfo;
	int i = 0;
	if(pdir == NULL){
	  fprintf(stderr, "Error opening directory: %s\n", strerror(errno));
	  return -1;
	}

	while((dirinfo = readdir(pdir))){
	  if(newIndex == newsize){
	    printf("Hit end of buffer in new string\n");
	    return -1;
	  }
	  
	  if(strncmp(dirinfo->d_name, ".", 1) != 0){
	    i = 0;
	    while(dirinfo->d_name[i] != 0){
	      new[newIndex] = dirinfo->d_name[i];
	      newIndex++;
	      i++;
	    }
	    new[newIndex] = ' ';
	    newIndex++;
	  }
	}
	closedir(pdir);
	origIndex++;
      }

      /* Print * (done below, outside of wildcard if block */
      else if(orig[origIndex-1] == '\\'){
	newIndex++;
	origIndex++;
      }

      /* Print specified files */
      else{
	DIR *pdir = opendir(".");
        struct dirent *dirinfo;
        
        if(pdir == NULL){
          fprintf(stderr, "Error opening directory: %s\n", strerror(errno));
          return -1;
        }

	/* Get what the specified file ending is */
	char file_end[25];
	int i = 0;
	origIndex++;
	while(orig[origIndex] != ' ' && orig[origIndex] != 0 && orig[origIndex] != '"'){
	  if(orig[origIndex] == '/'){
	    printf("* error: / found\n");
	    return -1;
	  }
	  file_end[i] = orig[origIndex];
	  origIndex++;
	  i++;
	}
	
	file_end[i] = 0;
	
	/* Check if file name should be added */
	int file_found = -1;
        while((dirinfo = readdir(pdir))){
	  if(newIndex == newsize){
	    printf("Hit end of buffer in new string\n");
	    return -1;
	  }

	  i = 0;
	  int should_add = 0;
	  int name_length = strlen(dirinfo->d_name);
	  int index = name_length - strlen(file_end); 
	  if(strncmp(dirinfo->d_name, ".", 1) != 0 && name_length >= index){
	    while(file_end[i] != 0){
	      if(dirinfo->d_name[index] == file_end[i]){
		i++;
		index++;
	      }
	      else{
		should_add = -1;
		break;
	      }
	    }

	    i = 0;
	    if(should_add == 0){
	      file_found = 0;
	      /* Add file name */
	      while(dirinfo->d_name[i] != 0){
		new[newIndex] = dirinfo->d_name[i];
		newIndex++;
		i++;
	      }
	      new[newIndex] = ' ';
	      newIndex++;
	    }
	    
	  }
	}

	/* No files were found */
	if(file_found == -1){
	  i = 0;
	  new[newIndex] = '*';
	  newIndex++;
	  while(file_end[i] != 0){
	    new[newIndex] = file_end[i];
	    newIndex++;
	    i++;
	  }
	}
	closedir(pdir);
      }      
    }
    /* End of Wildcard */

    
    else if(orig[origIndex] == '\\'){
        new[newIndex] = '*';
        newIndex++;
        origIndex+=2;
      }
    
    else{
      new[newIndex] = orig[origIndex];
      origIndex++;
      newIndex++;
    }
  }

  new[newIndex] = 0;
  
  return 0;
}
