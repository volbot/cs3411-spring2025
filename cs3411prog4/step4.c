#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int main( /*int argc, char *argv[]*/ void )
{
	char BUFFER[256];

	const int READ_FD = 0;
	const int TIMEOUT_SEC = 3;

	// declare fd_set variable
	fd_set READ_SET;
	// declare timeout struct
	struct timeval TIMEOUT;
	//loop to check for data indefinitely
	while(1){
		// populate TIMEOUT (select modifies, so we re/set within loop)
		TIMEOUT.tv_sec = TIMEOUT_SEC;
		TIMEOUT.tv_usec = 0;

		// populate READ_SET (select modifies, so we re/set within loop)
		// clear READ_SET
		FD_ZERO(&READ_SET);
		// add READ_FD to READ_SET 
		FD_SET(READ_FD,&READ_SET);

		// call select()
		int retval = select(READ_FD+1, &READ_SET, NULL, NULL, &TIMEOUT);
		int BYTES_IN_BUF;

		// check if READFD has data
		if (FD_ISSET(READ_FD, &READ_SET)) {
			// get number of bytes in READ_FD into BYTES_IN_BUF
			ioctl(READ_FD, FIONREAD, &BYTES_IN_BUF);
			// read that many bytes from READ_FD into BUFFER
			retval = read(READ_FD, BUFFER, BYTES_IN_BUF);
			// write to STDOUT from BUFFER
			write(1, BUFFER, retval);
		}
		// if timed out, write 'tick'
		if (!retval) write(1, "tick\n", strlen("tick\n"));
	}
}
