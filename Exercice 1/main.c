#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

pthread_t threadHandle;
void* fonctionThread();

int main()
{
    int *Occurence;
    int ok = pthread_create(&threadHandle, NULL, (void *(*)(void *))fonctionThread, NULL);

    int retour = pthread_join(threadHandle, (void **)&Occurence);

    if (retour != 0)
    {
        fprintf(stderr, " ----> La valeur de retour n'a pas pu être récupérer \n");
        return 1;
    }

    fprintf(stderr, "Voici le nombre d'occurence récupéré %d\n", *Occurence);

    return 0;
}


void* fonctionThread() {

    static int occ = 0; // Allouer dynamiquement la mémoire
    int fd;
    char comp[7]; // 6 caractères + '\0'
    int VarEOF;

    if ((fd = open("Serveur.txt", O_RDONLY)) == -1) {
        perror("Erreur d'ouverture du fichier");
        pthread_exit(NULL);
    }

    // Lecture du fichier par blocs de 6 caractères
    while ((VarEOF = read(fd, comp, 7)) > 0) {
        comp[VarEOF] = '\0'; // Null-terminate la chaîne
        if (strcmp(comp, "Yassine") == 0) {
            occ++;
        }
            printf("mot lu = %s", comp);
    }

    if (VarEOF == -1) {
        perror("Erreur de lecture");
    }

    close(fd);
    pthread_exit(&occ); // Retourner le pointeur vers le nombre d'occurrences
}

