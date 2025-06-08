#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//callback function supplied to qsort
//*a and *b are arrays of two ints, first being char and second being frequency
int comp(const int *a, const int *b)
{
	//take difference of frequencies
	int diff = b[1] - a[1];
	//if tie (no difference) take difference of char values
	if(!diff) diff = ((unsigned int)a[0]) - ((unsigned int)b[0]);
	return diff;
}

//bit counter keeps track of how many bits in bit buffer
static int bitcounter = 0;
//bit buffer stores bits to print, flushing and writing at 8 (one byte)
static unsigned char bitbuffer = 0;
//function to accept 1 bit and write it to the next unused bit in the buffer
void bit_commit(unsigned int bit)
{
	//set bit
	bitbuffer |= (bit << (7 - bitcounter));
	//increment buffer
	bitcounter++;
	//if bitcounter full, write and flush buffer+counter
	if(bitcounter == 8){
		write(1,&bitbuffer,1);
		bitcounter = 0;
		bitbuffer = 0;
	}
}
//function to flush remaining in bitbuffer and print up to next byte boundary (used when program complete)
void bit_flush()
{
	//if bitcounter != zero then there are committed but unwritten bits
	if(bitcounter){
		//white bitcounter != zero, commit zeroes
		while(bitcounter){
			bit_commit(0);
		}
	//otherwise we just need to print a byte of zeroes.
	//(unnecessary in practice but this is to match encoded output from provided binaries)
	} else {
		write(1,&bitbuffer,1);
	}

}
//shorthand function to commit a frequent character
void commit_freq_char(int i, int repeat){
	bit_commit(1);
	bit_commit(1);
	bit_commit((repeat >> 1) & 1);
	bit_commit((repeat >> 0) & 1);
	bit_commit((i >> 3) & 1);
	bit_commit((i >> 2) & 1);
	bit_commit((i >> 1) & 1);
	bit_commit((i >> 0) & 1);
}

//shorthand function to commit an infrequent character 
void commit_unfreq_char(unsigned char c){
	bit_commit(0);
	bit_commit((c >> 7) & 1);
	bit_commit((c >> 6) & 1);
	bit_commit((c >> 5) & 1);
	bit_commit((c >> 4) & 1);
	bit_commit((c >> 3) & 1);
	bit_commit((c >> 2) & 1);
	bit_commit((c >> 1) & 1);
	bit_commit((c >> 0) & 1);
}
//shorthand to commit a zero
void commit_zero(){
	bit_commit(1);
	bit_commit(0);
}

int main(void)
{
	size_t count = 1;
	unsigned char buf;
	int frequencies[256][2] = {0};
	//read in entire file, storing frequencies of characters
	while(count == 1){
		count = read(0,&buf,1);
		if(!count) break;
		if(!buf) continue;
		frequencies[buf][0]=buf;
		frequencies[buf][1]+=1;
	}
	unsigned char Dictionary[15];
	//sort frequencies[][] by frequency according to comparison function (line 8)
	qsort(frequencies,256,sizeof(frequencies[0]),(__compar_fn_t)comp);
	//loop through dictionary and write to encoded output
	for(int i = 0; i < 15; i++){
		if(frequencies[i][1]>0){
			Dictionary[i] = frequencies[i][0];
		} else {
			Dictionary[i] = 0;
		}
		buf=Dictionary[i];
		write(1,&buf,1);
	}
	//return to start of stdin file
	lseek(0,0,SEEK_SET);
	count=1;
	int written = 0;
	while(count == 1){
		//read character
		count = read(0,&buf,1);
		//if nothing is read then EOF reached, break loop
		if(count != 1){
			break;
		}
		written = 0;
		//if buf empty, then zero bit was read. commit zero
		if(buf == 0){
			commit_zero();
			written=1;
			continue;
		}
		//if nothing yet written, check if read character is frequent
		if(!written){
			for(int i = 1; i < 16; i++){
				//if found, check for up to 3 more of that character
				if(Dictionary[i-1]!=0 && buf==Dictionary[i-1]){
					int repeat = 0;
					unsigned char buf2 = buf;
					while(repeat < 3) {
						count = read(0,&buf2,1);
						//if nothing read then EOF reached, break this loop
						if(!count) break;
						if(buf2 == buf){
							repeat++;
						} else {
							//if char is NOT a repeat, then seek back
							//   one char so it can be reread and
							//   committed at next loop iter
							lseek(0,-1,SEEK_CUR);
							break;
						}
					}
					//commit freq character
					commit_freq_char(i,repeat);
					written = 1;
					break;
				}
			}
		}
		//if nothing yet written, commit character as unfreq
		if(!written){
			commit_unfreq_char(buf);
		}
	}
	//loop complete — commit 11000000 (EOF indicator, but also freq char zero)
	commit_freq_char(0,0);
	//flush bits if necessary
	bit_flush();

}
