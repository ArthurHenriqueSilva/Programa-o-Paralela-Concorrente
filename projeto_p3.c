/*
Compile: mpicc -g -Wall -o proj_p3 proj_p3.c -lpthread
Run: mpiexec -n 3 ./proj_p3
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include<string.h>


#define THREAD_POOL_SIZE 3
#define PROCESS_POOL_SIZE 3
#define BUFFER_SIZE 9


    
int relogio[PROCESS_POOL_SIZE];
int buffer1[BUFFER_SIZE][PROCESS_POOL_SIZE];
int buffer2[BUFFER_SIZE][PROCESS_POOL_SIZE];
pthread_mutex_t mutex_buffer1;
pthread_mutex_t mutex_buffer2;
int buffer1_pos = 0;
int buffer2_pos = 0;
pthread_cond_t full_buffer1;
pthread_cond_t full_buffer2;
pthread_cond_t empty_buffer1;
pthread_cond_t empty_buffer2;


// Receber mensagem mpi e colocar msg no buffer1
void *thread_1(void* args){
    int relogio_recebido[PROCESS_POOL_SIZE];
    // como declarar o array recebido ?
    MPI_Recv(relogio_recebido, PROCESS_POOL_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    pthread_mutex_lock(&mutex_buffer1);
    while(buffer1_pos == BUFFER_SIZE){
        pthread_cond_wait(&full_buffer1, &mutex_buffer1);
    }
    // Colocando relogio na ultima posicao do buffer
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        buffer1[buffer1_pos][i] = relogio[i];
    }
    pthread_mutex_unlock(&mutex_buffer1);
    pthread_cond_signal(&empty_buffer1);
    return NULL;
}

// Retirar clock do buffer1, atualizar clock do processo e enviar para buffer2
void *thread_2(void* args){
    int relogio_a_incorporar[PROCESS_POOL_SIZE];
    
    pthread_mutex_lock(&mutex_buffer1);
    while(buffer1_pos == 0){
        pthread_cond_wait(&empty_buffer1, &mutex_buffer1);
    }
    
    // Copiando o primeiro clock do buffer para a variável
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        relogio_a_incorporar[i] = buffer1[0][i];
    }
    
    // Atualizando buffer1
    for(int i = 1; i < buffer1_pos; i++){
        for(int j = 0; j < PROCESS_POOL_SIZE; j++){
            buffer1[i-1][j] = buffer1[i][j];
        }
    }
    buffer1_pos--;
    pthread_mutex_unlock(&mutex_buffer1);
    pthread_cond_signal(&full_buffer1);

    
    
    // Atualizar clock do processo
    relogio[my_rank]++;
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        relogio[i] = relogio[i] < relogio_a_incorporar[i] ? relogio[i] : relogio_a_incorporar[i];
    }
    printf("Clock do processo %d: {", my_rank);
    for (int i = 0; i < PROCESS_POOL_SIZE; i++){
        if (i == PROCESS_POOL_SIZE - 1){
            printf("%d", relogio[i]);
        }
        else{
            printf("%d, ", relogio[i]);
        }
    }
    printf("}");
    return NULL;
}

// Retirar clock do buffer2 e enviar pelo mpi
void *thread_3(void* args){
    int relogio_a_enviar[PROCESS_POOL_SIZE];
    pthread_mutex_lock(&mutex_buffer2);
    while(buffer2_pos == BUFFER_SIZE){
        pthread_cond_wait(&full_buffer2, &mutex_buffer2);
    }
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        buffer2[buffer2_pos][i] = relogio_a_enviar[i];
    }
    pthread_mutex_unlock(&mutex_buffer2);
    pthread_cond_signal(&empty_buffer2);
    
    int destino = rand() % PROCESS_POOL_SIZE;
    MPI_Send(relogio_a_enviar, PROCESS_POOL_SIZE, MPI_INT, destino, my_rank, MPI_COMM_WORLD);
    return NULL;
}

// Cada processo é indicado em uma instância de mpi. Além disso,cada processo possui 3 threads internas indicando o funcionamento do processo. Uma thread recebe a msg mpi e coloca no buffer de entrada, a próxima thread atualiza o clock do processo e direciona esse mesmo clock ao buffer de saída. Por fim, a última thread deve escolher randômicamente um processo diferente de si mesmo para enviar o clock em forma de msg mpi.
int main(void) {
    int comm_size;               
    int my_rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Status status;
    
    printf("Process %d começou!\n", my_rank);
    
    // Criação do relogio do processo
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        relogio[i] = 0;
    }
    
    
    // ideia de buffer = array de arrays = [clock1, clock2, clocn]
    // cada clock é um array = clock1 = [x,y,z]
    


    pthread_t thread[PROCESS_POOL_SIZE];
    long i = 0;
    if(pthread_create(&thread[0], NULL, &thread_1, (void*) i) != 0){
        perror("Failed to create thread.");
    }
    if(pthread_create(&thread[1], NULL, &thread_2, (void*) i) != 0){
        perror("Failed to create thread.");
    }
    if(pthread_create(&thread[2], NULL, &thread_3, (void*) i) != 0){
        perror("Failed to create thread.");
    }
    
    
    if(pthread_join(thread[0], NULL) != 0){
        perror("Failed to create thread.");
    }
    if(pthread_join(thread[1], NULL) != 0){
        perror("Failed to create thread.");
    }
    if(pthread_join(thread[2], NULL) != 0){
        perror("Failed to create thread.");
    }
    MPI_Finalize();
    pthread_mutex_destroy(&mutex_buffer1);
    pthread_cond_destroy(&empty_buffer1);
    pthread_cond_destroy(&full_buffer1);
    
    pthread_mutex_destroy(&mutex_buffer2);
    pthread_cond_destroy(&empty_buffer2);
    pthread_cond_destroy(&full_buffer2);
    return 0;
}
