#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/*  
 *  FDInfo Struct: stores information about a given std-pipeline (STDOUT or STDERR)
 *  this just allows me to easily pass information between main() and check_for_data()
 *  without needing to use 5+ parameters.
 */
struct FDInfo {
	int file_opened; // boolean, set to 1 once file has been opened
	char file_name[FILENAME_MAX]; // target filename
	int file_fd; // file descriptor for above file
	int term_fd; // file descriptor for terminal output (e.g. '1' for STDOUT)
	int pipe_fd[2]; // both ends of pipe, returned from pipe()
};

/*
 *  Function: init_FDInfo 
 *  	constructor for FDInfo structs. pretty self-explanatory.
 *  Parameters:
 *  	fd_info: pointer to an (ideally empty) FDInfo struct
 *  	fd: terminal file descriptor (e.g. '1', for STDOUT)
 *  	file_ext: desired file extension, including the 'dot'. (e.g. '.stdout', for STDOUT)
 *  	script_name: script name; first parameter of program.
 */
void init_FDInfo(struct FDInfo* fd_info, int fd, char* file_ext, char* script_name) {
	//simple init assignments
	(*fd_info).file_fd=0;
	(*fd_info).file_opened=0;
	(*fd_info).term_fd=fd;
	(*fd_info).pipe_fd[0] = 0;
	(*fd_info).pipe_fd[1] = 0;

	//assemble filename
	strcpy((*fd_info).file_name,script_name);
	strcat((*fd_info).file_name,file_ext);

	//create pipe
	pipe((*fd_info).pipe_fd);
}

/*
 *  Function: check_for_data 
 *  	deceptively complex. essentially handles everything that needs to
 *  	happen for both std-pipelines, from reading and writing to translating
 *  	depending on destination.
 *  Parameters:
 *  	fd_info: pointer to an (ideally populated) FDInfo struct
 *  	read_set: pointer to the select() call's fd_set struct 
 */
void check_for_data(struct FDInfo* fd_info, fd_set* read_set) {

	// check if pipe has data to be read
	if (FD_ISSET(fd_info->pipe_fd[0], read_set)) {

		char BUFFER[256];
		// WRITEBUF_TRANSL is an extra buffer for translating non-printable ASCII data. 
		// it could be smaller but the project spec says to use 256-byte buffers for 
		// reading and writing, so I figured it's better to be safe than sorry
		char WRITEBUF_TRANSL[256];
		int BYTES_IN_BUF;

		// get number of bytes in pipe 
		ioctl(fd_info->pipe_fd[0], FIONREAD, &BYTES_IN_BUF);

		// if bytes are to be written, but file hasn't been opened, open it and
		// mark having done so in the FDInfo struct
		if(BYTES_IN_BUF>0 && !fd_info->file_opened){
			(*fd_info).file_opened=1;
			(*fd_info).file_fd = open(fd_info->file_name, O_CREAT|O_WRONLY, 0666);
		}

		// loop to write in 256-byte increments, so as to not overload BUFFER
		while (BYTES_IN_BUF>0){

			// read up to 256 bytes from pipe into BUFFER
			int retval = read(fd_info->pipe_fd[0], &BUFFER, BYTES_IN_BUF>256?256:BYTES_IN_BUF);

			// write from BUFFER into the file as-is 
			write(fd_info->file_fd, BUFFER, retval);

			// loop through characters in BUFFER to translate the non-printable ones
			for(int i = 0; i < retval; i++){
				if(BUFFER[i]<32 || BUFFER[i]>126) sprintf(WRITEBUF_TRANSL,"<%02X>",BUFFER[i] & 0xFF);
				else sprintf(WRITEBUF_TRANSL,"%c",BUFFER[i]);
				// we write these one character at a time, because if more than 1/4 of BUFFER 
				// is non-printable then the translated BUFFER won't fit into a 256-byte buffer, 
				// and doing it in exactly four writes to prevent this feels like a wheel that 
				// doesn't need reinventing.
				write(fd_info->term_fd, WRITEBUF_TRANSL, strlen(WRITEBUF_TRANSL));
			}

			// subtract written characters from BYTES_IN_BUF
			BYTES_IN_BUF -= retval;
		}
	}
}

int main(int argc, char *argv[])
{
	// get first arg (script filename)
	char* SCR_name = argv[1];

	// initialize FDInfo structs
	struct FDInfo STDOUT_info;
	init_FDInfo(&STDOUT_info,1,".stdout",SCR_name);
	struct FDInfo STDERR_info;
	init_FDInfo(&STDERR_info,2,".stderr",SCR_name);

	// initialize arg array for supplied program
	char* ARGV_SUB[argc-1];

	// populate ARGV_SUB with args 2+ 
	for(int i = 2; i < argc; i++)
		ARGV_SUB[i-2]=argv[i];
	// null-terminate ARGV_SUB
	ARGV_SUB[argc-2] = '\0';

	// fork
	pid_t CHILD_PID = fork();
	if(!CHILD_PID){ // CHILD CODE
	
		// move stdout and stderr to the write ends of their respective pipes 
		dup2(STDOUT_info.pipe_fd[1],STDOUT_info.term_fd); //stdout
		dup2(STDERR_info.pipe_fd[1],STDERR_info.term_fd); //stderr

		// exec based on previously assembled ARGV_SUB
		int EXEC_RET = execvp(ARGV_SUB[0],ARGV_SUB);
		if(EXEC_RET==-1) exit(1);

	} else { // PARENT CODE

		// declare variables for select()
		fd_set READ_SET;
		struct timeval TIMEOUT;

		// loop indefinitely to keep checking the select() 
		while(1){
			// populate TIMEOUT
			TIMEOUT.tv_sec=1;
			TIMEOUT.tv_usec=1;

			// populate READ_SET
			FD_ZERO(&READ_SET);
			FD_SET(STDOUT_info.pipe_fd[0],&READ_SET);
			FD_SET(STDERR_info.pipe_fd[0],&READ_SET);

			// call select()
			// i'm using a ternary here to determine the higher file descriptor
			int retval = select((STDERR_info.pipe_fd[0]>STDOUT_info.pipe_fd[0]?STDERR_info.pipe_fd[0]:STDOUT_info.pipe_fd[0])+1, &READ_SET, NULL, NULL, &TIMEOUT);

			// check both pipes for data and do as is needed
			check_for_data(&STDOUT_info,&READ_SET);
			check_for_data(&STDERR_info,&READ_SET);

			// if timed out, check whether child has exited — if so, break loop.
			// i tried just using this waitpid() as a loop condition, but it would exit too early on longer files.
			if(!retval && waitpid(CHILD_PID,NULL,WNOHANG) != 0) break;
		}
	}

	// close STDOUT and STDERR files (if needed), exit with basic error reporting 
	int CLOSE_RET_STDOUT, CLOSE_RET_STDERR = 0;
	if(STDOUT_info.file_opened) CLOSE_RET_STDOUT = close(STDOUT_info.file_fd);
	if(STDERR_info.file_opened) CLOSE_RET_STDERR = close(STDERR_info.file_fd);
	if (!CLOSE_RET_STDOUT&&!CLOSE_RET_STDERR) exit(0);
	else exit(1);
}
