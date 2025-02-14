#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

pthread_t Slave[4];
pthread_t Master;

void *SlaveFonction();

void* MasterFonction();

void HandlerSIGINT(int);
void HandlerSIGUSR1(int);

int main()
{
    sigset_t mask;
    sigfillset(&mask);                    // Créer un ensemble remplis de signaux
    sigprocmask(SIG_SETMASK, &mask, NULL); // ON MASQUE TOUT


    struct sigaction A; // Armement du signale SIGINT

    A.sa_handler = HandlerSIGINT;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if ((sigaction(SIGINT, &A, NULL)) == -1)
    {
        perror("(Thread Principale) Erreur lors de l'armement de SINGINT\n");
        exit(1);
    }
    printf("le signal à bien été armé \n");



    struct sigaction B;

    B.sa_handler = HandlerSIGUSR1;
    sigemptyset(&B.sa_mask);
    B.sa_flags = 0;




    int bc;      // variable qui permettra de verifier que le thread a bien été crée 

    for (int i = 0; i < 4; i++)
    {

        bc = pthread_create(&Slave[i], NULL, (void *(*)(void *))SlaveFonction, NULL);

        if (bc != 0)
        {
            printf("Il y a eu une erreur lors de la création de l'esclave numéro %d \n", i);
            exit(0);
        }
    }

    // Creation du master

    bc = pthread_create(&Master, NULL, (void *(*)(void *))MasterFonction, NULL);



     if ((sigaction(SIGUSR1, &B, NULL)) == -1)
    {
        perror("(Thread Principale) Erreur lors de l'armement de SINGUSER1\n");
        exit(1);
    }
    printf("le signal à bien été armé \n");




    pause();    // mise en pause du thread principale

    exit(0);
}

void *SlaveFonction()
{
    static int ok = 0;
    ok++;

    printf("- (Thread %u)  - Thread numéro %d à bien été crée et se met en attente d'un signale\n", pthread_self(), ok);

    sigset_t mask;
    sigemptyset(&mask);                    // Créer un ensemble vide de signaux
    sigaddset(&mask, SIGINT);              // Ajouter SIGINT à l'ensemble
    sigprocmask(SIG_SETMASK, &mask, NULL); // Bloquer SIGINT dans le thread principal

    pthread_exit(NULL);
    return NULL;
}

void HandlerSIGINT(int sig)
{
    printf("- (Thread %u)  - Thread a bien recu le signale\n", pthread_self());
    kill(getpid(),SIGUSR1);
    pthread_exit(NULL);
}



void* MasterFonction()
{
    printf("(Master %u) bien crée \n",pthread_self());
    


    return 0;
}


void HandlerSIGUSR1(int sig)
{
    printf("- (Thread Principale) - Signale SIGUSR1 bien recu \n");
    exit(0);
}