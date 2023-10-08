#define        READ      0
#define        WRITE     1

typedef struct _io_info_{
     int fd1[2], fd2[2], fd0[2];
     int procs; 
     int self;
}io_info;
