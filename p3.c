/*
sem MPI
Compile:  gcc -g -Wall -o p3 projeto3_v3.c -lpthread 
run: ./exec

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_PROCESS 3
#define BUFFER_SIZE 9


/*
A estrutura de buffer será uma lista encadeada. Cada nó do buffer carrega a mensagem, no caso os values, que representa os clocks na fila. Além disso, cada nó possui um campo para index do determinado nó e um ponteiro para o próximo nó.

O buffer2 possui um campo a mais que indica o destino daquele clock enviado.

*/

struct buffer1 {
    int* values;
    int index;
    struct buffer1* next;
};
struct buffer1* head_b1 = NULL;
pthread_mutex_t mutex1;
pthread_cond_t full_b1;
pthread_cond_t empty_b1;

void addNode_b1(int* values) {
    struct buffer1* new_node = (struct buffer1*) malloc(sizeof(struct buffer1));
    new_node->values = (int*) malloc(NUM_PROCESS * sizeof(int));
    for (int i = 0; i < NUM_PROCESS; i++) {
        new_node->values[i] = values[i];
    }
    new_node->next = NULL;
    pthread_mutex_lock(&mutex1);
    if (head_b1 == NULL) {
        head_b1 = new_node;
        new_node->index = 0;
    } 
    else{
        struct buffer1* current = head_b1;
        while (current->next != NULL) {
            current = current->next;
        }
        while(current->index == BUFFER_SIZE){
            pthread_cond_wait(&full_b1, &mutex1);
        }
        current->next = new_node;
        new_node->index = current->index+1;
    }
    pthread_cond_signal(&empty_b1);
    pthread_mutex_unlock(&mutex1);
}


struct buffer1* removed_node_b1 = NULL;
void removeNode_b1() {
    pthread_mutex_lock(&mutex1);
    while(head_b1 == NULL) {
        printf("Aguardando dados no buffer1");
        pthread_cond_wait(&empty_b1, &mutex1);
    }

    struct buffer1* temp = head_b1;
    head_b1 = head_b1->next;
    removed_node_b1 = temp;
    free(temp->values);
    free(temp);

    struct buffer1* current = head_b1;
    while(current != NULL){
        current->index--;
        current = current->next;
    }

    printf("head_b1 removido\n\n");
    pthread_cond_signal(&full_b1);
    pthread_mutex_unlock(&mutex1);
}

void printList_b1() {
    pthread_mutex_lock(&mutex1);
    if (head_b1 == NULL) {
        printf("Lista 1 vazia\n\n");
        pthread_mutex_unlock(&mutex1);
        return;
    }

    struct buffer1* current = head_b1;
    while (current != NULL) {
        printf("[ ");
        for (int i = 0; i <  NUM_PROCESS; i++) {
            printf("%d ", current->values[i]);
        }
        printf("]\n\n");
        current = current->next;
    }
    pthread_mutex_unlock(&mutex1);
}


struct buffer2{
    int* values;
    int index;
    int destino;
    struct buffer2* next;
};

struct buffer2* head_b2 = NULL;
pthread_mutex_t mutex2;
pthread_cond_t full_b2;
pthread_cond_t empty_b2;

void addNode_b2(int* values, int destino){
    struct buffer2* new_node = (struct buffer2*) malloc(sizeof(struct buffer2));
    new_node->values = (int *) malloc(NUM_PROCESS * sizeof(int));
    for(int i = 0; i < NUM_PROCESS; i++){
        new_node->values[i] = values[i];
    }
    new_node->next = NULL;
    new_node->destino = destino;
    pthread_mutex_lock(&mutex2);
    if(head_b2 == NULL){
        head_b2 = new_node;
        new_node->index = 0;  
    }else{
        struct buffer2* current = head_b2;
        while(current->next != NULL){
            current = current->next;
        }
        while(current->index == BUFFER_SIZE){
            pthread_cond_wait(&full_b2, &mutex2);
        }
        current->next = new_node;
        new_node->index = current->index + 1;
    }
    pthread_cond_signal(&empty_b2);
    pthread_mutex_unlock(&mutex2);
}

struct buffer2* removed_node_b2 = NULL;
void removeNode_b2(){
    pthread_mutex_lock(&mutex2);
    while(head_b2 == NULL) {
        printf("Aguardando dados no buffer2");
        pthread_cond_wait(&empty_b2, &mutex2);
    }
    struct buffer2* temp = head_b2;
    head_b2 = head_b2->next;
    removed_node_b2 = temp;
    free(temp->values);
    free(temp);


    struct buffer2* current = head_b2;
    while(current != NULL){
        current->index--;
        current = current->next;
    }

    printf("Head_b2 removido\n\n");
    pthread_cond_signal(&full_b2);
    pthread_mutex_unlock(&mutex2);
}

void printList_b2(){

    pthread_mutex_lock(&mutex2);
    if(head_b2 == NULL){
        printf("Lista 2 vazia\n\n");
        pthread_mutex_unlock(&mutex2);
        return;
    }

    struct buffer2* current = head_b2;
    while(current != NULL){
        printf("[ ");
        for(int i = 0; i < NUM_PROCESS; i++){
            printf("%d ", current->values[i]);
        }
        printf("]\n\n");
        current = current->next;
    }
    pthread_mutex_unlock(&mutex2);
}


int main() {
    pthread_mutex_init(&mutex1, NULL);
    pthread_cond_init(&full_b1, NULL);
    pthread_cond_init(&empty_b1, NULL);

    int values1[] = {1, 2, 3};
    int values2[] = {4, 5, 6};
    int values3[] = {7, 8,9};
    int values4[] = {10,11,12};

    addNode_b1(values1);
    addNode_b1(values2);
    addNode_b1(values3);
    addNode_b1(values4);

    printList_b1();

    removeNode_b1();

    printList_b1();

    removeNode_b1();

    printList_b1();

    removeNode_b1();

    printList_b1();

    removeNode_b1();

    printList_b1();

    // removeNode_b1();


    pthread_mutex_init(&mutex2, NULL);
    pthread_cond_init(&full_b2, NULL);
    pthread_cond_init(&empty_b2, NULL);
    int values5[] = {9, 8, 7};
    int values6[] = {6, 5, 4};
    int values7[] = {3, 2, 1};
    int values8[] = {0,0,0};
    
    addNode_b2(values5, 0);
    addNode_b2(values6, 2);
    addNode_b2(values7, 3);
    addNode_b2(values8, 1);

    printList_b2();

    removeNode_b2();

    printList_b2();

    removeNode_b2();

    printList_b2();

    removeNode_b2();

    printList_b2();

    removeNode_b2();

    printList_b2();

    // removeNode_b2();



    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    pthread_cond_destroy(&full_b1);
    pthread_cond_destroy(&empty_b1);
    pthread_cond_destroy(&full_b2);
    pthread_cond_destroy(&empty_b2);
    return 0;
}
