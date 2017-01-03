/*	*	*	*	*	*	*	*	*	* * * * * * * * * * * * * * * * * * * * *
 * Unix Shell myshell	-	Author: Peter Lorei (pnl4@pitt.edu)	*
 * CS 449 Project 5	-	Due: 12/8/14 													*
 *	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	* *	*	*	*	*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#include <errno.h>
//-----Prototypes-------
int doCommand(char *inputArr[], char *inputString, int numArgs);

//-----Functions--------
/*      readLine
   Reads line of user input, tokenizes and places in inputArr.
   Places original (untokenized) string in inputString.
   Returns the number of arguments read.
*/
int readLine(char *inputArr[], char *buf, char *inputString) {
	char 	*token;
	int 	index = 0;
	int 	num = 0;
	int 	i;

	fgets(buf, 50, stdin); //get input from user
	strcpy(inputString, buf);

	token  = strtok(buf, " \t\n()<>|&;"); //split strings on each of these elements
	while(token != NULL) {		//need to know if < or > entered
		strcpy(inputArr[index++], token);
		token  = strtok(NULL, " \t\n()<>|&;");
		num++;
	}//end while
	inputArr[index] = NULL; //set to null so execvp can work
	return num; //return the number of strings tokenized
}//end readLine

/*		isUnix
   Determines whether or not given command is a valid UNIX command.
   If valid, forks and executes given command.
   Returns 1 if UNIX command, 0 if not.
*/
int isUnix(char *inputArr[]) {
	char	returnVal = 0;	//Value to return. 1 if valid UNIX command, 0 if not
	int	fd[2];		//file descriptors for pipe
	pid_t	pid;		//process ID

	if (pipe(fd) == -1) 	//create pipe to write from child to parent
       		printf("Failed to create pipe\n");
	pid = fork();
	if (pid == -1) {
    		printf("Failed to fork\n");
    	}
  	else if(pid==0) { //if child
  		char rVal = 1;
  		char *rP = &rVal; //return value pointer

  		execvp(inputArr[0], inputArr);
  		printf("%s\n", strerror(errno));
  		//if execvp returns then it failed to execute, send failure to parent
  		close(fd[0]); 		// Close unused end (only write from child)
       		write(fd[1], rP, 1); 	//write '1' if exec fails
        	close(fd[1]); 		//Close pipe
  		exit(1);
  	}
  	else {
  		int status;
   		if(wait(&status)==-1) {
   			printf("Error! Unable to wait for child process\n");
   			return 0;
   		}
  		char fromPipe = 0; 	//assume success
  		char *fP = &fromPipe; 	//pointer to fromPipe
   		close(fd[1]);
  		read(fd[0], fP, 1); 	//read 1 byte
  		returnVal = 1 - fromPipe;
	}
	return returnVal;
}//end isUnix

/*	changeDirectory
   Changes directory when cd is called.
   cd with no arguments or cd ~ changes to home directory.
   Returns 1 if unsuccessful, 0 if succesful.
*/
int changeDirectory(char *inputArr[], int numArgs) {
	int returnVal;
	if((numArgs == 1)||(strcmp(inputArr[1], "~")==0) )
		returnVal = chdir(getenv("HOME"));
	else
		returnVal = chdir(inputArr[1]);

	if(returnVal!=0)
		printf("%s\n", strerror(errno));

	return returnVal;
}//end changeDirectory

/*	isRedirect
  Reads input string and checks for presence of > or <.
  Returns 0 if no redirection, 1 if input redirected(<) or 2 if output redirected(>)
*/
int isRedirect(char *inputString) {
	int length = strlen(inputString);
	int i;

	for(i = 0; i<length; i++) {
		if(inputString[i]=='<')
			return 1;
		if(inputString[i]=='>')
			return 2;
	}
	return 0;
}//end isRedirect

/*	redirectIn
  Redirects input from keyboard to specified file and runs command.
  Returns 1 if error occurred, 0 if succesful.
*/
int redirectIn(char *inputArr[], int numArgs) {
	int 	i;
	int 	returnVal = 0;
	char 	*newArr[numArgs];

	for(i = 0; i<numArgs; i++)
		newArr[i] = (char *)malloc(30);
	for(i = 0; i<(numArgs-1); i++)
		strcpy(newArr[i], inputArr[i]);
	newArr[numArgs-1] = NULL;
	FILE *fp = freopen(inputArr[numArgs-1], "r", stdin);
	if(fp==NULL||isUnix(newArr)!= 1) {
		if(fp==NULL)
			printf("%s\n", strerror(errno));
		returnVal = 1;
	}
	fclose(fp);
	freopen ("/dev/tty", "r", stdin); //return input to the keyboard

	for(i = 0; i<numArgs; i++)
		free(newArr[i]);

	return returnVal;
}//end redirectIn

/*	redirectOut
  Redirects output from console to specified file and runs command.
  Returns 1 if error occurred, 0 if succesful.
*/
int redirectOut(char *inputArr[], int numArgs) {
	int 	i;
	int 	returnVal = 0;
	char 	*newArr[numArgs];

	for(i = 0; i<numArgs; i++)
		newArr[i] = (char *)malloc(30);
	for(i = 0; i<(numArgs-1); i++)
		strcpy(newArr[i], inputArr[i]);
	newArr[numArgs-1] = NULL; //write to file and remove filename

	FILE *fp = freopen(inputArr[numArgs-1], "w", stdout);
	if(fp==NULL||isUnix(newArr)!= 1) {
		if(fp==NULL)
			printf("%s\n", strerror(errno));
		returnVal = 1;
	}
	fclose(fp);
	freopen ("/dev/tty", "a", stdout); //return to printing to console

	for(i = 0; i<numArgs; i++)
		free(newArr[i]);

	return returnVal;
}//end redirectOut

/*	runEx
  Run executable that is first argument of user input.
  (Adds './' to the beginning of executable name)
  Return 0 if succesful. 1 if an error occurred.
*/
int runEx(char *exec) {
	char	returnVal = 0;	//Value to return. 1 if valid UNIX command, 0 if not
	int	fd[2];		//file descriptors for pipe
	char 	*envp[] = { NULL }; //run programs without arguments
	char 	*argv[] = { NULL };
	char	*command = (char*)malloc(30);

	strcpy(command, "./");
	strcat(command, exec); //put in form ./executable

	pid_t pid = fork();
	if (pid == -1) {
    		printf("Failed to fork\n");
    	}
  	else if(pid==0) {
  		char rVal = 1;
  		char *rP = &rVal; //return value pointer
  		execve(command, argv, envp);
  		printf("%s\n", strerror(errno));
  		//if execvp returns then it failed to execute
  		close(fd[0]); 		// Close unused end (only write from child)
       		write(fd[1], rP, 1); 	//write '1' if exec fails
        	close(fd[1]); 		//Close pipe
  		exit(1);
  	}
  	else {
  		int status;
   		wait(&status);
  		char fromPipe = 0; 	//assume success
  		char *fP = &fromPipe; 	//pointer to fromPipe
   		close(fd[1]);
  		read(fd[0], fP, 1); 	//read 1 byte
  		returnVal = fromPipe;	//return 0 if succesful
	}
	free(command);
	return returnVal;
}//end runEx

/*	getTime
  Uses time() syscall to get and print user and system time spent on execution.
  Times and runs command given.
  Return 0 if succesfull, 1 if error.
*/
int getTime(char *inputArr[], char *inputString, int numArgs) {
	int returnVal = 0; //return value, assume success
	int i;
	static struct tms start, end;	//variables to pass to time()
	double 	userTime, sysTime;
	long double clockticks = sysconf(_SC_CLK_TCK); //get clock ticks per second

	char *newArr[numArgs-1]; //omit 'time' from new command array
	for(i = 0; i<numArgs-1; i++)
		newArr[i] = (char *)malloc(30);
	for(i = 0; i<(numArgs-1); i++)
		strcpy(newArr[i], inputArr[i+1]);
	newArr[numArgs-1] = NULL; //set last item to null so execvp can run

	if(times(&start)==-1)
		printf("%s\n", strerror(errno));
	returnVal = doCommand(newArr, inputString, numArgs-1);
	//newArr has 1 less argument than inputArr (removed 'time')
	if(times(&end)==-1)
		printf("%s\n", strerror(errno));

	for(i = 0; i<numArgs-1; i++)
		free(newArr[i]);

	userTime = (end.tms_cutime-start.tms_cutime)/(double)clockticks;
	sysTime  = (end.tms_cstime-start.tms_cstime)/(double)clockticks;
	//use ctime and cstime to include time of work done by child processes
	printf("\nuser\t%dm%lfs\nsys\t%dm%lfs\n",
		(int)(userTime/60),(userTime - (int)(userTime/60)),
			(int)(sysTime/60), (sysTime - (int)(sysTime/60)) );

	return returnVal;
}//end getTime

/*	doCommand
  Finds type of command input and redirects to correct function.
  Return 1 and print error if error occurs.
*/
int doCommand(char *inputArr[], char *inputString, int numArgs) {
	if(numArgs!=0) { //avoid accessing memory that does not exist
		if(strcmp(inputArr[0], "exit")==0 )
			exit(EXIT_SUCCESS); //exit syscall
		else if(strcmp(inputArr[0], "cd")==0 ) {
			if(changeDirectory(inputArr, numArgs) != 0) {
				printf("Error! Invalid Directory\n");
				return 1;
			}
		}
		else if(strcmp(inputArr[0], "time")==0 ) {
			int timeVal = getTime(inputArr, inputString, numArgs);
			if(timeVal==1) {
				printf("Error! Unable to get time for command\n");
				return 1;
			}
		}
		else if(isRedirect(inputString)==1) {
			int e = redirectIn(inputArr, numArgs);
			if(e==1) {
				printf("Error! Unable to read from file\n");
				return 1;
			}
		}
		else if(isRedirect(inputString)==2) {
			int e = redirectOut(inputArr, numArgs);
			if(e==1) {
				printf("Error! Unable to write to file\n");
				return 1;
			}
		}
		else if(isUnix(inputArr)) { }
		else {
			if(runEx(inputArr[0])) {
				printf("Error! Unable to run file\n");
				return 1;
			}
		}//end else
	}//end if
	return 0; //if this point is reached, then return success(0)
}//end doCommand

int main(int argc, char *argv[]) {
	do {
		int	i; //lcv
		char	*bufP = (char *)malloc(50); //buffer to hold entire input
		char	*inputArr[10]; //holds tokenized command strings
		char	*inputString = (char *)malloc(50); //holds entire input as entered
		for(i = 0; i < 10; i++)
			inputArr[i] = (char *)malloc(30); //allocate space for strings

		printf("(My Shell)$ ");
		int numArgs = readLine(inputArr, bufP, inputString);
		doCommand(inputArr, inputString, numArgs);

		free(bufP);
		free(inputString);
		for(i = 0; i < 10; i++)
			free(inputArr[i]);
	}while(1); //will loop until exit syscall is made
	return (0);
}//end main
