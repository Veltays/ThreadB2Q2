#include <string.h>
#include <pthread.h>
#include <stdio.h>



typedef struct                 // création d'une structure donnée, qui aura un nom et un nombre de seconde
{
    char nom[20];
    int nbSecondes;
} DONNEE;


DONNEE data[] = {"MATAGNE", 15,              // initialisation des valeurs
                 "WILVERS", 10,
                 "WAGNER", 17,
                 "QUETTIER", 8,
                 "", 0};
                  
DONNEE Param;                              // Lui qui va être utiliser pour des création




pthread_t ThreadSecondaire[4];



void main()
{
    do
    {
       memcpy(&Param,&data[i],sizeof(DONNEE)) ;
       pthread_create(,&Param) ;
    } while (strcmp(Param.nom) == 0);
    
}