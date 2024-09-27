#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define READER_THREADS 10
#define WRITER_THREADS 3
#define ERASER_THREADS 1

typedef struct Node{
    int value;
    struct Node* next;
}Node;

typedef struct List{
    struct Node* head;
    int size;
}List;

List list;

int broj_citaca_ceka = 0, broj_citaca_cita = 0;
int broj_brisaca_ceka = 0, broj_brisaca_brise = 0;
int broj_pisaca_ceka = 0, broj_pisaca_pise = 0;

pthread_mutex_t m;
pthread_cond_t citaci;
pthread_cond_t brisaci;
pthread_cond_t pisaci;


void print_stanje(){
    printf("Aktivnih: Čitača: %d\tPisača: %d\tBrisača: %d\n", broj_citaca_cita, broj_pisaca_pise, broj_brisaca_brise);
    printf("Lista: ");
    Node* head = list.head;
    while(head!=NULL){
        printf("%d  ", head->value);
        head=head->next;
    }
    printf("\n\n");
}


int procitaj_element(int index){
    Node* head = list.head;
    while(index!=0){
        head=head->next;
        index--;
    }
    return head->value;
}

void obrisi_element(int index){
    Node* head = list.head;

    if (index == 0){
        list.head = head->next;
        free(head);
    } else {
        for (int i = 0; i < index - 1; i++) {
            head = head->next;
        }
        Node* temp = head->next;
        head->next = temp->next;
        free(temp);
    }

    list.size--;
}

void zapisi_element(int value){
    Node* head = list.head;
    Node* newElement = malloc(sizeof(Node));
    newElement->value = value;
    newElement->next = NULL;
    if(head==NULL){
        list.head=newElement;
    }else{
        while(head->next!=NULL){
            head = head->next;
        }
        head->next = newElement;
    }
    list.size++;
}


void *reader(void *arg) {
    int I = *(int *)arg;
    while (1) {
        int x = rand() % list.size;
        pthread_mutex_lock(&m);
        printf("Čitač %d želi čitati element %d liste\n", I, x);
        print_stanje();
        broj_citaca_ceka++;
        
        while(broj_brisaca_brise + broj_brisaca_ceka > 0){
            pthread_cond_wait(&citaci, &m);
        }

        broj_citaca_cita++;
        broj_citaca_ceka--;

        if(x>=list.size){
            int x_novi = rand() % list.size;
            printf("Čitač %d ne može pročitati element %d liste jer je obrisan dok je čekao. Čita ipak element %d\n", I, x, x_novi);
            x = x_novi;
        }
        int y = procitaj_element(x);
        printf("Čitač %d čita element %d liste (vrijednost %d)\n", I, x, y);
        print_stanje();
        pthread_mutex_unlock(&m);
        
        sleep(rand() % 6 + 5); 
        
        pthread_mutex_lock(&m);
        broj_citaca_cita--;
        if (broj_citaca_cita == 0 && broj_brisaca_ceka > 0) {
            pthread_cond_broadcast(&brisaci);
        }
        printf("Čitač %d više ne koristi listu\n", I);
        print_stanje();
        pthread_mutex_unlock(&m);

        sleep(rand() % 6 + 5);
    }
    return NULL;
}


void *writer(void *arg) {
    int I = *(int *)arg;
    while (1) {

        pthread_mutex_lock(&m);

        int x = rand() % 20;
        printf("Pisač %d želi dodati novi element s vrijednosti %d na kraj liste\n", I, x);
        print_stanje();
        broj_pisaca_ceka++;
        while (broj_brisaca_brise + broj_brisaca_ceka + broj_pisaca_pise > 0) {
            pthread_cond_wait(&pisaci, &m);
        }
        broj_pisaca_ceka--; 
        broj_pisaca_pise++;

        printf("Pisač %d počinje sa zapisivanjem novog elementa s vrijednosti %d na kraj liste\n", I, x);
        print_stanje();
        pthread_mutex_unlock(&m);
        
        sleep(rand() % 4 + 3); 
        
        pthread_mutex_lock(&m);

        zapisi_element(x);
        printf("Pisač %d je zapisao element s vrijednosti %d na kraj liste\n", I, x);
        print_stanje();

        broj_pisaca_pise--;
        if(broj_brisaca_ceka > 0){
            pthread_cond_broadcast(&brisaci);
        }
        if (broj_pisaca_ceka > 0) {
            pthread_cond_broadcast(&pisaci);
        }
        pthread_mutex_unlock(&m);


        sleep(rand() % 2 + 3); 
    }
    return NULL;
}

void *eraser(void *arg) {
    int I = *(int *)arg;
    while (1) {

        pthread_mutex_lock(&m);

        int x = rand() % list.size;
        
        printf("Brisač %d želi obrisati element %d\n", I, x);
        print_stanje();

        broj_brisaca_ceka++;
        while (broj_citaca_cita + broj_pisaca_pise > 0) {
            pthread_cond_wait(&brisaci, &m);
        }
        broj_brisaca_ceka--; 
        broj_brisaca_brise++;

        printf("Brisač %d započinje brisanje elementa %d\n", I, x);
        print_stanje();

        pthread_mutex_unlock(&m);
        
        sleep(rand() % 6 + 5); 
        
        pthread_mutex_lock(&m);
        
        obrisi_element(x);
        printf("Brisač %d je obrisao element %d\n", I, x);
        print_stanje();

        broj_brisaca_brise--;
        if (broj_citaca_ceka > 0) {
            pthread_cond_broadcast(&citaci);
        }
        if (broj_pisaca_ceka > 0) {
            pthread_cond_broadcast(&pisaci);
        }
        pthread_mutex_unlock(&m);

        sleep(rand() % 6 + 5); 
    }
    return NULL;
}

int main() {
    pthread_t readers[READER_THREADS];
    int readers_id[READER_THREADS];
    pthread_t writers[WRITER_THREADS];
    int writers_id[WRITER_THREADS];
    pthread_t erasers[ERASER_THREADS];
    int erasers_id[ERASER_THREADS];

    srand(time(NULL));

    list.head = NULL;
    list.size = 0;

    for (int i = 0; i < WRITER_THREADS; i++) {
        writers_id[i] = i;
        pthread_create(&writers[i], NULL, writer, &writers_id[i]);
    }

    sleep(10);

    for (int i = 0; i < READER_THREADS; i++) {
        readers_id[i] = i;
        pthread_create(&readers[i], NULL, reader, &readers_id[i]);
    }


    for (int i = 0; i < ERASER_THREADS; i++) {
        erasers_id[i] = i;
        pthread_create(&erasers[i], NULL, eraser, &erasers_id[i]);
    }


    for (int i = 0; i < READER_THREADS; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < WRITER_THREADS; i++) {
        pthread_join(writers[i], NULL);
    }

    for (int i = 0; i < ERASER_THREADS; i++) {
        pthread_join(erasers[i], NULL);
    }
    return 0;
}