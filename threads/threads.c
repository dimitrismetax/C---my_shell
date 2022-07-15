//Author: CHRISTOS DIMITRIS METAXAKIS CSD3802
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t chill, ready, serve;
int clients_counter = 0;

void *chef(void *arg)
{
    //wait
    sem_wait(&chill);
    printf("Chef is here..\n");
    sleep(1);

    sem_post(&chill);
}

void *client(void *arg)
{
    sem_post(&ready);
    printf("Customer %d **ARRIVED**\n", arg + 1);

    sleep(3);
    sem_wait(&serve);

    printf("Customer %d entered in-->\n", arg + 1);
    printf("Customer %d takes away food!!\n", arg + 1);
    printf("Customer %d exited<--\n", arg + 1);

    //signal
    sem_post(&serve);
}

int main(int argc, char *argv[])
{
    int i = 0;
    clients_counter = atoi(argv[1]);
    sem_init(&chill, 0, 1);
    sem_init(&ready, 0, 0);
    sem_init(&serve, 0, 1);
    pthread_t t1, t2[clients_counter];
    pthread_create(&t1, NULL, chef, NULL);
    sleep(2);
    for (i = 1; i <= clients_counter; i++)
    {
        pthread_create(&t2[i - 1], NULL, client, (void *)(i - 1));
        if (i % 3 == 0 || i == clients_counter)
        {
            printf("\n");
            sleep(4);
            printf("\nChef chills->FACEBOOK!\n");
        }
        sem_post(&serve);
    }
    pthread_join(t1, NULL);
    for (i = 0; i < clients_counter; i++)
    {
        pthread_join(t2[i], NULL);
    }
    sem_destroy(&chill);
    sem_destroy(&ready);
    sem_destroy(&serve);
    return 0;
}