#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFSIZE 1024
#define TOKEN_BUFSIZE 128
#define TOKEN_DELIM " \t\r\n\a"
#define PATH_BUFSIZE 128
#define Redirect_Out 1
#define No_Redirect 0

void printout_array(char **a, int n)
{
	for (int i = 0; i < n; ++i)
	{
		printf("%s\t", a[i]);
	}
}

void print_error()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}

char* read_line(FILE *stream) // read one line
{	
	char *line = NULL;
	ssize_t nread;
	size_t len = 0;

	if ((nread = getline(&line, &len, stream)) == -1)//EXIT SUCESSFUL when hit EOF
		exit(0);

	// printf("After\n");

	size_t length = strlen(line); //replace the new line with null
	if (line[length-1] == '\n') 
	{	
		// printf("line address length-1:%p\n", &line[length-1]);
		line[length-1] = '\0';
	}
	// printf("After\n");

	// printf("line address:%p\n", &line);
	return line;
}


char **copy_strings(char **strings, int n) //copy strings up until n
{	
	char **s = malloc((n+1) * sizeof(char*));
	// printf("N:%d\t",n);
	int i;
	for (i = 0; i < n; ++i){
		s[i] = malloc(TOKEN_BUFSIZE* sizeof(char));
		// printf("index:%d copy:%s\t", i, strings[i]);
		strcpy(s[i], strings[i]);
	}
	// printf("s[i]:%s\n", s[i-1]);
	s[n] = NULL;
	// printf("\n");
	return s;
}


int count_strings(char **strings)
{
	int n = 0;
	while (*strings++ != NULL)
		n++;

	return n;
}

char **parseCommands(char *line) // not considering allocation error
{
	// printf("Now parsing\n");		

	int bufsize = TOKEN_BUFSIZE;
	char **commands = malloc(bufsize * sizeof(char*));
	char *found;
	int index = 0;
	char* copy = strdup(line);
	while ((found = strsep(&copy, "&")) != NULL){ // check why > is included in commands
		commands[index++] = strdup(found);
			//reallocate if needed
		if (index >= bufsize){
			bufsize += TOKEN_BUFSIZE;
			commands = realloc(commands, bufsize * sizeof(char*));
		}		
	}

	commands[index] = NULL;
	// printf("FINISH Parsing\n");
	return commands;
}

int checkForRedirect(char* line){

	for (int i = 0; i < strlen(line); ++i){
		if (line[i] == '>')
			return 1;
	}

	return 0;
}

int getRedirectIndex(char *line){
	for (int i = 0; i < strlen(line); ++i){
		if (line[i] == '>' ){
			return i;
		}
	}

	return 0;
}

char* getSource(char* line){
	int index = getRedirectIndex(line);
	int index2 = 0;
	// printf("Index:%d\n", index);

	char* src = malloc(strlen(line) * sizeof(char));
	//if dest[0] == '\0', no destination on the same line
	src[0] = '\0';
	for (int j = 0; j < index; ++j){
		src[index2++] = line[j];
	}

	return src;
}

char* getRedirectLocation(char* line){

	int index = getRedirectIndex(line) + 1;
	int index2 = 0;
	// printf("Index:%d\n", index);

	char* dest = malloc(strlen(line) * sizeof(char));
	//if dest[0] == '\0', no destination on the same line
	dest[0] = '\0';
	for (int j = index; j < strlen(line); ++j){
		if (line[j] == '>'){
			// printf("Extra >\n");
			dest[0] = '>';
			break;
		}
		dest[index2++] = line[j];
	}

	return dest;

}

char **parse(char *line, int* redirect) // not considering allocation error
{
	// printf("Now parsing\n");	
	*redirect = 0;	
	int bufsize = TOKEN_BUFSIZE;
	char **commands = malloc(bufsize * sizeof(char*));
	char *found;
	int index = 0;
	char* copy = line;
	int should_stop = 0;
	found = strtok(copy, TOKEN_DELIM);
	while (found != NULL){ // check why > is included in commands
 		if (should_stop == 1){
 			// printf("12\n");
 			// printf("H\n");

 			strcpy(commands[0],"bad");
 			index++;
 			break;
 		}
		// printf("found:%s\n", found);
		if (*redirect == Redirect_Out){
			// printf("1\n");
			commands[index++] = strdup(found);
			should_stop = 1;
			// printf("1\n");
		}



		if (checkForRedirect(found) == 1){
			// printf("2\n");
			if (*redirect == Redirect_Out){
				// printf("now should stop\n");
				// printf("G\n");
 				strcpy(commands[0],"bad");
 				break;
				// should_stop = 1;
			}
			*redirect = Redirect_Out;
			// if after > has the file name with no whitespace, get the destination
			char *src = getSource(found);
			char *dest = getRedirectLocation(found);
			// printf("Src:%s\n", src);
			// printf("Dest:%s\n", dest);

			if (src[0] != '\0'){
				commands[index++] = strdup(src);
			}


			if (dest[0] != '\0' && dest[0] != '>'){
				commands[index++] = strdup(dest);
				should_stop = 1; // should exit the loop after this, if not there is errors
			}

			else if (dest[0] == '>'){
				// printf("HI");
				 // printf("I\n");

 				strcpy(commands[0],"bad");
 				break;
			}

			free(src);
			free(dest);
		}
	
		else if (*redirect == No_Redirect){		
			// printf("Index:%d\n", index);
			commands[index++] = strdup(found);

		}
		//reallocate if needed
		if (index >= bufsize){
			bufsize += TOKEN_BUFSIZE;
			commands = realloc(commands, bufsize * sizeof(char*));
		}

		found = strtok(NULL, TOKEN_DELIM);
			
	}

	// printf("Exit parsing\n");
	// for (int i = 0; i < index; ++i)
	// {
	// 	printf("Index:%d-command:%s\n", i, commands[i]);
	// }
	// printf("parsing:%s\n", commands[index-1]);
	
	if (index != 0)
		commands[index] = NULL;

	else{
		// printf("HI\n");
		// printf("index:%d\n", index);
		commands[index] = strdup(" ");
	}
	// printf("FINISH Parsing\n");
	return commands;
}

int built_in_exit(char **commands)
{
	if (commands[1] == NULL)
	{	
		// printf("now exiting...\n");
		exit(0);
	}
	print_error();

	return 1;
}

int built_in_cd(char **commands)
{
	if (commands[2] != NULL || commands[1] == NULL)
		print_error();

	else {
		if (chdir(commands[1]) == -1)
			print_error(); //print error if the path is wrong
	}

	return 1;
}

int built_in_path(char **commands, char **paths)
{
	int index = 1;
	*paths[0] = '\0';
	while (commands[index] != NULL){
			// printf("Copy path:%s\n", commands[index]);
			strcat(*paths,commands[index]);
			strcat(*paths, " ");

			index++;
	}
	// printf("built in paths:%s\n", *paths);

	return 1;
}

int execute_built_in(char** command, char** paths)
{	
	if (strcmp(command[0], "exit") == 0)
	{
		// printf("running exit\n");
		return built_in_exit(command);
	}

	else if (strcmp(command[0], "cd") == 0)
		return built_in_cd(command);

	return built_in_path(command, paths);
}

int run(char** commands, char** path, int* redirect)
{	
	char* final_path;

	// printf("Running\n");
	int n = count_strings(commands);
	// printf("n:%d\n", n);
	pid_t pids[n];
	// printf("Before parse\n");
	// for (int k = 0; k < count_strings(commands); ++k)
	// {
	// 	printf("commands:%s\t", commands[k]);
	// }
	// printf("Select path from:%s\n", *path);
	char* path_dup = strdup(*path);
	char **paths = parse(path_dup, redirect);
		// printf("After parse::Select path from:%s\n", *path);

	// printf("n_strings_for_each:%d\n", n_strings_for_each[0]);
	for (int i = 0; i < n; ++i){
		// printf("Before forking path:%s\n", *path);
		// printf("i:%d------------------------------------------------------\n", i);
		char **command = parse(commands[i], redirect);
		// printf("After parse\n");
		// for (int k = 0; k < count_strings(command); ++k)
		// {
		// 	printf("commands:%s\t", command[k]);
		// }
		// printf("\n");


		if (strcmp(command[0], "bad") == 0)
		{
			// printf("F");
			print_error();
			continue;
		}

		if (strcmp(command[0], " ") == 0)
		{
			// printf("Empty\n");
			continue;
		}
		if (strcmp(command[0], "cd") == 0 || strcmp(command[0], "exit") == 0 || strcmp(command[0], "path") == 0){
			 execute_built_in(command, path);
			 continue;
		}



		pids[i] = fork();
		// printf("pids[i]:%d\n", pids[i]);
		// printf("After forking]\n");
		if (pids[i] < 0){
			printf("Fork failed");
			print_error();
			exit(1);
		}

		else if (pids[i] == 0){ //child
			// printf("Child\n");

			int command_size = count_strings(command);

					// printf("commands[1]:%s\n", commands[1]);

			// printf("Parent PID:%ld\n", (long)getpid());

			// printf("n_string:%d\n", n_strings_for_each[i]);
			// printf("Child PID:%ld\t", (long)getpid());
			// printf("Parent PID:%ld\t", (long)getppid());
			// printf("i = %d\ts:", i);
				// printf("Before copying strings path:%s\n", *path);
			// printf("commands:\t");
			// printout_array(commands, n_strings_for_each[i]);
			// printf("Before running path:%s\n", *path);
				// printf("Redirect:%d\n", *redirect);

			if (*path[0] == '\0' || strcmp(command[0],"bad") == 0){
				// printf("WHY\n");
				print_error();
				exit(1);
			}

			else if (*redirect == Redirect_Out){

				// printf("n of strings:%d\n", n);
				 // printf("Redirecting\n");
				 *redirect = 0;
				if (command_size < 2){
					// printf("A");
					print_error();
					for (int i = 0; i < n; ++i){
						free(command[i]);
					}
					free(command);
					exit(1);
				}

			// printf("n:%d\n", n);
	 			char **temp = copy_strings(command, command_size - 1); //exec reclaims this memory afterwwards
				// printf("n-1:%s\n", s[n-1]);	
				int j = 0;
				bool right_path = false;
				while (paths[j] != NULL){
					strcat(paths[j], "/./");
					strcat(paths[j], temp[0]);
					// printf("Current path:%s\n", paths[j]);
					if (access(paths[j], X_OK) == 0){
						right_path = true;
						// printf("RIGHT PATH\n");
						final_path = strdup(paths[j]);
						break;
					}

					++j;
				}

				if (right_path == false){
					// printf("B");

					print_error();
					continue;
				} //no right path
				// printf("Now path:%s\n", final_path);

				// printf("Now path:%s\n", *path);
				// printf("dest:%s\n", command[command_size - 1]);	
				*redirect = 0;				
				freopen(command[command_size - 1], "w", stdout);
				freopen(command[command_size - 1], "w", stderr); 
				execv(final_path, temp);
				// printf("EXEC FAILED\n");
				for (int i = 0; i < command_size; ++i){
						free(command[i]);
				}
				free(command);
				temp = NULL;
				// printf("C");

				print_error();
				exit(1);
			}

			else{
				// printf("No redirect\n");
				// printf("n of strings:%d\n", n_strings_for_each[i]);
				// printout_array(s, n_strings_for_each[i]);
				int j = 0;
				bool right_path = false;
				while (paths[j] != NULL){
					strcat(paths[j], "/./");
					strcat(paths[j], command[0]);
					// printf("Current path:%s\n", paths[j]);

					if (access(paths[j], X_OK) == 0){
						right_path = true;
						// printf("RIGHT PATH\n");

						 final_path = strdup(paths[j]);
						break;
					}

					++j;
				}

				if (right_path == false){
					// printf("D");
					print_error();
					continue;
				} //no
				// printf("Now path:%s\n", final_path);

				execv(final_path, command);
				// printf("EXEC FAILED\n");
				// printf("current n:%d\n", n_strings_for_each[i]);
				for (int i = 0; i < command_size; ++i){
						free(command[i]);
				}
				free(command);
				// If exec fail then print error and exit
				// printf("E");
				print_error();
				exit(0);
			}
		}


		// printf("Parent\n");
	}

	// printf("Parent:Select path from:%s\n", *path);

		// printf("Parent waiting...\n");

	int status;
	pid_t pid;
	while (n > 0){
		pid = wait(&status);
		// printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
		// printf("Parent PID:%ld\n", (long)getpid());
		*redirect = No_Redirect;
		--n;
	}
		
	return 1;
	
		
}





int main(int argc, char *argv[])
{	
	char *line;
	char **commands;
	int status;
	char *paths = malloc(BUFSIZE * sizeof(char));
	strcpy(paths, "/bin/"); //default
	FILE *stream = stdin; //default
	if (argc > 2){
		print_error();
		exit(1);
	}

	if (argv[1] != NULL){
		stream = fopen(argv[1], "r");
	}

	if (stream == NULL){
		print_error();
		exit(1);
	}


	
	do {
		int redirect = No_Redirect;

		// char *path_dup = strdup(paths);
		// printf("------------------------------------------------------\n");
		if (stream == stdin)
			printf("wish> ");
		// printf("Start of the loop - path:%s\n", paths);
		line = read_line(stream);
		// paths = path_dup;
		// printf("address of path:%p\n", &paths);
		// printf("After reading line - path:%s\n", paths);
		// printf("After reading line - path:");

		// for (int i = 0; i < PATH_BUFSIZE; ++i)
		// {	
		// 	if (paths[i] == '\0')
		// 		printf("END ");
		// 	printf("%c ", paths[i]);

		// }

		// printf("\n");
		// printf("Main after read_line line:%s\n", line);
		commands = parseCommands(line);

		// printf("After parsing - path:%s\n", paths);
		// printf("Main after parsing - line:%s\n", line);
		// int i = 0;
		// while (commands[i] != NULL)
		// {
		// 	printf("Main after parsing-commands:%s\n", commands[i++]);
		// }

		// printf("Main before execute path:%s\n", paths);
		// printf("print:%s\n", commands[0]);
		// printf("While loop path before execute:%s\n", paths);

		status = run(commands, &paths, &redirect);
		// printf("Main after execute path:%s\n", paths);

		// printf("While loop path after execute:%s\n", paths);
		free(line);
		int n = count_strings(commands);
		// printf("n:%d\n", n);
		for (int i = 0; i < n; ++i){
			// printf("i:%d\tfree\n", i);
			free(commands[i]);
		}
		// printf("DONE\n");
		free(commands);
		// free(path_dup);
		line = NULL;
		commands = NULL;
		// path_dup = NULL;
	} while (status);

	fclose(stream);
	free(paths);
	return 0;
}

       



