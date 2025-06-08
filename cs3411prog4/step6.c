#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int check_for_data(int fd, fd_set* read_set, int writefd_term, int writefd_file) {
	if (FD_ISSET(fd, read_set)) {
		char BUFFER[256];
		int BYTES_IN_BUF;
		// get number of bytes in READ_FD into BYTES_IN_BUF
		ioctl(fd, FIONREAD, &BYTES_IN_BUF);
		// read that many bytes from READ_FD into BUFFER
		int retval = read(fd, BUFFER, BYTES_IN_BUF);
		// write to supplied terminal pipe from BUFFER
		write(writefd_term, BUFFER, retval);
		// write to supplied file descriptor from BUFFER
		write(writefd_file, BUFFER, retval);
		return 1;
	}
	return 0;
}

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

	//create pipes
	int PIPE_STDOUT_fd[2];
	pipe(PIPE_STDOUT_fd);
	int PIPE_STDERR_fd[2];
	pipe(PIPE_STDERR_fd);

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
	if(!CHILD_PID){ //CHILD CODE
		//move stdout and stderr to the write ends of the respective pipes 
		dup2(PIPE_STDOUT_fd[1],1); //stdout
		dup2(PIPE_STDERR_fd[1],2); //stderr
					   //
		//exec based on previously assembled ARGV_SUB
		int EXEC_RET = execvp(ARGV_SUB[0],ARGV_SUB);
		if(EXEC_RET==-1) exit(1);

	} else { //PARENT CODE
		//declare variables for select()
		fd_set READ_SET;
		struct timeval TIMEOUT;

		//loop condition checks at every iteration if child is still running
		while(waitpid(CHILD_PID,NULL,WNOHANG) == 0){
				
			//populate TIMEOUT
			TIMEOUT.tv_sec=1;

			//populate READ_SET
			FD_ZERO(&READ_SET);
			FD_SET(PIPE_STDOUT_fd[0],&READ_SET);
			FD_SET(PIPE_STDERR_fd[0],&READ_SET);

			//call select()
			int retval = select((PIPE_STDERR_fd[0]>PIPE_STDOUT_fd[0]?PIPE_STDERR_fd[0]:PIPE_STDOUT_fd[0])+1, &READ_SET, NULL, NULL, &TIMEOUT);

			//check both pipes for data and do as is needed
			check_for_data(PIPE_STDOUT_fd[0],&READ_SET,1,SCR_STDOUT_fd);
			check_for_data(PIPE_STDERR_fd[0],&READ_SET,2,SCR_STDERR_fd);
		}
	}

	//close STDOUT and STDERR files, exit with basic error reporting 
	int CLOSE_RET_STDOUT = close(SCR_STDOUT_fd);
	int CLOSE_RET_STDERR = close(SCR_STDERR_fd);
	if (!CLOSE_RET_STDOUT&&!CLOSE_RET_STDERR) exit(0);
	else exit(1);
}
