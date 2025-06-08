#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{

	//get first arg
	char* SCR_name = argv[1];

	//assemble STDOUT and STDERR file names
	size_t SCR_name_len = strlen(SCR_name);
	char SCR_STDOUT_name[SCR_name_len+7];
	char SCR_STDERR_name[SCR_name_len+7];
	strcpy(SCR_STDOUT_name,SCR_name);
	strcat(SCR_STDOUT_name,".stdout");
	strcpy(SCR_STDERR_name,SCR_name);
	strcat(SCR_STDERR_name,".stderr");

	//open STDOUT and STDERR files
	int SCR_STDOUT_fd = open(SCR_STDOUT_name, O_CREAT|O_WRONLY, 0666);
	if(SCR_STDOUT_fd < 0) exit(1);
	int SCR_STDERR_fd = open(SCR_STDERR_name, O_CREAT|O_WRONLY, 0666);
	if(SCR_STDERR_fd < 0) exit(1);

	//create write buffer
	char WRITEBUF[256];
	
	//initialize arg array for supplied program
	char* ARGV_SUB[argc-1];

	//write remaining args to STDOUT file
	ssize_t WRITE_COUNT = 0;
	for(int i = 2; i < argc; i++){
		/*
		//load arg into WRITEBUF
		size_t arg_len = strlen(argv[i]);
		strcpy(WRITEBUF,argv[i]);
		//write WRITEBUF into STDOUT file
		WRITE_COUNT = write(SCR_STDOUT_fd,WRITEBUF,arg_len);
		if(WRITE_COUNT!=arg_len) exit(1);
		*/

		//populate ARGV_SUB to get proper sliced array
		ARGV_SUB[i-2]=argv[i];
	}
	//null-terminate ARGV_SUB
	ARGV_SUB[argc-2] = '\0';

	//fork
	pid_t CHILD_PID = fork();
	if(!CHILD_PID){
		//CHILD CODE
		//move stdout and stderr to the files
		dup2(SCR_STDOUT_fd,1);
		dup2(SCR_STDERR_fd,2);
		//exec based on previously assembled ARGV_SUB
		int EXEC_RET = execvp(ARGV_SUB[0],ARGV_SUB);
		if(EXEC_RET==-1) exit(1);
	} else {
		//parent code
		//wait for child
		wait(NULL);
	}

	//close STDOUT and STDERR files, exit with basic error reporting 
	int CLOSE_RET_STDOUT = close(SCR_STDOUT_fd);
	int CLOSE_RET_STDERR = close(SCR_STDERR_fd);
	if (!CLOSE_RET_STDOUT&&!CLOSE_RET_STDERR) exit(0);
	else exit(1);
}
