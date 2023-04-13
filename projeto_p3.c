/*
Compile: mpicc -g -Wall -o proj_p3 proj_p3.c -lpthread
Run: mpiexec -n 3 ./proj_p3
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>


#define THREAD_POOL_SIZE 3
#define PROCESS_POOL_SIZE 3
#define BUFFER_SIZE 9


// o processo é representado por uma "classe" com os atributos identificados pelo diagrama de integração proposto.
// Todo processo tem dois buffers, 1 mutex e 2 variáveis de condição pra cada buffer. 
// buffer 1 é o buffer de entrada que armazena os recebimentos do mpi e o buffer2 é o buffer de saida que armazena os clocks que serão enviados pelo mpi.
// Todo processo possui um id.

typedef struct Process {
    int clock[PROCESS_POOL_SIZE];
    int id;
    /*
    int **buffer1;
    int **buffer2;
    */
    int buffer1[BUFFER_SIZE];
    int buffer2[BUFFER_SIZE];
    int buffer1_pos;
    int buffer2_pos;
    pthread_mutex_t mutex_buffer1;
    pthread_mutex_t mutex_buffer2;
    pthread_cond_t full_buffer1;
    pthread_cond_t full_buffer2;
    pthread_cond_t empty_buffer1;
    pthread_cond_t empty_buffer2;
} Process;

// Inicialização da classe com a alocação da memória necessária para cada instância de processo
// Função para inicializar a classe.
// Recebe um id que vai de até PROCESS_POOL_SIZE - 1.
// Retorna a posição em que o processo foi alocado.
Process* Process_init(int my_id) {
    Process* process = (Process*) malloc(sizeof(Process));
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        process->clock[i] = 0;
    }
    // Inicializa as estruturas de paralelização e exclusão mútua.
    pthread_mutex_init(&process->mutex_buffer1, NULL);
    pthread_mutex_init(&process->mutex_buffer2, NULL);
    pthread_cond_init(&process->full_buffer1, NULL);
    pthread_cond_init(&process->full_buffer2, NULL);
    pthread_cond_init(&process->empty_buffer1, NULL);
    pthread_cond_init(&process->empty_buffer2, NULL);

    // Identifica o processo pelo id dado pelo parâmetro.
    process->id = my_id;

    // Alocação de memória para os buffers
    
    // process->buffer1 = (int **)malloc(BUFFER_SIZE * sizeof(int *));
    // process->buffer2 = (int **)malloc(BUFFER_SIZE*sizeof(int *));

    // Aloca a memória para os clocks dentro dos buffers
    //for(int i = 0; i < BUFFER_SIZE; i++){process->buffer1[i] = (int *)malloc(PROCESS_POOL_SIZE*sizeof(int));process->buffer2[i] = (int *)malloc(PROCESS_POOL_SIZE*sizeof(int))};


    return process;
}

// Função que finaliza o processo, matando as estruturas de memória alocadas pelo processo;
// Recebe a posição do processo na memória.
// Não possui retorno.
void Process_destroy(Process* process) {
    pthread_mutex_destroy(&process->mutex_buffer1);
    pthread_mutex_destroy(&process->mutex_buffer2);
    pthread_cond_destroy(&process->full_buffer1);
    pthread_cond_destroy(&process->full_buffer2);
    pthread_cond_destroy(&process->empty_buffer1);
    pthread_cond_destroy(&process->empty_buffer2);
    free(process->buffer1);
    free(process->buffer2);
    free(process);
    
}

// Função acessório para tornar visível a mudança de clock pelos processos.
// Recebe a posição do processo na memória.
// Não possui retorno.
void print_my_clock(Process* process){
    printf("Clock do processo %d: {", process->id);
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        printf("%d", process->clock[i]);
    }
    printf("}\n");
}

// Função que consulta o buffer, retira seu primeiro valor e atualiza esse buffer.
// Recebe a posição do processo na memória e um id que identifica qual buffer será alterado.
int* get_msg_from_buffer(Process* process, int id_buffer){
    int* msg = NULL;
    // Se o id_buffer == 1, então a msg será retirada do buffer de entrada do processo.
    if (id_buffer == 1){
        pthread_mutex_lock(&process->mutex_buffer1);
        while (process->buffer1_pos == 0){
            pthread_cond_wait(&process->empty_buffer1, &process->mutex_buffer1);
        }
        // Outra abordagem para remover o elemento do buffer
        // int *msg = process->buffer1[0];
        // process->buffer1_pos--;
        // free(process->buffer1[0]);
        // for(int i = 0; i < process->buffer1_pos; i++){
        //     process->buffer1[i] = process->buffer1[i+1];
        // }
        msg = &(process->buffer1[0]);
        for(int i = 0; i < process->buffer1_pos; i++){
            process->buffer1[i] = process->buffer1[i+1];
        }
        process->buffer1_pos--;
        pthread_cond_signal(&process->full_buffer1);
        pthread_mutex_unlock(&process->mutex_buffer1);
    }
    // Do contrário, o único caso possível é id_buffer == 2, que indica que a msg dev ser retirada do buffer2, ou de sáida.
    else{
        pthread_mutex_lock(&process->mutex_buffer2);
        while (process->buffer2_pos == 0){
            pthread_cond_wait(&process->empty_buffer2, &process->mutex_buffer2);
        }
        // Outra abordagem para remover o elemento do buffer
        // int *msg = process->buffer2[0];
        // process->buffer2_pos--;
        // free(process->buffer2[0]);
        // for(int i = 0; i < process->buffer2_pos; i++){
        //     process->buffer2[i] = process->buffer2[i+1];
        // }
        msg = &(process->buffer2[0]);
        for(int i = 0; i < process->buffer2_pos; i++){
            process->buffer2[i] = process->buffer2[i+1];
        }
        process->buffer2_pos--;
        pthread_cond_signal(&process->full_buffer2);
        pthread_mutex_unlock(&process->mutex_buffer2);
    }

    return msg;
}

// Introduz o clock do processo ou o recebido pelo mpi em um dos buffers.
// Recebe o endereço doo processo e um identificador para o buffer.
// Não possui retorno.
void put_msg_into_buffer(Process* process, int id_buffer){
    // Se o id_buffer == 1, então a msg será colocada no buffer de entrada do processo.
    if (id_buffer == 1){
        pthread_mutex_lock(&process->mutex_buffer1);
        while(process->buffer1_pos == BUFFER_SIZE){
            pthread_cond_wait(&process->full_buffer1, &process->mutex_buffer1);
        }
        process->buffer1[process->buffer1_pos] = process->clock;
        process->buffer1_pos++;
        pthread_cond_signal(&process->empty_buffer1);
        pthread_mutex_unlock(&process->mutex_buffer1);
    }
    // Se o id_buffer == 2, então a msg será colocada no buffer de saída do processo.
    else{
        pthread_mutex_lock(&process->mutex_buffer2);
        while(process->buffer2_pos == BUFFER_SIZE){
            pthread_cond_wait(&process->full_buffer2, &process->mutex_buffer2);
        }
        process->buffer2[process->buffer2_pos] = process->clock;
        process->buffer2_pos++;
        print_my_clock(process);
        pthread_cond_signal(&process->empty_buffer2);
        pthread_mutex_unlock(&process->mutex_buffer2);
    }
}

// Executa em loop o receive de msg mpi e chama a função que irá colocar a msg recebida no buffer de entrada.
// Recebe o endereço do processo.
// Não possui retorno.
void *receive_mpi_put_in_buffer(Process* process){
    while(1){
        int clock_received[PROCESS_POOL_SIZE];

        MPI_Recv(clock_received, PROCESS_POOL_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        put_msg_into_buffer(process, 1);
    }

}

// Envia o clock do processo ao buffer2 para que esse seja enviado por send mpi.
// Recebe o endereço do processo.
// Não possui retorno.
void put_my_clock_in_buffer(Process* process){

    int copy_clock[PROCESS_POOL_SIZE];

    for(int i = 0; i < PROCESS_POOL_SIZE; i++)copy_clock[i] =  process->clock[i];
    put_msg_into_buffer(process, 2);
}

// Atualiza o clock do processo com base na msg do buffer1 e envia o clock atualizado para o buffer2
// Recebe o endereço do processo.
// Não possui retorno.
void *update_my_clock(Process* process){
    while(1){
        int* msg = get_msg_from_buffer(process, 1);
        process->clock[process->id]++;
        
        for(int i = 0; i < PROCESS_POOL_SIZE; i++){
            process->clock[i] = process->clock[i] < msg[i] ? process->clock[i] : msg[i];
        } 
        put_my_clock_in_buffer(process);
    }

}

// Envia a msg do buffer2 pelo mpi send.
// Recebe o endereço do processo.
// Não possui retorno.
void *send_mpi(Process* process){
    while(1){
        int id_destino = process->id;
        while (id_destino == process->id){
            id_destino = rand() % PROCESS_POOL_SIZE;
        }

        pthread_mutex_lock(&process->mutex_buffer2);
        int* msg = get_msg_from_buffer(process, 2);
        pthread_mutex_unlock(&process->mutex_buffer2);

        MPI_Send(msg, PROCESS_POOL_SIZE, MPI_INT, id_destino, process->id, MPI_COMM_WORLD);
    }

}

// Linha de execução que cada processo irá seguir. Cada processo executará 3 threads em loop infinito.
// Recebe o endereço do processo.
// Não possui retorno.
void process_main(Process* process){
    pthread_t* threads = malloc (3*sizeof(pthread_t));
    if(pthread_create(&threads[0], NULL, receive_mpi_put_in_buffer, process)){
        perror("Failed to create processs inner thread");
    }

    if(pthread_create(&threads[1], NULL, update_my_clock, process)){
        perror("Failed to create processs inner thread");
    }

    if(pthread_create(&threads[2], NULL, send_mpi, process)){
        perror("Failed to create processs inner thread");
    }

    Process_destroy(process);

    }


// Cada processo é indicado em uma thread, além disso cada processo possui 3 threads internas indicando o funcionamento do processo. Uma thread recebe a msg mpi e coloca no buffer de entrada, a próxima thread atualiza o clock do processo e direciona esse mesmo clock ao buffer de saída. Por fim, a última thread deve escolher randômicamente um processo diferente de si mesmo para enviar o clock em forma de msg mpi.
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    pthread_t* process_list = malloc (PROCESS_POOL_SIZE*sizeof(pthread_t));
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        Process* process = Process_init(i);
        if(pthread_create(&process_list[i], NULL, process_main, Process* process) != 0){
            perror("Failed to create process.");
        }
    }

    // esse trecho não é executado já que os loops são eternos. Deve-se usar o ctrl-C para encerrar a execução
    for(int i = 0; i < PROCESS_POOL_SIZE; i++){
        if(pthread_join(process_list[i], NULL) != 0){
            perror("Failed to Join Process.");
        }
    }
    return 0;
}