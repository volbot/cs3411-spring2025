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

//define conx state macros + some others
#define CONX_STATE_NOT_CONNECTED 0
#define CONX_STATE_CONNECTED 1
#define CONX_STATE_LISTING 2

#define MAX_CONXS 32

//struct for conx information
struct ConxState {
	int fd;
	int state;
	int pos;
};

void init_ConxState(struct ConxState* conx_state, int fd) {
	if(fd){
		conx_state->fd = fd;
		conx_state->state = CONX_STATE_CONNECTED;
	} else { //if no FD supplied, zero out
		conx_state->fd = 0;
		conx_state->state = CONX_STATE_NOT_CONNECTED;
	}
	conx_state->pos = 0;
}

void taper_readwrite(int fd, int bytes_to_read) {
	//read from fd and write to stdout
	int _bytes_to_read = bytes_to_read;
	char writebuf[256];
	while(_bytes_to_read > 0){
		int nread = read(fd,&writebuf,256);
		write(1,&writebuf,nread);
		_bytes_to_read -= nread;
	}
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
	struct ConxState conxs[MAX_CONXS];
	for(int i = 0; i < MAX_CONXS; i++)
		init_ConxState(&conxs[i],0);

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
		int max_fd = listener; //init highest fd
				       //loop through conxs to add to fds
		for(int i = 0; i < MAX_CONXS; i++)
			if(conxs[i].state == CONX_STATE_CONNECTED){
				FD_SET(conxs[i].fd,&fds);
				//if conx fd is higher than max, save it
				if(conxs[i].fd > max_fd)
					max_fd=conxs[i].fd;
			}

		//select on fds
		int r_sel = select(max_fd+1,&fds,NULL,NULL,&tv);

		if(r_sel > 0){
			//check for conx on socket
			if(FD_ISSET(listener,&fds)){
				//accept conx
				length = sizeof(s2);
				conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);

				//write to conx
				char log_string[8];
				sprintf(log_string,"log #:\n");
				write(conn,log_string,sizeof(log_string));

				//save conx data to conx_state
				//i'm doing this with a loop so that it can reuse freed slots
				for(int i = 0; i < MAX_CONXS; i++)
					if(!conxs[i].state) {
						init_ConxState(&conxs[i],conn);
						//printf("conx %d opened\n",i);
						break;
					}

			}
			//check for stdin
			if(FD_ISSET(0,&fds)){
				//create properly sized buffer
				int bytes_to_read;
				ioctl(0,FIONREAD,&bytes_to_read);
				taper_readwrite(0,bytes_to_read);
			}
			//check for conx updates
			for(int i = 0; i < MAX_CONXS; i++){
				if(conxs[i].state == CONX_STATE_CONNECTED 
						&& FD_ISSET(conxs[i].fd,&fds)
				  ) {
					int bytes_to_read;
					ioctl(conxs[i].fd,FIONREAD,&bytes_to_read);
					//if bytes found, print to stdout
					if(bytes_to_read>0) {
						taper_readwrite(conxs[i].fd,bytes_to_read);
					}
					//if no bytes found, set state to disconnected
					else {
						init_ConxState(&conxs[i],0);
						//printf("conx %d closed\n",i);
					}
				}
			}
		}
	}
}
