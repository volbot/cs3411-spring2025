#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int check_for_data(int fd, fd_set* read_set) {
	if (FD_ISSET(fd, read_set)) {
		char BUFFER[256];
		int BYTES_IN_BUF;
		// get number of bytes in READ_FD into BYTES_IN_BUF
		ioctl(fd, FIONREAD, &BYTES_IN_BUF);
		// read that many bytes from READ_FD into BUFFER
		int retval = read(fd, BUFFER, BYTES_IN_BUF);
		// write to STDOUT from BUFFER
		write(1, BUFFER, retval);
		return 1;
	}
	return 0;
}

int main( /*int argc, char *argv[]*/ void )
{

	const int READ_FD = 0;
	const int TIMEOUT_SEC = 3;

	// declare fd_set variable
	fd_set READ_SET;
	// declare timeout struct
	struct timeval TIMEOUT;

	//open "transit" pipe
	int PIPE_FD = open("transit", O_RDWR | O_NONBLOCK); 

	//loop to check for data indefinitely
	while(1){
		// populate TIMEOUT (select modifies, so we re/set within loop)
		TIMEOUT.tv_sec = TIMEOUT_SEC;
		TIMEOUT.tv_usec = 0;

		// populate READ_SET (select modifies, so we re/set within loop)
		// clear READ_SET
		FD_ZERO(&READ_SET);
		// add READ_FD and PIPE_FD to READ_SET 
		FD_SET(READ_FD,&READ_SET);
		FD_SET(PIPE_FD,&READ_SET);

		// call select()
		int retval = select((READ_FD>PIPE_FD?READ_FD:PIPE_FD)+1, &READ_SET, NULL, NULL, &TIMEOUT);

		// check if READFD has data
		check_for_data(READ_FD, &READ_SET);
		check_for_data(PIPE_FD, &READ_SET);


		// if timed out, write 'tick'
		if (!retval) write(1, "tick\n", strlen("tick\n"));
	}
}
