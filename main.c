#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "io_info.h"

//+-----------------------------------------------------------------------+

io_info** initIO(int procs){
     io_info** local = (io_info**)malloc(procs * sizeof(io_info*));
     return local;
}

//+-----------------------------------------------------------------------+

// Функция для создания каналов в родительском процессе
void createPipes(io_info *io) {
    int success;

    success = pipe(io->fd1);
    if (success == -1) {
        fprintf(stderr, "fd1 pipe failed");
        exit(EXIT_FAILURE);
    }
    success = pipe(io->fd2);
    if (success == -1) {
        fprintf(stderr, "fd2 pipe failed");
        exit(EXIT_FAILURE);
    }
    success = pipe(io->fd0);
    if (success == -1) {
        fprintf(stderr, "fd0 pipe failed");
        exit(EXIT_FAILURE);
    }
}

int main(const int argc, const char *argv[]){

     if(argc <= 2 || strcmp(argv[1], "-p") != 0){
          printf("Prease, provide an argument!\n");
          return 1;
     }

     int i;
     int some_id, ret, status;
     Message msg_for_send, msg_for_recv;
     io_info io;

     FILE *hPipes = fopen(pipes_log, "w");
     if(!hPipes){
          printf("Couldn't create %s.\n", pipes_log);
          exit(EXIT_FAILURE);
     }
     FILE *hEvents = fopen(events_log, "a");
     if(!hEvents){
          printf("Couldn't create %s.\n", events_log);
          exit(EXIT_FAILURE);
     }

     int processes = atoi(argv[2]);
     i = 0;
     
     io.procs = processes;
     createPipes(&io); // Создаем каналы только в родительском процессе после форка


     //Информация обо всех открытых дескрипторах каналов (чтение / запись) в pipes.log
     fprintf(hPipes, "Pipes info:\n");
     fprintf(hPipes, "fd1: %d / %d\n", io.fd1[0], io.fd1[1]);
     fprintf(hPipes, "fd2: %d / %d\n", io.fd2[0], io.fd2[1]);
     fprintf(hPipes, "fd0: %d / %d\n", io.fd0[0], io.fd0[1]);
     fprintf(hPipes, "-----------------------------------------------------------------------------\n");

     fclose(hPipes);
     
     
     for(i = 0; i < processes; i++){
           some_id = fork();
            if(some_id < 0){
               printf("Couldn't create child process.\n");
               exit(EXIT_FAILURE);
            }
            else if(some_id == 0){
                printf("Inside Child process! \n My id is %d\n",getpid());
               
                break;
            }
            /* При запуске программы родительский процесс осуществляет необходимую
               подготовку для организации межпроцессного взаимодействия, после чего создает X
               идентичных дочерних процессов. Функция родительского процесса ограничивается
               созданием дочерних процессов и дальнейшим мониторингом их работы.*/
            else{
                printf("Inside Parent process!\n My id is %d\n", getpid());
                memset(&msg_for_recv, 0, sizeof(msg_for_recv));
                sleep(1);
                read(io.fd0[READ], &msg_for_recv, MAX_MESSAGE_LEN);
                fprintf(stdout, "Parent process got message: %s", msg_for_recv.s_payload);
            }

         }

         while ((ret = wait(&status)) > 0){
                    memset(&msg_for_recv, 0, sizeof(msg_for_recv));
                    sleep(1);
                    read(io.fd0[READ], &msg_for_recv, MAX_MESSAGE_LEN);
                    fprintf(stdout, "Parent process got message: %s", msg_for_recv.s_payload);
                    printf("After While My id is %d and my  child with pid = %d exiting with return value = %d\n" ,
                    getpid(),ret, WEXITSTATUS(status));
          }

/**********************************************************************************************************/

     if ((0 > ret) && (some_id == 0)){ 

          io.self = i;

          memset(&msg_for_send, 0, sizeof(Message));
          memset(&msg_for_recv, 0, sizeof(Message));

          msg_for_send.s_header.s_magic = MESSAGE_MAGIC;
          msg_for_send.s_header.s_payload_len = MAX_PAYLOAD_LEN;
          msg_for_send.s_header.s_type = STARTED;
          msg_for_send.s_header.s_local_time = 0;
          sprintf(msg_for_send.s_payload, log_started_fmt, i+1, getpid(), getppid());


          //процедура синхронизации со всеми остальными процессами в распределенной системе
          while(send_multicast(&io, (const Message*)&msg_for_send) != 0);
          while(receive_any(&io, &msg_for_recv) != 0);
          
          fprintf(stdout, "%s", msg_for_send.s_payload);
          fprintf(hEvents, "%s", msg_for_send.s_payload);

          //Все события логируются на терминал и в файл events.log
          sprintf(msg_for_send.s_payload, log_received_all_started_fmt, i+1);
          fprintf(stdout, "%s", msg_for_send.s_payload);
          fprintf(hEvents, "%s", msg_for_send.s_payload);

          //«полезная» работа дочернего процесса
          sprintf(msg_for_send.s_payload, log_done_fmt, i+1);
          fprintf(stdout, "%s", msg_for_send.s_payload);
          fprintf(hEvents, "%s", msg_for_send.s_payload);

          memset(&msg_for_send, 0, sizeof(Message));
          msg_for_send.s_header.s_magic = MESSAGE_MAGIC;
          msg_for_send.s_header.s_payload_len = MAX_PAYLOAD_LEN;
          msg_for_send.s_header.s_type = DONE;
          msg_for_send.s_header.s_local_time = 0;

          //процедура синхронизации процессов перед их завершением
          while(send_multicast(&io, (const Message*)&msg_for_send) != 0);
          while(receive_any(&io, &msg_for_recv) != 0);

          sprintf(msg_for_send.s_payload, log_received_all_done_fmt, i+1);
          fprintf(stdout, "%s", msg_for_send.s_payload);
          fprintf(hEvents, "%s", msg_for_send.s_payload);

          exit(i+1);
     }
/********************************************************************/
/*                    Родительский процесс                          */
/*     завершается при завершении всех остальных процессов          */
/********************************************************************/
     else{
          printf("Parent process exiting\n");  
          fclose(hEvents);
     }

     return EXIT_SUCCESS;
}
