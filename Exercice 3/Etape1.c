#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

typedef struct // création d'une structure donnée, qui aura un nom et un nombre de seconde
{
    char nom[20];
    int nbSecondes;
} DONNEE;


DONNEE data[] = {"MATAGNE", 15, // initialisation des valeurs
                 "WILVERS", 10,
                 "WAGNER", 17,
                 "QUETTIER", 3,
                 "", 0};

DONNEE Param; // Lui qui va être utiliser pour des création



pthread_t ThreadSecondaire[4];          //Handler Des thread Secondaire
void *fonctionThread(DONNEE *param);    //Fonction qui va être éxécuté par les différents thread

int main()
{
    /*
     * On copie la structure de DONNEE[i] dans une variable nommer param,
     * Création des 4 threads secondaire en envoyant cette structure param
     * on attend la mort des 4 thread lancé en ne récupérant aucun paramètre
     * on quitte le programme avec return 0
    */

    fprintf(stderr,"Création des Threads secondaire \n\n");
    for (int i = 0; i < 4; i++)
    {
        memcpy(&Param, &data[i], sizeof(DONNEE));
        pthread_create(&ThreadSecondaire[i], NULL, (void *(*)(void *))fonctionThread, &Param);
    }

    
    for (int i = 0; i < 4; i++)
    {
        pthread_join(ThreadSecondaire[i], NULL);
        printf("(Thread Principal %u) Thread esclave %d terminé.\n", pthread_self(), i+1);
    }

    return 0;
}

void *fonctionThread(DONNEE *param)
{
    /*
      * On affiche la création du thread 
      * On affiche le PID, le TID, le Nom et le nombre de seconde a patiénte
      * On initialise une structure timespec pour le nanosleep,
      * On quitte la fonction en ne retournant rien
    */

    fprintf(stderr, "---------------\n");
    fprintf(stderr, "(ThreadSecondaire %d.%u) \t Le thread secondaire a bien été crée \n", getpid(), pthread_self());
    fprintf(stderr, "  - TID :  %u \n  - PID :  %d \n  - NOM : - %s \n  - SEC :  %d seconde\n", pthread_self(), getpid(),param->nom, param->nbSecondes);
    fprintf(stderr, "---------------\n\n");



    struct timespec duree;
    duree.tv_sec = param->nbSecondes; // Secondes

    nanosleep(&duree,NULL);


    pthread_exit(NULL);

}