/*
 * Comp3520 Assignment 1 myshell
 * James Alexander 30792962
 * jale4381
 *
 * To keep a little consistency with the layout of the tutorials, 
 * in which most of this exercises were devleoped, 
 * I have decided to keep everything in one large main method
 * and in one file, instead of seperating the components. 
 * 
 * */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h>



#define MAX_BUFFER 1024                        // max line buffer
#define MAX_ARGS 64                            // max # args
#define SEPARATORS " \t\n"                     // token sparators
extern char** environ;
extern int errno;							   // system error number
void syserr(char*);								// erro report and abort routine
char path[1024];								// tempoary path used in cd
char home_path[1024];							// path to home directory for readme file
char * prompt = "==> " ;						// shell prompt

void (*signal(int sig, void (*func)(int)))(int);

pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);

/*syserr reporting method*/
void syserr(char *msg){
	fprintf(stderr, "%s: %s", strerror(errno), msg);
	abort();
}

/*sig_process for processing caught signals*/
void sig_process(int sig_number) {
	printf("caught signal, press Enter to continue\n");
	return;
}

/*set parent path method in child processes*/
void set_parent(void){
	setenv("parent", getenv("shell"), 1);		// update the environment with the shell path

}

/* beign main method */
int main (int argc, char ** argv)
{
	char buf[MAX_BUFFER];						// line buffer
	char * args[MAX_ARGS];						// pointers to arg strings
	char ** arg;								// working pointer thru args
	pid_t pid;									// process ID
	int dont_wait;								// dont_wait true/false value
	int status;									// waitpid function status
	int io = 0;									// IO redirection signal true/false value
	int append_status = 0;						// Append to end of file true/false
	int write_out = 0;							// flag for write out condition
	char inputfile[1024];						// name of inputfile
	char outputfile[1024];						// name of outputfile
	int prompt_print = 1;						// print out prompt flag (default yes)
	FILE *ofp;									// file pointer for output redirection
	int print_ev = 0;							// print_ev command for echo and environment printing flag	
	char temp_string[1024];

	/* get the home dir path */
	getcwd(home_path, 1023);
	strcat(home_path, "/myshell");		// concatonate with /myshell
	setenv("shell", home_path, 1);		// update the environment
	getcwd(home_path, 1023);			// reset home_path


	/* check for batch command argument first */
	if(argc > 1) {
		freopen(argv[1], "r", stdin);
		prompt_print = 0;
	}

	/* keep reading input until "quit" command or eof of redirected input */
	while (!feof(stdin)) { 
		/* catch the kill signal in parent only */
		if (signal(SIGINT, sig_process));
	
		/* get command line from input */
		memset(buf, 0, MAX_BUFFER);						// set buffer to 0 to wipe any possible data
		if (prompt_print) {
			fputs (getcwd(temp_string, 1023), stdout);	// print out current path
			fputs (prompt, stdout);						// write prompt
		}
		if (fgets(buf, MAX_BUFFER, stdin)) {			// read a line of input

			/*check buffer command for &*/
			if (buf[strlen(buf)-2] == '&'){				// check buffer input for & flag 
				dont_wait = 1;							// set dont_wait flag
				buf[strlen(buf)-2] = '\0';				// has to be null terminating for execvp function to work
			}
			else {
				dont_wait = 0;
			}

			/* tokenize the input into args array */
			arg = args;
			*arg++ = strtok(buf,SEPARATORS);				// tokenize input
			while ((*arg++ = strtok(NULL,SEPARATORS)));		// last entry will be NULL 

			if (args[0] == NULL){
				continue;
			}
			
			/* Check for <, > or >> here */
			int j =0;
			while ((args[j] != NULL)){
				if (!strcmp(args[j], "<")){
					strcpy(args[j], "\0");					//Copy in null terminator for execution of input file to stop here
					strcpy(inputfile, args[j+1]);			//Set input file name
					io = 1;
				}
				else if (!strcmp(args[j], ">")){
					strcpy(outputfile, args[j+1]);			//set output file name
					write_out = 1;
					args[j] = NULL;							//set it to null for exec command to stop at this args
				}
				else if (!strcmp(args[j], ">>")){
					strcpy(outputfile, args[j+1]);
					append_status = 1;
					args[j] = NULL;
				}
				j++;

			}

			/* === start command line input matching === */
			if (args[0]) {											// if there's anything there

				/*check for pause command*/
				if (!strcmp(args[0], "pause")) {					// "pause" command method
					getpass("Press Enter to continue...");
					continue;
				}

				/* check for Input Output command */
				if (io == 1){
					/*access check for file existence and read permission*/
					if (!access(inputfile, R_OK)){
						switch (pid = fork()){
							case -1:
								syserr("fork");
							case 0:
								set_parent();							// set_parent path
								printf("In child process");
								freopen(inputfile, "r", stdin);			// replace stdin in read mode
								if ( append_status){					// check output mode
									freopen(outputfile, "a+", stdout);	// replace stdout in append mode
								}
								else {
									freopen(outputfile, "w", stdout);	// replace stdout stream in write mode
								}				
								execv(args[0], args);					// execute in child thread
								syserr("exec");
							default:
								if (!dont_wait){
									waitpid(pid, &status, WUNTRACED);	// wait for this command to execute
									printf("\nDone outputing\n");
								}
						}
					}
					else {
						syserr("inputfile");
					}
					/*Set status values to false again*/
					io = 0;
					append_status = 0;
					continue;
				}

				/* clr command */
				if (!strcmp(args[0],"clr")) {					// "clear" command
					switch (pid = fork()) {
						case -1:
							syserr("forK");
						case 0:
							set_parent();
							if (execvp("clear",  args) < 0) {	//clear with no args
								perror("execl");
								exit(1);
							}
						default:								// parent process dont_wait conditions
							if (!dont_wait){
								waitpid(pid, &status, WUNTRACED);
							}
					}
					/* set values to false */
					dont_wait = 0;
					continue;
				}


				/* echo command */
				if (!strcmp(args[0], "echo")) {	
					if (write_out == 1){
						print_ev =1;									// set print out as false
						if ((ofp = fopen(outputfile, "w")) == NULL){	// open the file in correct mode
							syserr("fopen");							// through syserr message if unsucessfull
						}
					}
					else if (append_status == 1){
						print_ev = 1;
						if ((ofp = fopen(outputfile, "a")) == NULL){
							syserr("fopen");
						}
					}
					int i = 1;
					while (args[i] != NULL){
						if (!print_ev){							// print out to stdout or file depending on flag
							printf("%s ", args[i]);
						}
						else {
							fprintf(ofp, "%s ", args[i]);		// else print out to a file
						}
						++i;
					}
					if (print_ev) {
						fclose(ofp);
					}
					print_ev = 0;								//set flags back to false
					write_out = 0;
					append_status = 0;
					printf("\n");								//add new line for consistency in prompt
					continue;
				}

				/*dir command*/
				if (!strcmp(args[0], "dir")) {	
					switch (pid = fork ()) { 
						case -1:
							syserr("fork"); 
						case 0:														// in the child proccess
							//printf("Process ID in child after fork: %d\n", pid);
							set_parent();											// set the parent path
							if (write_out == 1){
								freopen(outputfile, "w", stdout);					// replace stdout stream			
							}
							else if (append_status == 1){
								freopen(outputfile, "a+", stdout); 
							}
							//execvp("ls", args);									// old exec command
							if (args[1]) {											// for dir command with args
								if(execl("/bin/ls", "ls", "-al", args[1], NULL)){
									perror("execl");								//error checks for dir command
									exit(1);
								}
							}
							if (execl("/bin/ls", "ls", "-al", NULL) < 0) {			//dir command with no args
								perror("execl");
								exit(1);
							}
							syserr("execl");										//custom error report and abort (as execvp should NOT return!)
						default:													// parent process dont_wait conditions
							if (!dont_wait){
								waitpid(pid, &status, WUNTRACED);
							}
					}
					/* set values to false */
					dont_wait = 0;
					append_status = 0;
					write_out = 0;
					continue;
				}

				/*environment command*/
				if (!strcmp(args[0],"environ")) {						// "ervironment command"						
					if (write_out == 1){								// choose correct output mode
						print_ev =1;									// set print out as false
						if((ofp = fopen(outputfile, "w")) == NULL){		// open the file in correct mode
							syserr("fopen");
						}
					}					
					else if (append_status == 1){						// append output mode
						print_ev = 1;
						if((ofp = fopen(outputfile, "a")) == NULL){
							syserr("fopen");
						}
					}
					char **test = environ;
					while(*test) {
						if(!print_ev){
							printf("%s\n", *test);				// print out the envrionment to stdout or file
						}
						else {
							fprintf(ofp, "%s\n", *test);
						}
						test++;
					}
					if (print_ev) {
						fclose(ofp);
					}
					write_out = 0;								//set flags to false
					append_status = 0;
					print_ev = 0;
					continue;
				}

				/*cd command*/
				if (!strcmp(args[0], "cd")){			// 'cd' command 
					if (!args[1]){
						getcwd(path, 1023);
						printf("%s\n", path);

					}
					else {
						if (!chdir(args[1])){			// check for valid directory and change to it
							getcwd(path, 1023);			// get the new environment
							setenv("PWD", path, 1);		// update the environment
						}
						else {
							printf("Directory is not valid\n"); // appropriate error message for non-existent directories
						}
					}
					continue;
				}

				/*quit comment*/
				if (!strcmp(args[0],"quit")){   // "quit" command
					break;						// break out of 'while' loop
				}

				/*help command*/
				if (!strcmp(args[0], "help")) {
					//system("more readme");
					switch (pid = fork()) {
						case -1:
							syserr("fork");
						case 0:
							set_parent();
							if (write_out == 1){
								freopen(outputfile, "w", stdout);	// replace stdout stream to file			
							}
							else if (append_status == 1){
								freopen(outputfile, "a+", stdout); // append stdout stream to file
							}
							strcpy(temp_string, home_path);
							strcat(temp_string, "/readme");
							if (execlp("more", "more", temp_string, NULL) < 0) {
								perror("execlp");
								exit(1);
							}
						default:												// parent process dont_wait conditions
							if (!dont_wait){
								waitpid(pid, &status, WUNTRACED);
							}
					}
					/* set values to false */
					dont_wait = 0;
					continue;
				}

				/* else pass command onto OS with appropriate checks for & */
				switch(pid = fork()) {
					case -1:
						syserr("fork");
					case 0:
						set_parent();
						execvp(args[0] , args);
						syserr("execvp");	
					default:
						if (!dont_wait){
							waitpid(pid, &status, WUNTRACED);
						}
				}

				/* reset all flag values to false */ 
				io = 0;
				write_out = 0;
				append_status = 0;
				print_ev = 0;
				dont_wait = 0;
			}
		}
	}
	return 0; 
}
