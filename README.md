This is a Microshell in C.

It was built to demonstrate the ability of the C language in taking input from the user, parsing it and doing necessary command expansion, and then executing the command, whether that be through an exece system call, or a built in function call.
The Microshell uses arrays and pointers to process the command line input. It also can process double quotes, and understands special characters like #, ?, and \ for command expansion.
Once a line has been processed, the line goes through command expansion, replace certain text with the proper environment variables, and can read from a file and execute commands from that file.
The Microshell can also understand built in functions, such as exit, set and unset environment variables, sstat, and others. More can be easily added in the future.
The Microshell can also understand pipelines, so that an output of one command can be piped into the input of another command.
