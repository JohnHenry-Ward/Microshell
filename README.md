This is a Microshell in C. It's main use is to read commands from the command line and appropriatly execute it. It helped me understand the UNIX operating system, gain experience writing C code, and help me understand programming topics like pointers, system vs library calls, and memory. Below is a list of the main features implemented in this Microshell.


Argument Parsing
*Use system calls like: execvp, fork, waitpid, pipe
*Uses arrays and pointers to parse a command line argument
Command Line Expansion
  -Enviornment Variables: ${...}
  -Process ID: $
  -Command Line Arguments: an integer
  -Number of Arguments: #
  -Exit Value: $?
  -Command Expansion: $(...)
  -Pipe: |
  -Wildcards: *
Built In's
  -Exit
  -Set/unset environment varaibles
  -Cd
  -Shift/Unshift
  -Sstat
