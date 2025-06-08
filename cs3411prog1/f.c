#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F_first 1
#define F_last 2
#define F_data_int 3
#define F_data_char 4
#define F_data_float 5
#define F_print 6

void *f(int code, void *mem, void *data) {
    if (code == F_first) { // case for first call 
        // ensure supplied size is sensible
        if (mem != NULL || data == NULL) return NULL;
        int size = (int)(size_t)data;
        if (size <= 2) return NULL;

        // allocate memory to provided pointer, returning null if errored
        mem = malloc(size);
        if (!mem) return NULL;

        // initialize offset and return mem pointer
        *(short *)mem = 2;
        return mem;
    }

    if (!mem) return NULL;

    // get total and current offsets
    short *offset = (short *)mem;
    char *current = mem + *offset;

    // first 3 cases for data entry are quite similar
    if (code == F_data_int) {
        // ensure there's still enough room for an int
        if (*offset + 1 + sizeof(int) > (int)(size_t)data) return NULL;
        // add data marker for int
        *current++ = F_data_int;
        // write data and increment offsets
        memcpy(current, data, sizeof(int));
        current += sizeof(int);
        *offset += 1 + sizeof(int);
    } else if (code == F_data_char) {
        // ensure there's still enough room for a char (with +1 for null terminator)
        size_t len = strlen((char *)data) + 1;
        if (*offset + 1 + len > (int)(size_t)data) return NULL;
        // add data marker for char
        *current++ = F_data_char;
        // write data and increment offsets
        memcpy(current, data, len);
        current += len;
        *offset += 1 + len;
    } else if (code == F_data_float) {
        // ensure there's still enough room for a float
        if (*offset + 1 + sizeof(float) > (int)(size_t)data) return NULL;
        // add data marker for float
        *current++ = F_data_float;
        // write data and increment offsets
        memcpy(current, data, sizeof(float));
        current += sizeof(float);
        *offset += 1 + sizeof(float);
    // case for printing data
    } else if (code == F_print) {
        // set current to initial offset
        current = mem + 2;
        // iterate through bytes pointed to by mem
        while (current < mem + *offset) {
            // get current data marker
            char type = *current++;
            // printf subsequent data based on data marker
            if (type == F_data_int) {
                printf("%d", *(int *)current);
                current += sizeof(int);
            } else if (type == F_data_char) {
                printf("%s", current);
                current += strlen(current) + 1;
            } else if (type == F_data_float) {
                printf("%f", *(float *)current);
                current += sizeof(float);
            }
        }
    // case for final call
    } else if (code == F_last) {
        // free memory pointer. quite simple
        free(mem);
    } else {
        // return null if supplied data marker is invalid
        return NULL;
    }

    //return mem pointer
    return mem;
}
