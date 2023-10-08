#include "ipc.h"
#include "io_info.h"
#include <unistd.h>

//+------------------------------------------------------------------------------+

int send(void * self, local_id dst, const Message * msg){
    io_info* tmp = (io_info*)self;
	   
    switch(dst){
        case 0:
            close(tmp->fd1[READ]);
            //send the message to 2 process
            write(tmp->fd1[WRITE], msg, MAX_MESSAGE_LEN);
            sleep(1);
            //send the message to parent process
            write(tmp->fd0[WRITE], msg, MAX_MESSAGE_LEN);
            break;
        case 1:
            close(tmp->fd2[READ]);
            //send message to 1 process
            write(tmp->fd2[WRITE], msg, MAX_MESSAGE_LEN);
            sleep(1);
            //send the message to parent process
            write(tmp->fd0[WRITE], msg, MAX_MESSAGE_LEN);
            break;
        default:
            return -1;
        }

    return 0;
}

//+------------------------------------------------------------------------------+

int send_multicast(void * self, const Message * msg){
   io_info* tmp = (io_info*)self;

   for(unsigned char i = 0; i < tmp->procs; i++){
       if(i == tmp->self)
         continue;   
      send(self, i, msg);
   }

   return 0;
}

//+------------------------------------------------------------------------------+

int receive(void * self, local_id from, Message * msg){
    io_info* tmp = (io_info*)self;

    switch(from){
        case 0:
         close(tmp->fd2[WRITE]);
         //get message from 2 process
         read(tmp->fd2[READ], msg, MAX_MESSAGE_LEN);
         break;
        case 1:
         close(tmp->fd1[WRITE]);
         //get message from 1 process
         read(tmp->fd1[READ], msg, MAX_MESSAGE_LEN);
         break;
        default:
            return -1;
       }
      
       return 0;
}

//+------------------------------------------------------------------------------+

int receive_any(void * self, Message * msg){
    io_info* tmp = (io_info*)self;

    for (unsigned char i = 0; i < tmp->procs; i++){
        if(i == tmp->self)
            continue;
        receive(self, i, msg);

    }
    return 0;

}
