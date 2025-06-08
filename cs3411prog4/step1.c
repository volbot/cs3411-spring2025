#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{

	//get first arg
	char* SCR_name = argv[1];

	//assemble STDOUT file name
	size_t SCR_name_len = strlen(SCR_name);
	char SCR_STDOUT_name[SCR_name_len+7];
	strcpy(SCR_STDOUT_name,SCR_name);
	strcat(SCR_STDOUT_name,".stdout");

	//open STDOUT file
	int SCR_STDOUT_fd = open(SCR_STDOUT_name, O_CREAT|O_WRONLY, 0666);
	if(SCR_STDOUT_fd < 0) exit(1);

	//create write buffer
	char WRITEBUF[256];

	//write remaining args to STDOUT file
	ssize_t WRITE_COUNT = 0;
	for(int i = 2; i < argc; i++){
		//load arg into WRITEBUF
		size_t arg_len = strlen(argv[i]);
		strcpy(WRITEBUF,argv[i]);
		//write WRITEBUF into STDOUT file
		WRITE_COUNT = write(SCR_STDOUT_fd,WRITEBUF,arg_len);
		if(WRITE_COUNT!=arg_len) exit(1);
	}

	//close STDOUT file 
	int SYSCALL_RET = close(SCR_STDOUT_fd);
	if (!SYSCALL_RET) exit(0);
	else exit(1);
}
