#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *fonctionThread(int *param); // ? Declaration d'une fonction qui
                                  // - return un void* ceci permet de retourner n'importe quels type de donnée

pthread_t threadHandle; //? phtread = Elle contient un type de donnée (pthread) representant un handle,
                        //? threadHandle variable qui stockera l'id du thread, elle sera utilisé pr gerer un thread

int main()
{
    fprintf(stderr, " -- Les parametre s'initialise -- \n");

    int param = 5;
    int retour;
    int *recupRetThread;

    fprintf(stderr, " ----> Initialisation OK \n");

    fprintf(stderr, " -- Tentative de création du thread -- \n");

    retour = pthread_create(&threadHandle, NULL, (void *(*)(void *))fonctionThread, &param);
    // comme param
    // - Handle du thread
    // - Attribut du thread, a null pr linnstant
    // - la fonction qui va etre executée par le thread
    // - Parametre de cette fonction S
    // elle retourne erno si il y a une erreur

    if (retour != 0)
    {
        fprintf(stderr, " ----> Thread n'as pas pu être crée \n");
        return 1;
    }
    fprintf(stderr, " ----> Thread bien crée OK \n\n");



    fprintf(stderr, " -- Tentative de récuperation de la valeur de retour -- \n");
    retour = pthread_join(threadHandle, (void **)&recupRetThread);
    if (retour != 0)
    {
        fprintf(stderr, " ----> La valeur de retour n'a pas pu être récupérer \n");
        return 1;
    }

    fprintf(stderr,"----> Valeur de retour bien récupérer : %d \n\n",*recupRetThread);
}

void *fonctionThread(int *param)
{
    static int valeurRetour = 1000; //impossible de retourner une valeur
    fprintf(stderr, " -- On rentre dans le thread -- \n\n");

    pthread_exit(&valeurRetour);
}