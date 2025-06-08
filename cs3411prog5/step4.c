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

//define conx state macros
#define CONX_STATE_NOT_CONNECTED 0
#define CONX_STATE_CONNECTED 1
#define CONX_STATE_LISTING 2

//struct for conx information
struct ConxState {
	int fd;
	int state;
	int pos;
};

void init_ConxState(struct ConxState* conx_state, int fd) {
	conx_state->fd = fd;
	conx_state->state = CONX_STATE_NOT_CONNECTED;
	conx_state->pos = 0;
}

//################
//## TCP SERVER ##
//################
int main(int argc, char *argv[])
{
	//define constants for port binding
	int listener, conn, length;
	struct sockaddr_in s1, s2;

	//initialize conx states
	struct ConxState conxs[32];

	//create listener and bind to port
	listener = socket(AF_INET, SOCK_STREAM, 0);
	bzero((char*)&s1,sizeof(s1));
	s1.sin_family = (short) AF_INET;
	s1.sin_addr.s_addr = htonl(INADDR_ANY);
	s1.sin_port = htons(0);
	bind(listener,(struct sockaddr*) &s1, sizeof(s1));
	length = sizeof(s1);

	//print port
	getsockname(listener,(struct sockaddr*)&s1,(socklen_t*)&length);
	char port_string[22];
	sprintf(port_string,"Bound to port: %d\n",ntohs(s1.sin_port));
	write(1,port_string,sizeof(port_string));

	//listen for conx
	listen(listener,1);

	//begin select loop
	struct timeval tv; fd_set fds;
	while(1){
		//populate timeout
		tv.tv_sec=1;tv.tv_usec=0;
		//populate fds
		FD_ZERO(&fds);
		FD_SET(listener,&fds);	//socket fd
		FD_SET(0,&fds);		//stdin fd

		//select on fds
		int r = select(listener+1,&fds,NULL,NULL,&tv);

		if(r > 0){
			//check for conx on socket
			if(FD_ISSET(listener,&fds)){
				//accept conx
				length = sizeof(s2);
				conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);
			}
			//check for stdin
			if(FD_ISSET(0,&fds)){
				//create properly sized buffer
				int bytes_to_read;
				ioctl(0,FIONREAD,&bytes_to_read);
				char writebuf[bytes_to_read];
				//read from stdin
				read(0,&writebuf,bytes_to_read);
				//write to stdout
				write(1,&writebuf,bytes_to_read);
			}
		}
	}
}
