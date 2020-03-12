#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include<linux/kernel.h>
#include <sys/types.h>
#include <errno.h>
#define STDIN 0
#define STDOUT 1
#define READ_END 0
#define WRITE_END 1
#define SIZE 1024
const char * sysname = "shellgibi";
//MYBG dont wait for it to be terminated just wait for changing state.
enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};
struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
};
/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t * command)
{
	int i=0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background?"yes":"no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete?"yes":"no");
	printf("\tRedirects:\n");
	for (i=0;i<3;i++)
		printf("\t\t%d: %s\n", i, command->redirects[i]?command->redirects[i]:"N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i=0;i<command->arg_count;++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}


}
/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i=0; i<command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i=0;i<3;++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next=NULL;
	}
	free(command->name);
	free(command);
	return 0;
}
/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[SIZE], hostname[SIZE];
    gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}
/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters=" \t"; // split at whitespace
	int index, len;
	len=strlen(buf);
	while (len>0 && strchr(splitters, buf[0])!=NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len>0 && strchr(splitters, buf[len-1])!=NULL)
		buf[--len]=0; // trim right whitespace

	if (len>0 && buf[len-1]=='?') // auto-complete
		command->auto_complete=true;
	if (len>0 && buf[len-1]=='&') // background
		command->background=true;

	char *pch = strtok(buf, splitters);
	command->name=(char *)malloc(strlen(pch)+1);
	if (pch==NULL)
		command->name[0]=0;
	else
		strcpy(command->name, pch);

	command->args=(char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index=0;
	char temp_buf[SIZE], *arg;
	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch) break;
		arg=temp_buf;
		strcpy(arg, pch);
		len=strlen(arg);

		if (len==0) continue; // empty arg, go for next
		while (len>0 && strchr(splitters, arg[0])!=NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len>0 && strchr(splitters, arg[len-1])!=NULL) arg[--len]=0; // trim right whitespace
		if (len==0) continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|")==0)
		{
			struct command_t *c=malloc(sizeof(struct command_t));
			int l=strlen(pch);
			pch[l]=splitters[0]; // restore strtok termination
			index=1;
			while (pch[index]==' ' || pch[index]=='\t') index++; // skip whitespaces

			parse_command(pch+index, c);
			pch[l]=0; // put back strtok termination
			command->next=c;
			continue;
		}

		// background process
		if (strcmp(arg, "&")==0)
			continue; // handled before

		// handle input redirection
		redirect_index=-1;
		if (arg[0]=='<')
			redirect_index=0;
		if (arg[0]=='>')
		{
			if (len>1 && arg[1]=='>')
			{
				redirect_index=2;
				arg++;
				len--;
			}
			else redirect_index=1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index]=malloc(len);
			strcpy(command->redirects[redirect_index], arg+1);
			continue;
		}

		// normal arguments
		if (len>2 && ((arg[0]=='"' && arg[len-1]=='"')
			|| (arg[0]=='\'' && arg[len-1]=='\''))) // quote wrapped arg
		{
			arg[--len]=0;
			arg++;
		}
		command->args=(char **)realloc(command->args, sizeof(char *)*(arg_index+1));
		command->args[arg_index]=(char *)malloc(len+1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count=arg_index;
	return 0;
}
void prompt_backspace()
{
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}
/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index=0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

    // tcgetattr gets the parameters of the current terminal
    // STDIN_FILENO will tell tcgetattr that it should write the settings
    // of stdin to oldt
    static struct termios backup_termios, new_termios;
    tcgetattr(STDIN_FILENO, &backup_termios);
    new_termios = backup_termios;
    // ICANON normally takes care that one line at a time will be processed
    // that means it will return if it sees a "\n" or an EOF or an EOL
    new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
    // Those new settings will be set to STDIN
    // TCSANOW tells tcsetattr to change attributes immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


    //FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state=0;
	buf[0]=0;
  	while (1)
  	{
		c=getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c==9) // handle tab
		{
			buf[index++]='?'; // autocomplete
			break;
		}

		if (c==127) // handle backspace
		{
			if (index>0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c==27 && multicode_state==0) // handle multi-code keys
		{
			multicode_state=1;
			continue;
		}
		if (c==91 && multicode_state==1)
		{
			multicode_state=2;
			continue;
		}
		if (c==65 && multicode_state==2) // up arrow
		{
			int i;
			while (index>0)
			{
				prompt_backspace();
				index--;
			}
			for (i=0;oldbuf[i];++i)
			{
				putchar(oldbuf[i]);
				buf[i]=oldbuf[i];
			}
			index=i;
			continue;
		}
		else
			multicode_state=0;

		putchar(c); // echo the character
		buf[index++]=c;
		if (index>=sizeof(buf)-1) break;
		if (c=='\n') // enter key
			break;
		if (c==4) // Ctrl+D
			return EXIT;
  	}
  	if (index>0 && buf[index-1]=='\n') // trim newline from the end
  		index--;
  	buf[index++]=0; // null terminate string

  	strcpy(oldbuf, buf);

  	parse_command(buf, command);

  	// print_command(command); // DEBUG: uncomment for debugging

    // restore the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
  	return SUCCESS;
}
int process_command(struct command_t *command);
int main()
{
	while (1)
	{
		struct command_t *command=malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code==EXIT) break;

		code = process_command(command);
		if (code==EXIT) break;

		free_command(command);
	}

	printf("\n");
	return 0;
}


void auto_complete(struct command_t *command){
	char buf[SIZE];
	FILE *file; 
	FILE *for_my_commands;
	char *ptr = strchr(command->name,'/');
		if(ptr != NULL){
			char cmd[SIZE];
			//If command name includes / and . it is a relative path to an executable in the current directory
			if(command->name[0] == '.' && command->name[1] == '/'){
				strcpy(cmd,"/bin/ls | /bin/grep ");
				char buffer[SIZE];
				strncpy(buffer, command->name+2,SIZE);
				buffer[(int)(strchr(buffer,'?')-buffer)] = '\0';
				strcat(cmd,buffer);
				//get the results of the ls | grep command->name into the file
				if((file = popen(cmd,"r"))== NULL){
				printf("%s\n","Error openning pipe" );
				exit(-1);
				}
				 while (fgets(buf, SIZE, file) != NULL) {
	       		 	printf("\n %s",buf);
	   			 }	
			}

		} else{ 
			//take the path variable for searching executables 
			char *path = getenv("PATH");  
			char modified[SIZE];
		    strcpy(modified,path);
			char *token = strtok(modified,":");
			char name[SIZE];
			strcpy(name,command->name);
			name[(int)(strchr(name,'?')-name)] = '\n';
			int i = 0; //counter for matching executables 
			int y = 0; //counter for matching commands from newly created ones
			bool flag = false; //whether there is a command exactly matches with the given name
			char only_possible[SIZE];
			char exec_my_files[SIZE];
			//for the newly created commands I give a string composed of them to the grep and take the matching results to 
			//for_my_commands file
			strcpy(exec_my_files,"echo \"myjobs\nmybg\nmyfg\npause\npsvis\n\" | grep ");
			strcat(exec_my_files,command->name);
			exec_my_files[(int)(strchr(exec_my_files,'?')-exec_my_files)] = '\0';
			if((for_my_commands = popen(exec_my_files,"r"))== NULL){
				printf("%s\n","Error openning pipe" );
				exit(-1);
			}
	   		while (fgets(buf, SIZE, for_my_commands) != NULL) {
	       		if(y>1) // I have a counter to check how many matching commands found
	       		    printf("%s\n",only_possible); // only_possible stores the previous matching command
	       		    strcpy(only_possible,buf);
	       		    y+=1; 
	       		    if(strcmp(buf,name) == 0) 
	       		    	flag = true;

	   			 }
			while(token != NULL){
				char exec_path[SIZE]; 
				strcpy(exec_path,"/bin/ls "); //ls every element of the path and direct the results to grep
				strcat(exec_path,token); 
				strcat(exec_path," | ");
				strcat(exec_path,"/bin/grep ");
				strcat(exec_path,command->name);
				
				exec_path[(int)(strchr(exec_path,'?')-exec_path)] = '\0';
				if((file = popen(exec_path,"r"))== NULL){ // store the matching results in the file
					printf("%s\n","Error openning pipe" );
					exit(-1);
				}
				 while (fgets(buf, SIZE, file) != NULL) {
	       		    if(i>1)
	       		    	printf("%s\n",only_possible);
	       		    strcpy(only_possible,buf);
	       		    i+=1;
	       		    if(strcmp(buf,name) == 0)
	       		    	flag = true;

	   			 }

				token = strtok(NULL,":"); 
				
			}
			if(i+y==1 && flag){ 
			 //at the end if we have a fully typed command we should have 
			 //found only one command from new ones and executables
	   			struct command_t *command=malloc(sizeof(struct command_t));
				memset(command, 0, sizeof(struct command_t)); 
				command->name = "ls";
				process_command(command);
	   			} else if(i>1){ 
	   			 	//only_possible always holds the previous matching command. 
	   			 	//In the case that we have more than 1 command, in order to print last matching one we should do this
	   			 	printf("%s\n", only_possible); 
	   			 } else{
	   			 	printf("%s\n", only_possible); 
	   			 }
		}
			return SUCCESS;
}
int execute_command(struct command_t *command){
	char *path = getenv("PATH"); 
		char modified[SIZE];
		strcpy(modified,path);
		char *token = strtok(modified,":");
		char *ptr = strchr(command->name,'/');
		if(ptr != NULL){
			execv(command->name,command->args);//when we find the executable we execute it, it should be in the current directory 
			exit(0);//#

		} else{
			while(token != NULL){
				char *exec_path[500] = {'\0'}; //for each possible directory equating exec path to empty string which will eventually store the possible executable path
				strcat(exec_path,token); //get one path from the given paths and copy it to exec_path
				strcat(exec_path,"/");
				strcat(exec_path,command->args[0]);//concatenate exec path with the executable name
				command->args[0]=exec_path; //for execv to work the first element of the args should be the path to executable
				if(access(exec_path,X_OK)==0){//access checks whether there is such file in the given path X_OK means executable files
					
					execv(exec_path,command->args);//when we find the executable we execute it 
					exit(0);//#
				} 
				token = strtok(NULL,":"); //execv only returns if there is an error. So after execution this lines wont be executed
				command->args[0] = command->name; //we assing command->args the filename as a value in order to correctly check following possible paths   
		}
	}
	return SUCCESS;
}
//takes a number as an argument and plays this many random songs from your current working directory
void mixPlay(struct command_t *command){
		if(fork() == 0){
		   char cwd[SIZE];
		   getcwd(cwd,SIZE);
		   strcat(cwd,"/mixPlay.py");
		   const char* play[] = { "python", cwd, command->args[0],"&", 0};
            execvp(play[0], play);
            perror("execvp play");
            exit(1);
		}
		else
			wait(NULL);

	
	}
//no arguments. It parses the data in the main page of bbc and 
//prints you the daily titles
void bbc(struct command_t *command){
	 char cwd[SIZE];
	 getcwd(cwd,SIZE);
	 strcat(cwd,"/parser.py");
	 command->args[0] = (char *)malloc(SIZE);
	 strcpy(command->args[0],cwd);
	 strcpy(command->name,"python");
	 command->arg_count = 1;
	 process_command(command);
	}

void play_alarm(struct command_t *command_given){
	int pid = fork();
	if(pid < 0){
		fprintf(stderr, "%s\n","Fork failed" );
		exit(1);
	}
	if(pid == 0){
		int cron_pipe[2];
		if(pipe(cron_pipe) < 0){
			fprintf(stderr, "%s\n","Cron pipe failed" );
			exit(1);
		}
		if(fork()==0){
			close(cron_pipe[READ_END]);
			char cwd[SIZE];
			getcwd(cwd,SIZE);
			strcat(cwd,"/");
			dup2(cron_pipe[WRITE_END],STDOUT);
			char line_to_crontab[SIZE];
			char *time = command_given->args[0];
			char *music = command_given->args[1];
			char *hour = strtok(time,".");
			char *minute = strtok(NULL,".");
			strcpy(line_to_crontab,minute);
			strcat(line_to_crontab," ");
			strcat(line_to_crontab,hour);
			strcat(line_to_crontab," * * * ");
			strcat(line_to_crontab,"XDG_RUNTIME_DIR=\"/run/user/1000\" "); //added before the command to make crontab play a music file
			strcat(line_to_crontab,"aplay ");
			strcat(line_to_crontab,cwd);
			strcat(line_to_crontab, music);
			const char* echo_command[] = { "echo", line_to_crontab, 0}; //writes the cron job to the write end of the pipe
			execvp(echo_command[0],echo_command);
			perror("echo execv");
			exit(1);
		} else{
			wait(NULL);
			close(cron_pipe[WRITE_END]);
			char buf[SIZE];
			dup2(cron_pipe[READ_END],STDIN);//we replace stdin with the read end of the pipe
			const  char* prog2[] = { "crontab", "-", 0};//takes file as an input and replaces it with the crontab file
	        execvp(prog2[0], prog2);
	        perror("execvp of alarm failed");
        }
	} else{
		wait(NULL);
	}
}
int process_command(struct command_t *command)
{
	int r;
	if(command->auto_complete){
		
		auto_complete(command);
		return SUCCESS;
	}
	if (strcmp(command->name, "")==0) return SUCCESS;

	if (strcmp(command->name, "exit")==0)
		return EXIT;

	if (strcmp(command->name, "cd")==0)
	{
		if (command->arg_count > 0)
		{
			r=chdir(command->args[0]);
			if (r==-1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}
	if(strcmp(command->name, "myjobs") == 0){
		char *user=getenv("USER");
    	if(user==NULL){
    		fprintf(stderr, "%s\n","Couldnt get the user" );
    		exit(1);
    	}
    	//replaced the name and the arguments of the command with ps command and it continued its execution in the 
    	//process command function like other commands read from shellgibi
    	char *cmd = "ps";
    	strcpy(command->name,cmd);
		command->args = (char **)malloc(sizeof(char *)*4) ;
		command->args[0] = (char *)malloc(SIZE);
		strcpy(command->args[0], "-u");
		command->args[1] = (char *)malloc(SIZE);
		strcpy(command->args[1], user);
		command->args[2] = (char *)malloc(SIZE);
		strcpy(command->args[2], "--format");
		command->args[3] = (char *)malloc(SIZE);
		strcpy(command->args[3], "pid,stat,ucmd");
		command->arg_count = 4; 
	}
	//sends sigstop signal to process with the given pid
	if(strcmp(command->name,"pause") == 0){ 
		int res = kill(atoi(command->args[0]),SIGSTOP);
		if(res == -1)
			printf("%s\n","Kill stop failed" );
		return SUCCESS;
		
	}
	//sends sigcont signall to a stopped process and makes it continue run in the background
	if(strcmp(command->name,"mybg") == 0){
		int res = kill(atoi(command->args[0]),SIGCONT);
		if(res == -1)
			printf("%s\n","Kill cont bg failed" );
		return SUCCESS;
	}
	//takes a process to foreground
	if(strcmp(command->name,"myfg") == 0){
		pid_t pid_fg = fork();
		if(pid_fg < 0){
			fprintf(stderr,"%s\n","Fg fork failed" );
			exit(1);
		}
		if(pid_fg == 0){
			//child process sends sigcont signal to the process with given pid 
			//and takes it to foreground
			int res = kill(atoi(command->args[0]),SIGCONT);
		if(res == -1)
			printf("%s\n","Kill cont fg failed" );
		exit(0);
		} else{
			char status[SIZE];
			//parent process waits until the process with given id is stopped by a signal
			int return_pid = waitid(P_PID, atoi(command->args[0]), &status, WSTOPPED); 
			
		}
			return SUCCESS;
	}
	if(strcmp(command->name,"alarm") == 0){
			play_alarm(command);
			return SUCCESS;
	}
	if(strcmp(command->name,"mixPlay") == 0){
		mixPlay(command);
		return SUCCESS;
	}
	if(strcmp(command->name,"bbc") == 0){
		bbc(command);
	}
	if(strcmp(command->name,"psvis") == 0){
		//it prepares a new line which should be parsed and execute the following
		//./psvis given_pid | dot -Tx11
		char *prog_name[SIZE];
		strcat(prog_name, "./");
		strcat(prog_name,"psvis ");
		strcat(prog_name,command->args[0]);
		strcat(prog_name," | dot -Tx11");
		struct command_t *command=malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		parse_command(prog_name,command);
		process_command(command);
		return SUCCESS;
		
	}
	pid_t pid=fork();
	if (pid==0) // child
	{
		/// This shows how to do exec with environ (but is not available on MacOs)
	    // extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// increase args size by 2
		command->args=(char **)realloc(
			command->args, sizeof(char *)*(command->arg_count+=2));
	
		// shift everything forward by 1
		for (int i=command->arg_count-2;i>0;--i)
			command->args[i]=command->args[i-1];

		// set args[0] as a copy of name
		command->args[0]=strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		// stdout of these should be the stdin of the next one 		
		command->args[command->arg_count-1]=NULL;
		//do the redirection for the stdin by duplicating stdin to the given redirect
		if(command->redirects[0] != NULL){
			int file_dsc = open(command->redirects[0],O_RDONLY | O_WRONLY | O_APPEND,0666);
			if(file_dsc < 0) 
        		printf("Error opening the file\n", command->redirects[0]); 
			dup2(file_dsc,STDIN);
		}
		//does the redirection for > by duplicating stdout with dup2
		if(command->redirects[1] != NULL){
			//we truncate the file or create it if it doesnt exist 
			int file_dsc = open(command->redirects[1], O_RDONLY | O_WRONLY | O_TRUNC | O_CREAT,0666);
			if(file_dsc < 0) 
        		printf("Error opening the file\n", command->redirects[1]); 
			dup2(file_dsc,STDOUT);
		}
		//does the redirection for >>
		if(command->redirects[2] != NULL){
			//it doesnt truncate the file or create it. It opens it with O_APPEND
			int file_dsc = open(command->redirects[2],O_WRONLY | O_CREAT | O_RDONLY | O_APPEND, 0666);
			if(file_dsc < 0) 
        		printf("Error opening the file\n",command->redirects[2]); 
			dup2(file_dsc,STDOUT);
		}
		//if command has next call the process_command recursively
		if(command->next != NULL){
			int fd[2];
			if(pipe(fd)<0){
				fprintf(stderr, "%s\n","Pipe failed");
				exit(1);
			} 
			pid_t pid_next = fork();
			if(pid_next<0){
				fprintf(stderr, "%s\n","Pipe failed");
				exit(1);
			}
			if(pid_next>0){
				close(fd[WRITE_END]);
				dup2(fd[READ_END],STDIN);
				process_command(command->next);
				exit(0);
			} else{
				close(fd[READ_END]);
				dup2(fd[WRITE_END],STDOUT);
			}
		}
		//path gives us the directories to search for executables
		//it gives a full string in which directories are seperated by a ':' in between 
		int res = execute_command(command);
		if(res != SUCCESS){
			fprintf(stderr, "%s\n","Error happened while executing" );
			exit(1);
		}

		//execvp(command->name, command->args); // exec+args+path
	
		/// TODO: do your own exec with path resolving using execv()
	}
	else
	{
		char status[SIZE];
		if (!command->background)
			waitid(P_PID, pid, &status, WSTOPPED);  // wait until the currently running process is stopped by a signal
		return SUCCESS;
	}

	// TODO: your implementation here

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}
