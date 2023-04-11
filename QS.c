/*
Make File commands
Compile: gcc -g -Wall -o QS  QS.c -lpthread
Usage: ./QS


Implementação de QuickSort Paralelizado utilizando a proposta de pool de threads para execução das tarefas.

Cada chamada recursiva da implementação trivial do QuickSort é traduzida como uma tarefa posta em fila para execução de acordo com disponibilidade das threads.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

// Definição do tamnaho do pool de threads e do limite de posições na fila de execução.
#define THREAD_NUM 4
#define BUFFER 64

// Variável de controle para início e fim das threads
int Endline = 1;


// Struct para carregar os parâmetros do quicksort para o pthread_create
typedef struct Data{
   int *arr;
   long int low;
   long int high;
}Data;

// Troca de valores entre dois elementos de um array
void swap(int* a, int*b){
   int t = *a;
   *a = *b;
   *b = t;
}

// Definição da posição de pivot
int partition(int arr[], int low, int high){
   int pivot = arr[high];
   int i = (low - 1);
   for (int j = low; j <= high -1; j++){
      if (arr[j] <= pivot){
         i++;
         swap(&arr[i], &arr[j]);
      }
   }
   swap(&arr[i+1], &arr[high]);
   return(i + 1);
}

// Declaração da função  qs_thread
void qs_thread(int *arr, int low,int high);

// Iniciação das ferramentas de controle de execução paralela
pthread_mutex_t mutex;
pthread_cond_t FULL;
pthread_cond_t EMPTY;

// Contador de tarefas em espera para execução
int task_count = 0;

// Fila de execução
Data dataQ[BUFFER];

// Função que chama o método qs_thread para as threads
void execute(Data* data, int id){
   qs_thread(data->arr, data->low, data->high);
}

// Função responsável por executar a tomada da task mais antiga da fila e atualizar o registro.
Data getTask(){
   pthread_mutex_lock(&mutex);

   while (task_count == 0){
      pthread_cond_wait(&EMPTY, &mutex);
   }

   Data task = dataQ[0];
   int i;
   for (i = 0; i < task_count - 1; i++){
      dataQ[i] = dataQ[i + 1];
   }
   
   task_count--;
   pthread_mutex_unlock(&mutex);
   pthread_cond_signal(&FULL);
   return task;
}

// Função responsável por enviar uma task à fila de execução.
void submitTask(Data task){
   pthread_mutex_lock(&mutex);
   while (task_count == BUFFER){
      pthread_cond_wait(&FULL, &mutex);
   }

   dataQ[task_count] = task;
   task_count++;

   
   pthread_mutex_unlock(&mutex);
   pthread_cond_signal(&EMPTY);

}

// Função que inicia as threads no contexto de execução com base nos parâmetros da fila.
void *startThread(void* args){
   int id = (int) args;
   while (Endline){
      Data task = getTask();
      execute(&task, id);
   }
   return NULL;
}

// Método para impressão do array em terminal
void printARR(int arr[], int size){
   for (int i = 0; i < size; i++){
      printf("%d ", arr[i]);
   }
   printf("\n");
}

// Implementação do método quicksort de modo que a chamada recursiva é posta como uma task para fila de execução
void qs_thread(int *arr, int low,int high){
   if (low < high){
      int piv = low + (high - low)/2;
      piv = partition(arr, low, high);

      Data t1 = {arr, low, piv - 1};
      Data t2 = {arr, piv + 1, high};
      submitTask(t1);
      submitTask(t2);
   }
}


// Função main que randomiza os valores do array e executa o pool de threads.
int main(){
   pthread_mutex_init(&mutex, NULL);
   pthread_cond_init(&EMPTY, NULL);
   pthread_cond_init(&FULL, NULL);
   pthread_t thread[THREAD_NUM];

   int i;
   int size = 15;
   int arr[size];
   int n = sizeof(arr)/sizeof(arr[0]);

   for (i = 0; i < size; i++){
      arr[i] = rand() + 1;
   }
   printARR(arr, n);

   for (i = 0; i < THREAD_NUM; i++){
      if (pthread_create(&thread[i], NULL, &startThread, (void *) i) != 0){
         perror("Failed to create thread");
      }
   }
   

   srand(time(NULL));
   
   // Chamada raiz para execução do pool de threads. A partir da primeira execução do qs_thread, são geradas as chamadas de task no pool. 
   qs_thread(arr, 0 , size+1); 
   printARR(arr, n);
   Endline = 0;
   
   for (i = THREAD_NUM; i > 0; i--){
      if (pthread_join(thread[i], NULL) != 0){
         perror("Failed to join thread");
      }
   }

   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&EMPTY);
   pthread_cond_destroy(&FULL);

   return 0;
}
