/* Prototypes
   Created 4/15/20
 */

int expand (char *orgi, char *new, int newsize);
int isBuiltIn (char *line);
int runBuiltIn (int index, char** args, int argc, int outfd);
void strmode (mode_t mode, char *p);
int processline (char *line, int infd, int outfd, int flags);
int waitForChild (int cpid);
void catchSigInt (int sigInt);
#define WAIT 1
#define NOWAIT 0
#define EXPAND 2
#define NOEXPAND 0
