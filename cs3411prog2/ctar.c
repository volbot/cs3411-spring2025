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

//Function I made to print error messages to stderr
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

int main(int argc, char *argv[])
{
    if(argc <= 2) {
        errprint(1, "Usage: ctar <-a|-d> <samplefilename.ctar> [file_1] ... \n");
        exit(1);
    }
    char* cmd = argv[1];
    char* filename = argv[2];
    if(strcmp(cmd,"-a")==0){ //APPEND/CREATE CASE
        struct stat filestatus;
        //read in filenames and check validity
        char *filenames[argc - 3];
        int filesizes[argc - 3];
        for(int i = 0; i < argc - 3; i++) {
            char *temp_filename = argv[i+3];
            //get information on file at hand
            if(lstat(temp_filename,&filestatus) != 0){
                errprint(5, "Failed to stat file '",temp_filename,"'. (",strerror(errno),")\n");
                exit(1);
            }
            //save information to use in later loops
            filenames[i] = argv[i+3];
            filesizes[i] = filestatus.st_size;
        }
        int appendmode = 1;
        //all files are valid, now check for target file
        if(lstat(filename,&filestatus) != 0){
            //lstat returned error, make sure it's only because file doesn't exist
            if(errno != ENOENT){
                errprint(5, "Failed to stat file '",filename,"'. (",strerror(errno),")\n");
                exit(1);
            } else {
                //file doesn't exist, so we disable appendmode
                appendmode = 0;
            }
        }

        //create or open ctar file
        int fd_ctar = open(filename, O_CREAT|O_RDWR, 0644);
        if(fd_ctar == -1){
            errprint(5, "Failed to open file '",filename,"'. (",strerror(errno),")\n");
            exit(1);
        }
        hdr head; //static pointer to first header, which we need so we can keep updating hdr.eop
        hdr * head2; //used as a moving pointer to whichever header we're currently adding to 
        ssize_t last_head_pos = 0; //for keeping track of where head2 should be written when
                                   //complete
        int file_index = 0; //for iterating through argument-supplied filenames
        if(appendmode == 1){
            //APPENDMODE==1, so the file already exists
            //read preexisting header
            ssize_t count = read(fd_ctar, &head, sizeof(hdr));
            if (count == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                exit(1);
            }
            head2 = &head;
            //read up to the final preexisting header
            while (head2->next != 0) {
                hdr temp;
                //jump to current header's hdr.next
                lseek(fd_ctar, head2->next, SEEK_SET);
                last_head_pos = head2->next;
                ssize_t count = read(fd_ctar, &temp, sizeof(hdr));
                //load current header into head2
                head2 = &temp;
                if (count == -1) {
                    errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                    close(fd_ctar);
                    exit(1);
                }
            }
            //jump to end of file
            lseek(fd_ctar, head.eop, SEEK_SET);
            //EDGE CASE: if the current header is full, we need to create a new empty one
            //   and point the last one to it
            if(head2->block_count==4){
                head2->next=head.eop;
                last_head_pos=head.eop;
                hdr temp = {
                    0x63746172,
                    0,
                    0,
                    {0,0,0,0},
                    {0,0,0,0},
                    {0,0,0,0},
                    0
                };
                head2 = &temp;
                lseek(fd_ctar,sizeof(hdr),SEEK_CUR);
            }
        } else {
            //APPENDMODE=0, so file doesn't already exist
            //create new first header
            hdr temp = {
                0x63746172,
                sizeof(hdr),
                0,
                {0,0,0,0},
                {0,0,0,0},
                {0,0,0,0},
                0
            };
            head = temp;
            //jump to end of header
            lseek(fd_ctar,sizeof(hdr),SEEK_SET);
            last_head_pos = 0;
            head2 = &head;
        }

        //loop through arg-supplied files
        while(file_index < sizeof(filenames)/sizeof(filenames[0])){ 
            char * temp_filename = filenames[file_index];
            //open file at hand
            int fd_temp = open(temp_filename,O_RDONLY);
            if(fd_temp == -1){
                errprint(5, "Failed to open file '",temp_filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                exit(1);
            }
            //save current fd offset to current header's hdr.file_name[i]
            //note: i'm using head2->block_count for these references, because file_index has 
            //  different meaning depending on appendmode
            head2->file_name[head2->block_count] = lseek(fd_ctar, 0, SEEK_CUR);
            short namelen = strlen(temp_filename);
            //WRITE FILENAME LENGTH
            ssize_t writecount = write(fd_ctar,&namelen,sizeof(short));
            if (writecount == -1) {
                errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                close(fd_temp);
                exit(1);
            }
            //WRITE FILENAME
            writecount = write(fd_ctar,temp_filename,namelen);
            if (writecount == -1) {
                errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                close(fd_temp);
                exit(1);
            }
            //create buffer to store read file data
            char * buf_temp = malloc(filesizes[file_index]);
            //read filedata into buffer
            ssize_t readcount = read(fd_temp,buf_temp,filesizes[file_index]);
            if (readcount == -1) {
                errprint(5, "Failed to read '",temp_filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                close(fd_temp);
                free(buf_temp);
                exit(1);
            }
            //write from buffer into tar file
            writecount = write(fd_ctar,buf_temp,readcount);
            if (writecount == -1) {
                errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                close(fd_temp);
                free(buf_temp);
                exit(1);
            }
            //clean up; we're done with this file
            free(buf_temp);
            close(fd_temp);
            //save current file's size to current header's hdr.file_size[i]
            head2->file_size[head2->block_count] = filesizes[file_index];
            //increment current header's hdr.block_count
            head2->block_count = head2->block_count + 1;
            //increment ORIGINAL header's eop
            head.eop = lseek(fd_ctar, 0, SEEK_CUR);
            //I hate this conditional. It works though, to test for either
            // A) end of current header AND more files left to tar, OR
            // B) end of argument-supplied files
            if((head2->block_count == 4 && file_index < argc - 3) || file_index == argc - 4) {
                //jump to where current header was born
                lseek(fd_ctar, last_head_pos, SEEK_SET);
                //if this header needs to point to another header, insert that position now, 
                //   or else it won't contain it on write. lost an hour or two on this one x_X
                if(file_index+1 < sizeof(filenames)/sizeof(filenames[0])){
                    head2->next=head.eop;
                }
                //write current header
                ssize_t count = write(fd_ctar,head2,sizeof(hdr));
                if (count == -1) {
                    errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
                    close(fd_ctar);
                    exit(1);
                }
                //if we need another header, make it now and update head2 to point to it
                if(file_index+1 < sizeof(filenames)/sizeof(filenames[0])){
                    lseek(fd_ctar, head.eop+sizeof(hdr), SEEK_SET);
                    last_head_pos = head.eop;
                    hdr temp = {
                        0x63746172,
                        0,
                        0,
                        {0,0,0,0},
                        {0,0,0,0},
                        {0,0,0,0},
                        0
                    };
                    head2 = &temp;
                }
            }
            file_index++;
        }
        //alldone writing files!
        //seek back to start and write first header with final hdr.eop
        lseek(fd_ctar, 0, SEEK_SET);
        ssize_t count = write(fd_ctar,&head,sizeof(hdr));
        if (count == -1) {
            errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
            close(fd_ctar);
            exit(1);
        }
        close(fd_ctar);

    } else if (strcmp(cmd,"-d") == 0) { //DELETE CASE
        if(argc != 4){
            errprint(1, "Usage: ctar -d <samplefilename.ctar> <file_to_delete> \n");
            exit(1);
        }
        struct stat filestatus;
        //check to make sure target ctar file exists
        if(lstat(filename,&filestatus) != 0){
            errprint(5, "Failed to stat file '",filename,"'. (",strerror(errno),")\n");
            exit(1);
        }
        //open ctar file
        int fd_ctar = open(filename, O_RDWR);
        if(fd_ctar == -1){
            errprint(5, "Failed to open file '",filename,"'. (",strerror(errno),")\n");
            exit(1);
        }

        hdr head;
        int complete = 0;
        //janky way to loop. but it works
        while(!complete){
            //save position of this header to return to, in case this one contains target file
            int head_pos = lseek(fd_ctar, 0, SEEK_CUR);
            //read in current header
            ssize_t count = read(fd_ctar, &head, sizeof(hdr));
            if (count == -1) {
                errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                close(fd_ctar);
                exit(1);
            }
            //loop up to hdr.block_count
            for(int i = 0; i < head.block_count; i++){
                //if no filename, or file already deleted, continue to next
                if(head.file_name[i]==0){
                    continue;
                }
                if(head.deleted[i]==1){
                    continue;
                }
                //jump to filename location
                lseek(fd_ctar,head.file_name[i],SEEK_SET);
                //GET FILENAME LENGTH
                short namelen;
                read(fd_ctar,&namelen,2);
                if (count == -1) {
                    errprint(5, "Failed to read '",filename,"'. (",strerror(errno),")\n");
                    close(fd_ctar);
                    exit(1);
                }
                //GET FILENAME
                char * temp_filename = malloc(namelen+1); 
                temp_filename[namelen]='\0';
                read(fd_ctar,temp_filename,namelen);
                if (count == -1) {
                    errprint(5, "Failed to read '",temp_filename,"'. (",strerror(errno),")\n");
                    close(fd_ctar);
                    free(temp_filename);
                    exit(1);
                }
                //check if this filename matches the supplied one
                if(strcmp(temp_filename,argv[3])==0){
                    //if so, flip bit in hdr.deleted[i]
                    head.deleted[i]=1;
                    //then, jump back to head_pos and write this header
                    lseek(fd_ctar, head_pos, SEEK_SET);
                    count = write(fd_ctar,&head,sizeof(hdr));
                    if (count == -1) {
                        errprint(5, "Failed to write to '",filename,"'. (",strerror(errno),")\n");
                        close(fd_ctar);
                        free(temp_filename);
                        exit(1);
                    }
                    //flip complete bit and break, so after freeing temp_filename, we leave loop
                    complete=1;
                    break;
                }
                free(temp_filename);
            }
            //if we still haven't found it, jump to next header if exists, or otherwise
            //  signal to end loop :(
            if(head.next!=0){
                lseek(fd_ctar, head.next, SEEK_SET);
            } else {
                complete = 1;
            }
        }
        close(fd_ctar);
    } else {
        errprint(1, "Usage: ctar <-a|-d> <samplefilename.ctar> [file_1] ... \n");
        exit(1);
    }
}
