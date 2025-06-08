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

//################
//## TCP SERVER ##
//################
int main(int argc, char *argv[])
{
	//define constants for port binding
	int listener, conn, length; char ch;
	struct sockaddr_in s1, s2;

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

	//accept conx
	length = sizeof(s2);
	conn = accept(listener, (struct sockaddr*) &s2, (socklen_t*) &length);

	//read characters from conx
	while (read(conn, &ch, 1) == 1) write(1,&ch,1);
	write(1,"\n",1);

}
