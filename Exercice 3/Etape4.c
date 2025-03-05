#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>



typedef struct                          //? création d'une structure donnée, qui aura un nom et un nombre de seconde
{
    char nom[20];
    int nbSecondes;
} DONNEE;


DONNEE data[] = {"MATAGNE", 15,         //? création d'une structure donnée, qui aura un nom et un nombre de seconde
                 "WILVERS", 10,
                 "WAGNER", 17,
                 "QUETTIER", 3,
                 "", 0};

DONNEE Param;                           //? Variable globale qui permettra d'envoyer les données dans la fonction/



pthread_t ThreadSecondaire[4];          //? Handler Des thread Secondaire



pthread_mutex_t MutexParam;             //? Initialisation du mutex pour proteger la variable globale Param
pthread_mutex_t MutexCompteur;          //? Initialisation du mutex pour proteger la variable globale compteur lors des incrémentations

pthread_cond_t condCompteur;            //? Variable qui permettra de reconnaitre une conditions

pthread_key_t cle;


int compteur;                           //? Variable globale qui permet de savoir combien de Thread secondaire on a 


void *fonctionThread(DONNEE *param);    // ?Fonction qui va être éxécuté par les différents thread
void HandlerSIGINT(int);



int main()
{
    /* 
    ? Fonction Main  
     * Initialisation des différents mutex, et variable de conditions, key
     * On supprime tout les signaux
     * Boucle de création
        * Ouverture d'un mutex pour protégé la variable globale Param 
        * On copie la structure de DONNEE[i] dans une variable nommer param,
        * Création des 4 threads secondaire en envoyant cette structure param
     
     * On lock la variable Compteur
     * Boucle vérification de si il reste des Thread vivant, si positive, alors on attend(quelqu'un est encore vivant) sinon tt le monde est mort
        * On attendra que la conditions soit envoyé,    
     * On unlock le thread 

     * on quitte le programme avec return 0
    */


    pthread_key_create(&cle,NULL);


    // Initialisation des différents mutex, et variable de conditions
    pthread_mutex_init(&MutexParam,NULL);
    pthread_mutex_init(&MutexCompteur,NULL);
    pthread_cond_init(&condCompteur, NULL);


    //on masque le SIGINT

    sigset_t maskSIGINT;
    sigemptyset(&maskSIGINT);
    sigaddset(&maskSIGINT, SIGINT);
    sigprocmask(SIG_SETMASK, &maskSIGINT, NULL);


    // Création des différents Thread avec une protection de param
    fprintf(stderr,"Création des Threads secondaire \n\n");
    for (int i = 0; i < 4; i++)
    {
        pthread_mutex_lock(&MutexParam);         
        memcpy(&Param, &data[i], sizeof(DONNEE));
        pthread_create(&ThreadSecondaire[i], NULL, (void *(*)(void *))fonctionThread, &Param);
    }


    // Notification de morts des différentes Thread, avec une protection du compteur
    pthread_mutex_lock(&MutexCompteur);
    while(compteur > 0)
    {
        pthread_cond_wait(&condCompteur,&MutexCompteur);
        fprintf(stderr,"(Thread principale %u) un thread est mort \n",pthread_self());
    }
    pthread_mutex_unlock(&MutexCompteur);


    //Affichage que tout le monde est mort 
    fprintf(stderr,"Tous le monde est mort \n");

    return 0;
}



    /* 
    ? Fonction Thread
      * On lock le compteur pour l'incrementer
      * On affiche l'identité du thread,
      * On sauvegarde le param pour l'afficher lors de la mort
      * On initialise la structure duree pour le nanosleep
      * On unlock le mutexParam
      * On attend x seconde
      * On affiche que le Thread a fini d'attendre
      * On lock le compteur et on le décremente
      * Si le compteur est positive
        * On envoie un signale pour activé la conditions dans le main
      * On quitte la fonction main
    */

void *fonctionThread(DONNEE *param)
{   

    // On démasque le SIGINT
    sigset_t maskSIGINT;
    sigdelset(&maskSIGINT, SIGINT);
    sigprocmask(SIG_SETMASK, &maskSIGINT, NULL);

    //armement du signal SIGINT

    struct sigaction A; 
    A.sa_handler = HandlerSIGINT; 
    sigemptyset(&A.sa_mask);    
    A.sa_flags = 0;

   if ((sigaction(SIGINT, &A, NULL)) == -1)
    {
      fprintf(stderr,"(ThreadSecondaire %d.%u) erreur de l'armement de SIGINT",pthread_self(), getpid());
      exit(0);
    }

    // On lock le compteur pour l'incrementer
    pthread_mutex_lock(&MutexCompteur);
    compteur++;
    pthread_mutex_unlock(&MutexCompteur);

    // On affiche l'identité du thread,
    fprintf(stderr, "---------------\n");
    fprintf(stderr, "(ThreadSecondaire %d.%u) \t Le thread secondaire a bien été crée \n", getpid(), pthread_self());
    fprintf(stderr, "  - TID :  %u \n  - PID :  %d \n  - NOM :  %s \n  - SEC :  %d seconde\n", pthread_self(), getpid(),param->nom, param->nbSecondes);
    fprintf(stderr, "---------------\n\n");

    //On sauvegarde le param pour l'afficher lors de la mort
    char *Nom = (char *)malloc(sizeof(param->nom));
    strcpy(Nom, param->nom); // Copier le nom dans l'espace alloué
    int Seconde = param->nbSecondes;

    if (pthread_setspecific(cle, (void *)Nom))
        fprintf(stderr,"Une erreur est survenue lors du setspecific");

        


    //On initialise la structure duree pour le nanosleep
    struct timespec duree,reste;
    duree.tv_sec = param->nbSecondes; // Secondes


    //On unlock le mutexParam
    pthread_mutex_unlock(&MutexParam);

    //On attend x seconde

    do
    {
        if(nanosleep(&duree,&reste) == -1)
        {
            fprintf(stderr,"(ThreadSecondaire %d.%u) La fonction a été interrompu lors de son attente de %d sec,de il lui reste %ld seconde à attendre \n",pthread_self(), getpid(),duree.tv_sec,reste.tv_sec);
            duree.tv_sec = reste.tv_sec;
        }
    } while (reste.tv_sec > 0);
    

    
    fprintf(stderr, "(ThreadSecondaire %d.%u) \t Le thread secondaire %s a bien fini d'attendre %d  \n",getpid(), pthread_self(), Nom,Seconde);


    //On lock le compteur et on le décremente
    pthread_mutex_lock(&MutexCompteur);
    compteur--;
    pthread_cond_signal(&condCompteur);  //On envoie un signale pour activé la conditions dans le main
    pthread_mutex_unlock(&MutexCompteur);

    // On quitte le thread
    pthread_exit(NULL);

}


void HandlerSIGINT(int signal)
{
    fprintf(stderr,"\n\n----------------------SIGINT----------------------\n");
    fprintf(stderr,"\n(ThreadSecondaire %d.%u) est rentré dans le SIGINT \n",pthread_self(), getpid());

    char * NomThread = (char*)pthread_getspecific(cle);
    fprintf(stderr,"(ThreadSecondaire %d.%u) s'occupe de %s \n",pthread_self(), getpid(),NomThread);



    fprintf(stderr,"\n\n------------------FIN SIGINT----------------------\n");
    
}