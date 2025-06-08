#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>

// define conx state macros + some others
#define CONX_STATE_NOT_CONNECTED 0
#define CONX_STATE_CONNECTED 1
#define CONX_STATE_LISTING 2

// set maximum connections number to the maximum FD (as per man select(2))
#define MAX_CONXS FD_SETSIZE

// struct for conx information
struct ConxState {
	int fd;		// file descriptor returned by accept()
	int state;	// current state of conx, set according to macros above
	int pos;	// current position for 'list' mode; stored as num. blocks
			// 	i.e. pos:2 == 2 * 128 == 256 bytes in
};

/*
 *  init_ConxState
 *  	initializes a ConxState struct to an fd, or alternatively empties it.	
 *  parameters:
 *  	conx:	pointer to ConxState struct, refers to client
 *  	fd:	file descriptor returned by accept().
 *  		if 0, initialize struct to not-connected
 */
void init_ConxState(struct ConxState* conx_state, int fd) {
	if(fd){
		conx_state->fd = fd;
		conx_state->state = CONX_STATE_CONNECTED;
	} else { // if no FD supplied, zero out
		conx_state->fd = 0;
		conx_state->state = CONX_STATE_NOT_CONNECTED;
	}
	conx_state->pos = 0;
}

/*
 *  list_to_client
 *  	contains logic for listing mode, to help with redundancy in main()
 *  parameters:
 *  	conx: 		pointer to ConxState struct, refers to client
 *  	logfile_fd: 	file descriptor for logfile
 */
void list_to_client(struct ConxState* conx, int logfile_fd) {
	char list_readbuf[129];
	// seek to conx's position in file
	lseek(logfile_fd,conx->pos * 128,SEEK_SET);
	// read up to 129 bytes
	int numread = read(logfile_fd,list_readbuf,129);
	// if all 129 bytes are read, then more data exists — increment pos for next call
	if(numread==129){
		(*conx).pos++;
		// if not, then this is the end of the data — exit list mode
	} else {
		(*conx).state=CONX_STATE_CONNECTED;
		(*conx).pos = 0;
	}
	// write at most 128 bytes. we read 129 to see if the next byte has data, but
	// we don't want to print it
	write(conx->fd,list_readbuf,numread>128?128:numread);
	write(conx->fd,"\n",1);
}

//################
//## TCP SERVER ##
//################
int main(int argc, char *argv[])
{
	// define constants for port binding
	int listener, conn, length;
	struct sockaddr_in s1, s2;

	// initialize conx states
	struct ConxState conxs[MAX_CONXS];
	for(int i = 0; i < MAX_CONXS; i++)
		init_ConxState(&conxs[i],0);

	// create listener and bind to port
	listener = socket(AF_INET, SOCK_STREAM, 0);
	bzero((char*)&s1,sizeof(s1));
	s1.sin_family = (short) AF_INET;
	s1.sin_addr.s_addr = htonl(INADDR_ANY);
	s1.sin_port = htons(0);
	bind(listener,(struct sockaddr*) &s1, sizeof(s1));
	length = sizeof(s1);

	// print port
	getsockname(listener,(struct sockaddr*)&s1,(socklen_t*)&length);
	char port_string[22];
	sprintf(port_string,"Bound to port: %d\n",ntohs(s1.sin_port));
	write(1,port_string,sizeof(port_string));

	// listen for conx
	listen(listener,1);

	// begin select loop
	struct timeval tv; fd_set fds;

	// open log file
	int logfile_fd = open("logger.log", O_CREAT|O_RDWR|O_TRUNC, 0666);

	while(1){
		// populate timeout
		tv.tv_sec=5;tv.tv_usec=0;
		// populate fds
		FD_ZERO(&fds);
		FD_SET(listener,&fds);	// socket fd
		int max_fd = listener;	// init highest fd
		for(int i = 0; i < MAX_CONXS; i++) {
			// loop through conxs to add to fds
			if(conxs[i].state){
				FD_SET(conxs[i].fd,&fds);
				// if conx fd is higher than max, save it
				if(conxs[i].fd > max_fd)
					max_fd=conxs[i].fd;
			}
		}

		// select on fds
		int r_sel = select(max_fd+1,&fds,NULL,NULL,&tv);

		if(r_sel > 0){
			// check for conx on socket
			if(FD_ISSET(listener,&fds)){
				// accept conx
				length = sizeof(s2);
				conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);

				// write to conx
				char log_string[8];
				sprintf(log_string,"log #:\n");
				write(conn,log_string,sizeof(log_string));

				// save conx data to conx_state
				// i'm doing this with a loop so it can immediately reuse freed slots
				for(int i = 0; i < MAX_CONXS; i++)
					if(!conxs[i].state) {
						init_ConxState(&conxs[i],conn);
						break;
					}

			}
			// check for conx updates
			for(int i = 0; i < MAX_CONXS; i++){
				if(conxs[i].state && FD_ISSET(conxs[i].fd,&fds)) {
					int bytes_to_read;
					ioctl(conxs[i].fd,FIONREAD,&bytes_to_read);
					if(bytes_to_read>0) {
						// if bytes found, try to parse command 

						// read into buffer
						char readbuf[bytes_to_read];
						read(conxs[i].fd,&readbuf,bytes_to_read);

						if(conxs[i].state == CONX_STATE_CONNECTED){
							// commands for STATE_CONNECTED (not listing)

							// lots of nested conditions, so i'm using a flag to determine whether
							// the log command was found
							int cmd_log_found = 0;

							if(sizeof(readbuf)>=3){
								// only check for log if 3 or more characters exist, because
								// the way i'm taking a substring can segfault if too short

								// get substring: first three characters of input
								char cmd_buf[4];
								for(int j = 0; j < 3; j++)
									cmd_buf[j]=readbuf[j];
								cmd_buf[3]='\0'; // null-terminate
								if(strcmp(cmd_buf,"log")==0){
									// check substring for log command

									// flip flag
									cmd_log_found = 1;

									write(conxs[i].fd,"#log: logging\n",14);

									// get the substring for printing to logfile
									// i skip the first 4 characters: "log ", and the
									// last 1 char, which is always a newline 
									char after_buf[bytes_to_read-5];
									for(int j = 0; j < bytes_to_read-5; j++)
										after_buf[j]=readbuf[j+4];

									// replace last char, always a carriage return,
									// with a newline for separation
									after_buf[bytes_to_read-6]='\n';

									// seek to end, in case list moved file cursor
									// since last write
									lseek(logfile_fd,0,SEEK_END);
									// write to logfile
									write(logfile_fd,after_buf,bytes_to_read-5);
								}
							}
							if(!cmd_log_found) {
								// if log didn't run, check for other commands

								if(strcmp(readbuf,"list\r\n")==0){
									// socket conx always ends input with '\r\n'

									write(conxs[i].fd,"#log: listing\n",14);

									// update conx state
									conxs[i].state = CONX_STATE_LISTING;

									list_to_client(&conxs[i],logfile_fd);

								// ran out of standard commands — output command not recognized
								} else write(conxs[i].fd,"#log: Command not recognized\n",29);
							}
						} else if(conxs[i].state == CONX_STATE_LISTING){
							// commands for STATE_LISTING

							if(strcmp(readbuf,"\r\n")==0){
								// if only received newline, continue listing

								write(conxs[i].fd,"#log: more\n",11);

								list_to_client(&conxs[i],logfile_fd);

							// ran out of 'list' mode commands — output command not recognized
							} else write(conxs[i].fd,"#log: Command not recognized\n",29);

						}
					} else {
						//if no bytes to read, set state of conx to not-connected
						init_ConxState(&conxs[i],0);
					}
				}
			}
		}
	}
}
