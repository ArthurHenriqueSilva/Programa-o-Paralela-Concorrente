**
 * Código para implementação de relógios vetoriais utilizando MPI
 * Meta: implementar a interação entre três processos ilustrada na figura
 * da URL:
 *
 * https://people.cs.rutgers.edu/~pxk/417/notes/images/clocks-vector.png
 *
 * Compilação: mpicc -g -Wall -o my_rvet my_rvet.c
 * Execução:   mpiexec -n 3 ./my_rvet
 */
 
#include <stdio.h>
#include <string.h>  
#include <mpi.h>    

#define CLOCK_SIZE 3

typedef struct Clock {
   int p[3];
} Clock;




void Event(int pid, Clock *clock){
   clock->p[pid]++;
   printf("Process: %d -> Clock: (%d,%d,%d)\n", pid, clock->p[0],clock->p[1],clock->p[2]);
}


void Send(int id_s, Clock *clock, int id_d){
   clock->p[id_s]++;
   
   int copy_clock[CLOCK_SIZE];
   for (int i = 0; i < CLOCK_SIZE; i++) copy_clock[i] = clock->p[i];
   
   MPI_Send(copy_clock, CLOCK_SIZE, MPI_INT, id_d, id_s, MPI_COMM_WORLD);
   printf("Process: %d -> Clock: (%d,%d,%d)\n", id_s, clock->p[0],clock->p[1],clock->p[2]);
   

}



void Receive(int id_r, Clock *clock, int id_s){
   clock->p[id_r]++;
   
   int clock_received[CLOCK_SIZE];
   
   MPI_Recv(clock_received, CLOCK_SIZE, MPI_INT, id_s, id_s, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   
   for (int i = 0; i < CLOCK_SIZE; i++){
      clock->p[i] = clock->p[i] < clock_received[i] ? clock->p[i] : clock_received[i];
   }
   
   printf("Process: %d -> Clock: (%d,%d,%d)\n", id_s, clock->p[0],clock->p[1],clock->p[2]);
}

// Representa o processo de rank 0
void process0(){
   Clock clock = {{0,0,0}};
   
   Event(0, &clock);
   
   Send(0, &clock, 1);
   
   Receive(0, &clock, 1);
   
   Send(0, &clock, 2);
   
   Receive(0, &clock, 2);
   
   Send(0, &clock, 1);
   
   Event(0, &clock);
}

// Representa o processo de rank 1
void process1(){
   Clock clock = {{0,0,0}};
   Send(1, &clock, 0);
   Receive(1, &clock, 0);
   Receive(1, &clock, 0);
}

// Representa o processo de rank 2
void process2(){
   Clock clock = {{0,0,0}};
   Event(2, &clock);
   Send(2, &clock, 0);
   Receive(2, &clock, 0);

}

int main(void) {
   int        comm_size;              
   int        my_rank;          

   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

   if (my_rank == 0) {
      process0();
   } else if (my_rank == 1) {  
      process1();
   } else if (my_rank == 2) {  
      process2();
   }

   /* Finaliza MPI */
   MPI_Finalize();

   return 0;
}  /* main */
