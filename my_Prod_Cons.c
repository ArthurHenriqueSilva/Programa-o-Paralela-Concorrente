/*
Usage
    Compile: gcc -g -Wall -o my_P_C  my_Prod_Cons.c -lpthread
    Run: ./my_P_C



*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

// Tamanho do pool de threads. Um pool para consumidor, outro para produtor.
#define POOL_SIZE 3
// Tamanho do buffer
#define BUFFER_SIZE 9
// Número de clocks que devem ser criados
#define LIMIT_PRODUCER 10

// Estrutura de clock adotada. Um clock de 3 posições. Os valores são randômicos.
typedef struct Clock{
    int data[3];
}Clock;

// Criado o buffer ou a fila de clocks a serem mostrados em tela.
Clock buffer[BUFFER_SIZE];

// Representa quanto do buffer está em uso
int Clock_count = 0;


// Delcaração das estruturas de paralelização
pthread_mutex_t mutex;
pthread_cond_t condFull;
pthread_cond_t CondEmpty;

// Função utilziada para debug e anáise de continuidade do BUFFER.
void PrintBuffer(){

    printf("Buffer atual: [ ");
    for (int i = 0; i < BUFFER_SIZE; i++){
        printf("(%d, %d, %d),", buffer[i].data[0], buffer[i].data[1], buffer[i].data[2]);
    }
    printf("]\n");
}

// Função final das threads consumidoras. Recebe o id da thread produtora que criou aquele clock, além de printar o próprio clock.
void Execute(Clock* c, int id){

    printf("Consumer Thread %d got this random clock from BUFFER: [%d, %d, %d].\n", id, c -> data[0], c -> data[1], c -> data[2]);
}


// Pega o clock do buffer e atualiza esse.
Clock getClock(){

    pthread_mutex_lock(&mutex);

    while (Clock_count == 0){
        pthread_cond_wait(&CondEmpty, &mutex);
    }

    Clock clock = buffer[0];

    for (int i = 0; i < Clock_count - 1; i++){
        buffer[i] = buffer[i + 1];
    }

    Clock_count--;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condFull);
    return clock;
}

// Função usada para enviar clocks ao buffer.
void submitClock(Clock c){
    pthread_mutex_lock(&mutex);
    while (Clock_count == BUFFER_SIZE){
        pthread_cond_wait(&condFull, &mutex);
    }

    buffer[Clock_count] = c;
    Clock_count++;
    pthread_cond_signal(&CondEmpty);
    pthread_mutex_unlock(&mutex);

}

// Função auxiliar a start_Producer_thread capaz de gerar os valores aleatórios de clock.
Clock createClock(int id){

    struct Clock thisClock;

    for (int i = 0; i < 3; i++){
        thisClock.data[i] = (rand() % 7 * rand() % 13) + id;
    }
    return thisClock;
}

// Mantém as threads consumidoras em ação até que o counter estoure o limite.
void *start_Consumer_Thread(void * args){
    int id = (int) args;
    while (1){
        Clock clock = getClock();
        Execute(&clock, id);
        sleep(2);
    }
    return NULL;
}

// Mantém as threads produtoras ativas até que se alcance o limite de clocks gerados(LIMIT_PRODUCER). A função gera a chamada para a função cerateClock que gera os valores randomicos para os clocks.
void * start_Producer_Thread(void * args){
    int id = (int) args;
    int counter = 0;

    while(counter <= LIMIT_PRODUCER){
        Clock c = createClock(id);
        submitClock(c);
        counter++;
        sleep(2);
    }
    printf("Producer Thread %d made %d clocks randomly and Finshed its work succesfully.\n", id, counter-1);
    return NULL;
}

// A função main inicializa as estruturas de paralização e o cenário produtor-consumidor.
int main(int argc, char* argv[]){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&CondEmpty, NULL);
    pthread_cond_init(&condFull, NULL);

    pthread_t producers[POOL_SIZE];
    for (int i = 0; i < POOL_SIZE; i++){
        if(pthread_create(&producers[i], NULL,start_Producer_Thread, (void*) i) != 0){
            perror("Failed to create Producer thread.");
        }
    }


    pthread_t consumers[POOL_SIZE];
    for (int i = 0.; i < POOL_SIZE; i++){
        if (pthread_create(&consumers[i], NULL, *start_Consumer_Thread, (void*) i) != 0){
            perror("Failed to create consumer Thread.");
        }
    }


    for (int i = 0; i < POOL_SIZE; i++){
        if (pthread_join(producers[i], NULL) != 0){
            perror("Failed to Join a producer Thread.");
        }
        
    }
    printf("Press ctrl + C to stop execution!");
    
        for (int i = 0; i < POOL_SIZE; i++){
        if (pthread_join(consumers[i], NULL) != 0){

            perror("Failed to Join a consumer Thread.");
        } 
    }
    return 0;
}
