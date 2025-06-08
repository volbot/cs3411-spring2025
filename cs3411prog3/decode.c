#include <unistd.h>
#include <stdio.h>
#include <string.h>

//bitcounter keeps track of read bits
static int bitcounter=0;
//bitbuffer stores last read byte for individual bit retrieval
static unsigned char bitbuffer=0;
//function to get one bit at a time form file
int read_bit(){
	//if no bits are stored, then read a byte
	if(!bitcounter){
		bitcounter=8;
		int count = read(0,&bitbuffer,1);
		//if nothing could be read, EOF reached — return -1
		if(count != 1) return -1;
	}
	//decrement bitcounter for next iter
	bitcounter--;
	//return most significant bit yet unreturned
	return (bitbuffer >> bitcounter) & 1;
}

//function to read multiple bits at a time
unsigned char read_bits(int count){
	unsigned char ret = 0;
	//loop up to count, reading bits and assembling into a char buffer
	for(int i = 0; i < count; i++){
		int bit = read_bit();
		if(bit<0) break;
		ret |= (bit << ((count - 1) - i));
	}
	return ret;
}

int main(void)
{
	//init Dictionary to zero
	unsigned char Dictionary[15] = {0};
	//read in Dictionary values from encoded file
	for(int i = 0; i < 15; i++){
		read(0,&Dictionary[i],1);
	}
	int bit = 0;
	while(bit >= 0){
		//read bit to determine first course of action
		bit = read_bit();
		//if bit negative, EOF reached; exit. (this should never happen)
		if(bit == -1){
			break;
		}
		//if bit == 0, read next 8 bits as unfreq character and write to file
		if(!bit){
			char byte = read_bits(8);
			write(1,&byte,1);
		//if bit != 0 then bit == 1; read next bit to specify course of action
		} else {
			bit = read_bit();
			//if bit == 0, then write a zero byte and continue
			if(!bit) {
				char byte = 0;
				write(1,&byte,1);
			//if bit != 0, then bit == 1; read next 6 bits as freq character
			} else {
				//read repeat (2bit)
				int repeat = read_bits(2);
				//read Dictionary index (4bit)
				int dict_dex = read_bits(4);
				//if both are zero, then EOF reached. exit
				if(!repeat){
					if(!dict_dex) return 0;
				}
				//increment repeat and decrement dict_dex to get addressable values
				repeat++;
				dict_dex--;
				char towrite[repeat+1];
				//fill write buffer with freq char
				for(int i = 0; i < repeat; i++){
					towrite[i]=Dictionary[dict_dex];
				}
				//write to file
				write(1,&towrite,repeat);
				memset(towrite,0,repeat+1);
			}
		}
	}
}
