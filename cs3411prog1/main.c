#include <unistd.h>
#include <stdio.h>

#define  F_first        1
#define  F_last         2
#define  F_data_int     3
#define  F_data_char    4
#define  F_data_float   5
#define  F_print        6

void * f (int code, void * mem, void * data);

int main()
{
    int     i_a;
    float   f_a;

    void  * m; 
    int   * ip;
    float * fp;
    
 
    fp = & f_a;
    ip = & i_a;

    m = f (F_first, 0, (void *)200); // main test case allocation         
    //m = f (F_first, 0, 0);         // test case allocation for 0 size. should return null, and the rest of calls should error
    
    // the rest is mostly provided test cases
    m = f (F_data_char, m,  (void *)"System programming class has ");

    f_a = 69.7;
    m = f (F_data_float,  m,  (void *)fp);
    m = f (F_data_char, m,  (void *)" registered");
    m = f (F_data_char, m,  (void *)" students in a");
    m = f (F_data_char, m,  (void *)" classroom of ");

    i_a = 70;
    m = f (F_data_int,  m,  (void *)ip);
    m = f (F_data_char, m,  (void *)"\n");

    // here I've added a few extra prints, to ensure that the data isn't freed until the F_last call
    m = f (F_print, m, 0);
    m = f (F_print, m, 0);
    m = f (F_print, m, 0);
    m = f (F_last, m, 0);
}
