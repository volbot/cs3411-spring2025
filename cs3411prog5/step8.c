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

#define MAX_CONXS FD_SETSIZE

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

//################
//## TCP SERVER ##
//################
int main(int argc, char *argv[])
{
	//define constants for port binding
	int listener, conn, length; char ch;
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
			if(conxs[i].state){
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
			//check for conx updates
			for(int i = 0; i < MAX_CONXS; i++){
				if(conxs[i].state && FD_ISSET(conxs[i].fd,&fds)) {
					int bytes_to_read;
					ioctl(conxs[i].fd,FIONREAD,&bytes_to_read);
					//if bytes found, try to parse command 
					if(bytes_to_read>0) {
						char readbuf[bytes_to_read];
						read(conxs[i].fd,&readbuf,bytes_to_read);
						if(conxs[i].state == CONX_STATE_CONNECTED){
							if(strcmp(readbuf,"log\r\n")==0){
								write(conxs[i].fd,"#log: logging\n",14);
							} else if(strcmp(readbuf,"list\r\n")==0){
								write(conxs[i].fd,"#log: listing\n",14);
								conxs[i].state = CONX_STATE_LISTING;
							} else {
								write(conxs[i].fd,"#log: Command not recognized\n",29);
							}
						} else if(conxs[i].state == CONX_STATE_LISTING){
							if(strcmp(readbuf,"\r\n")==0){
								write(conxs[i].fd,"#log: more\n",11);
							} else {
								write(conxs[i].fd,"#log: Command not recognized\n",29);
							}

						}
					} else {
						init_ConxState(&conxs[i],0);
						//printf("conx %d closed\n",i);
					}
				}
			}
		}
	}
}
