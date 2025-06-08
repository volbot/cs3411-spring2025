#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
//Only used for sprintf!
#include <stdio.h>

typedef struct
{
    int magic; /* This must have the value 0x63746172. */
    int eop; /* End of file pointer used for checking file validity */
    int block_count; /* Number of entries in the block which are in-use. */
    int file_size[4]; /* File size in bytes for files 1..4 */
    char deleted[4]; /* Contains binary one at position i if i-th entry was deleted. */
    int file_name[4]; /* pointer to the name of the file. */
    int next; /* pointer to the next header block. */
} hdr;

void errprint(int argcount, ...) {
    char toprint[255] = "";
    char temp[255] = "";
    va_list args;
    va_start(args, argcount);
    for (int i = 0; i < argcount; i++) {
        sprintf(temp, "%s%s", toprint, va_arg(args, char *));
        sprintf(toprint, "%s", temp);
    }
    va_end(args);
    write(2,toprint,strlen(toprint));
}

int main(int argc, char *argv[]) {
    if (argc!=2){
        errprint(1, "Usage: utar <samplefilename.ctar>\n");
        exit(1);
    }

    struct stat filestatus;
    char* filename = argv[1];
    if(lstat(filename,&filestatus) != 0){
        errprint(5, "Failed to stat file '",filename,"'. (",strerror(errno),")\n");
        exit(1);
    }

    //create or open ctar file
    int fd_utar = open(filename, O_RDONLY);
    if(fd_utar == -1){
        errprint(5, "Failed to open file '",filename,"'. (",strerror(errno),")\n");
        exit(1);
    }

    //read header
    hdr head;
    ssize_t count = read(fd_utar, &head, sizeof(hdr));
    if (count == -1) {
        errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
        close(fd_utar);
        exit(1);
    }

    //check hdr.magic and hdr.eop to ensure validity of target file
    if(head.magic!=0x63746172 || head.eop!=filestatus.st_size){
        errprint(3, "File '",filename,"' is not a valid ctar.\n");
        close(fd_utar);
        exit(1);
    }

    int cont = 1;
    char * temp_filename;
    while(cont){
        for(int i = 0; i < 4; i++){
            if(head.file_name[i]==0){
                continue;
            }
            if(head.deleted[i]==1){
                continue;
            }
            lseek(fd_utar,head.file_name[i],SEEK_SET);
            //GET FILENAME
            short namelen;
            read(fd_utar,&namelen,2);
            if (count == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_utar);
                exit(1);
            }
            temp_filename = malloc(namelen+1); 
            count = read(fd_utar,temp_filename,namelen);
            temp_filename[namelen]='\0';
            if (count == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_utar);
                free(temp_filename);
                exit(1);
            }
            //END GET FILENAME
            //ENSURE FILE DOESN'T EXIST
            if(lstat(temp_filename,&filestatus)==0){
                errprint(3,"File ",temp_filename," already exists. Move away that file and restart.\n");
                close(fd_utar);
                free(temp_filename);
                exit(1);
            }
            if(errno != ENOENT){
                errprint(3,"File ",temp_filename," already exists. Move away that file and restart.\n");
                close(fd_utar);
                free(temp_filename);
                exit(1);
            }
            //END ENSURE FILE DOESN'T EXIST
            //GET DATA FROM CTAR FILE
            char * buf_temp = malloc(head.file_size[i]);
            ssize_t readcount = read(fd_utar, buf_temp, head.file_size[i]);
            if (readcount == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_utar);
                free(temp_filename);
                free(buf_temp);
                exit(1);
            }
            int fd_temp = open(temp_filename, O_CREAT|O_WRONLY, 0644);
            if(fd_temp == -1){
                close(fd_utar);
                free(temp_filename);
                free(buf_temp);
                exit(1);
            }
            //END GET DATA FROM CTAR FILE
            //WRITE DATA TO NEW FILE
            ssize_t writecount = write(fd_temp,buf_temp,readcount);
            if (writecount == -1) {
                errprint(5, "Failed to write to '",temp_filename,"'. (",strerror(errno),")\n");
                close(fd_utar);
                close(fd_temp);
                free(temp_filename);
                free(buf_temp);
                exit(1);
            }
            close(fd_temp);
            free(temp_filename);
            free(buf_temp);
            //END WRITE DATA TO NEW FILE
        }
        if(head.next==0){
            cont=0;
        } else {
            lseek(fd_utar,head.next,SEEK_SET);
            ssize_t count = read(fd_utar, &head, sizeof(hdr));
            if (count == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_utar);
                exit(1);
            }
        }
    }

    close(fd_utar);
}
