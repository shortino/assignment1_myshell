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

pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);

/*syserr reporting method*/
void syserr(char *msg){
	fprintf(stderr, "%s: %s", strerror(errno), msg);
	abort();
}

/*set parent path method*/
void set_parent(void){
	char temp_path[1024];
	getcwd(temp_path, 1023);
	strcat(temp_path, "/myshell");		// concatonate with /myshell
	setenv("parent", home_path, 1);		// update the environment
}

/*main method*/
int main (int argc, char ** argv)
{
	char buf[MAX_BUFFER];                      // line buffer
	char * args[MAX_ARGS];                     // pointers to arg strings
	char ** arg;                               // working pointer thru args
	char * prompt = "==> " ;                    // shell prompt
	pid_t pid;									//process ID
	int dont_wait;								//dont_wait true/false value
	int status;									//waitpid function status
	int io = 0;									//IO redirection signal true/false value
	int append_status = 0;						//Append to end of file true/false
	int write_out = 0;							// flag for write out condition
	char inputfile[1024];						//name of inputfile
	char outputfile[1024];						//name of outputfile
	int prompt_print = 1;						//print out prompt flag (default yes)
	FILE *ofp;
	int print_ev = 0;							//print_ev command for echo and environment printing flag	
	char temp_string[1024];

	/* get the home dir path */
	getcwd(home_path, 1023);
	strcat(home_path, "/myshell");		// concatonate with /myshell
	setenv("shell", home_path, 1);		// update the environment
	getcwd(home_path, 1023);			// reset home_path


	/* check for batch command argument first */
	if(argc > 1) {
		//	printf("%s", argv[1]);
		freopen(argv[1], "r", stdin);
		prompt_print = 0;
	}

	/* keep reading input until "quit" command or eof of redirected input */
	while (!feof(stdin)) { 
		/* get command line from input */
		memset(buf, 0, MAX_BUFFER);				// set buffer to 0 to wipe any possible data
		if (prompt_print) {
			fputs (prompt, stdout);				// write prompt
		}
		if (fgets(buf, MAX_BUFFER, stdin)) {	// read a line

			/*check buffer command for &*/
			if (buf[strlen(buf)-2] == '&'){		// check buffer input for & flag 
				dont_wait = 1;					// set dont_wait flag
				buf[strlen(buf)-2] = '\0';		// has to be null terminating for execvp function to work
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
			/* Check for < or >> here */
			int j =0;
			while ((args[j] != NULL)){
				if (!strcmp(args[j], "<")){
					strcpy(args[j], "\0");				//Copy in null terminator for execution
					strcpy(inputfile, args[j+1]);		//Set input file name
					io = 1;
				}
				else if (!strcmp(args[j], ">")){
					strcpy(outputfile, args[j+1]);		//set output file name
					write_out = 1;
					args[j] = NULL;						//set it to null for exec command to stop at this args
				}
				else if (!strcmp(args[j], ">>")){
					strcpy(outputfile, args[j+1]);
					append_status = 1;
					args[j] = NULL;
					//strcpy(args[j], "\0");				//Copy in null terminator for execution
				}
				j++;

			}


			/*===start command line input matching===*/
			if (args[0]) {												// if there's anything there

				/*check for pause command*/
				if (!strcmp(args[0], "pause")) {			// "pause" command method
					getpass("Press Enter to continue...");
					continue;
				}

				/* check for IO command */
				if (io == 1){
					//access check for file existence and read permission
					if (!access(inputfile, R_OK)){
						switch (pid = fork()){
							case -1:
								syserr("fork");
							case 0:
								set_parent();
								printf("In child process");
								freopen(inputfile, "r", stdin);			//replace stdin
								if ( append_status){				//check output mode
									freopen(outputfile, "a+", stdout);
								}
								else {
									freopen(outputfile, "w", stdout);	//replace stdout stream
								}				
								execv(args[0], args);					//execute in child thread
								syserr("exec");
							default:
								if (!dont_wait){
									waitpid(pid, &status, WUNTRACED);	//wait for this command to execute
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

				/*clr command*/
				if (!strcmp(args[0],"clr")) { // "clear" command
					switch (pid = fork()) {
						case -1:
							syserr("forK");
						case 0:
							set_parent();
							if (execvp("clear",  args) < 0) {		//with no args
								perror("execl");
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


				/*echo command*/
				if (!strcmp(args[0], "echo")) {	
					if (write_out == 1){
						print_ev =1;									//set print out as false
						if ((ofp = fopen(outputfile, "w")) == NULL){		//open the file in correct mode
							syserr("fopen");
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
							fprintf(ofp, "%s ", args[i]);
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
				if (!strcmp(args[0], "dir")) { //"dir" command modified
					switch (pid = fork ()) { 
						case -1:
							syserr("fork"); 
						case 0:                 // child proccess with 
							//printf("Process ID in child after fork: %d\n", pid);
							if (write_out == 1){
								freopen(outputfile, "w", stdout);	//replace stdout stream			
							}
							else if (append_status == 1){
								freopen(outputfile, "a+", stdout); 
							}
							//execvp("ls", args);								//old exec command
							if (args[1]) {										//for dir command with args
								if(execl("/bin/ls", "ls", "-al", args[1], NULL)){
									perror("execl");								//error checks for dir command
									exit(1);
								}
							}
							if (execl("/bin/ls", "ls", "-al", NULL) < 0) {		//with no args
								perror("execl");
								exit(1);
							}
							syserr("exec");										//custom error report and abort (as execvp should NOT return!)
						default:												// parent process dont_wait conditions
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
				if (!strcmp(args[0],"environ")) {		// "ervironment command"						
					if (write_out == 1){
						print_ev =1;									//set print out as false
						if((ofp = fopen(outputfile, "w")) == NULL){		//open the file in correct mode
							syserr("fopen");
						}
					}					
					else if (append_status == 1){
						print_ev = 1;
						if((ofp = fopen(outputfile, "a")) == NULL){
							syserr("fopen");
						}
					}
					char **test = environ;
					while(*test) {
						if(!print_ev){
							printf("%s\n", *test);
						}
						else {
							fprintf(ofp, "%s\n", *test);
						}
						test++;
					}
					if (print_ev) {
						fclose(ofp);
					}
					write_out = 0;										//set flags to false
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
							if (write_out == 1){
								freopen(outputfile, "w", stdout);	//replace stdout stream			
							}
							else if (append_status == 1){
								freopen(outputfile, "a+", stdout); 
							}
							strcpy(temp_string, home_path);
							strcat(temp_string, "/readme");
							if (execlp("more", "more", temp_string, NULL) < 0) {		//with no args
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

				/* else pass command onto OS (or in this instance, print them out) */

				/*char *cmd = (char *)malloc(MAX_BUFFER);
				  arg = args;
				  while (*arg){
				  strcat(cmd, *arg++);
				  strcat(cmd, " ");
				  }*/
				switch(pid = fork()) {
					case -1:
						syserr("fork");
					case 0:

						execvp(args[0] , args);
						syserr("execvp");	
						//free(cmd);
					default:
						if (!dont_wait){
							waitpid(pid, &status, WUNTRACED);
						}
				}

				/* reset all flag values to false */ 
				//free(cmd);
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




/*
   char *val = getenv("PWD");
   PWD = "/home"
   getcwd(buf, 1024-1);
   putenv();
   setenv("PWD", "/home", 0); //0 doest overide the existing environment e.g pwd, 1 doest overide


//change dir
if (dir_name == NULL) {
dir = getcwd(...);
print dir;
}
else {
chdir(dir_name);
setenv("PWD", dir_name, 1?);
}

*/
