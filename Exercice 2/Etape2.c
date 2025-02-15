#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

pthread_t Slave[4];            //? Handle de tout les threads slave
pthread_t Master;              //? Handle du thread master


void *SlaveFonction();         //? Fonction qui sera exécutée par les threadSlaves
void *MasterFonction();        //? Fonction qui sera exécutée par le  threadMaster

void HandlerSIGINT(int);       //? Fonction qui sera exécutée  lors de la réception d'un SIGINT

int main()
{
    /*
        ? - Masquage de tout les signaux
            - On crée une variable de type sigset_t pour répresenté un ensemble de signaux
            - On remplit ce vecteur que de 1 avec sigfillset pour inclure tout les signaux
            - Utilisation de sigprocmask avec SIG_SETMASK afin de bloquer tous les signaux spécifié dans maskALL
    */
    sigset_t maskALL;
    sigfillset(&maskALL);
    sigprocmask(SIG_SETMASK, &maskALL, NULL);






    /*
     ? - Création des Threads Slave
            - Création d'une variable 'CréationOK' qui permettra de vérifier que la création s'est passée correctement
            - Boucle permettant de crée 4 Thread Slave qui vont chacun à leurs tours rentré dans la fonction SlaveFonction sans paramètre
            - Vérification que chaque Thread Slave vont belle et bien être crée
    */

    int CreationOK; 

    for (int i = 0; i < 4; i++)
    {

        CreationOK = pthread_create(&Slave[i], NULL, (void *(*)(void *))SlaveFonction, NULL);

        if (CreationOK != 0)
        {
            printf("(Thread Principale %u) Il y a eu une erreur lors de la création de l'esclave numéro %d \n",pthread_self(), i);
            exit(0);
        }
    }
    printf("(Thread Principale %u) Les 4 Threads Esclaves on bien été crée \n",pthread_self());




    /*
     ? - Création des Threads Master
            - Création du threads master en utilisant pthread_create(), thread qui va exécuter la fonction MasterFonction sans prendre aucun paramètre
            - Vérification que le thread master a bien été crée
    */

    CreationOK = pthread_create(&Master,NULL,(void*(*)(void*))MasterFonction,NULL);
    if (CreationOK != 0)
        {
            printf("(Thread Principale %u) Il y a eu une erreur lors de la création du thread Master \n",pthread_self());
            exit(0);
        }
    printf("(Thread Principale %u) Le Thread Master à bien été crée \n",pthread_self());

    /*
        ? - Mise en pause en attendant un signal et Terminaison
            - Récupération des 4 threads slaves en attendant aucun paramètre de retour
            - Utilisation de la fonction pause() qui va bloquer le processus en attendant n'importe qu'elle signale
            - Quitte le programme
    */

    int ValRetour;
    for (int i = 0; i < 4; i++)
    {
        pthread_join(Slave[i],NULL);
        printf("(Thread Principal %u) Thread esclave %d terminé.\n",pthread_self(), i);
    }

    pause();    
    exit(0);
}




void *SlaveFonction()
{

    /*
        ? - Affiche d'identité
            - Création d'une variable nommé NumThreadCrée qui va permettre de savoir qu'elle est le numéro du thread crée
            - Affichage du t_id et du Numéro du thread crée

    */

    static int NumThreadCree = 0;
    NumThreadCree++;
    printf("- (Thread %u)  - Thread numéro %d à bien été crée et se met en attente d'un signale\n", pthread_self(), NumThreadCree);

    /*
     ? Utilisation des signaux
        - Mise en pause en attendant la réception d'un signale SIGINT
        - Le signal SIGINT met fin a la pause, permettant au thread de poursuivre son exécution
        - Quitte la fonction en ne renvoyant rien
    */

    pause();
    pthread_exit(NULL);
    

}


void *MasterFonction(){
    


    /*
        ? - Démasquage de SIGINT
            - On crée une variable de type SIGSET dans laquels on aura un vecteur pour tous nos signaux
            - On remplis notre vecteur en sélectionnant tout les signaux
            - On supprime le SIGINT de nos signaux permettant de ne pas le cacher dans la fonction MasterThread
            - On applique le nouveau masque


    */

    sigset_t maskNoSIGINT;
    sigfillset(&maskNoSIGINT);   
    sigdelset(&maskNoSIGINT, SIGINT);   

    sigprocmask(SIG_SETMASK, &maskNoSIGINT, NULL);


    /*
      ? - Armement du signal SIGINT
            - Création d'une structure de type sigaction.
            - Association de la structure avec le Handler(Fonction à exécuter) SIGINT.
            - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
            - On n'active aucune option particulière lors de la réception d'un signal.
            - On lie notre signal à notre structure sigaction.
    */
    struct sigaction A; 

    A.sa_handler = HandlerSIGINT;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if ((sigaction(SIGINT, &A, NULL)) == -1)
    {
        perror("(Thread Master) Erreur lors de l'armement du signal SINGINT\n");
        exit(1);
    }
    printf("(Thread Master  %u) le signal SIGINT à bien été armé \n",pthread_self());

    /*
     ? - Boucle de réception de signal
        - Création d'une boucle infini qui, quand elle va recevoir un signal SIGINT va afficher son idéntité
        - lorsqu'elle recoit un signal de type SIGINT, elle va rentré dans le handlerSIGINT et puis, continué sont éxécution et se remettre en attente d'un signal


    */


    printf("(Thread Master %u) Le thread master a bien été crée \n",pthread_self());
    while(true)
    {
        pause();    
        printf("(Thread Master %u) Un signal SIGINT à bien été recu \n",pthread_self());
    };
}

void HandlerSIGINT(int sig)
{
    /*
     ? Réception du SIGINT
        - Affichage de l'identité
        - Fermeture du thread
        
    */

    printf("- (Thread %u)  - Thread a bien recu le signale\n", pthread_self());
    //pthread_exit(NULL);
    

}



// ? exit() ou pthread_exit()

// * - exit()       : Cette fonction est utilisée pour quitter l'ensemble du programme, lorsqu'elle est appelé tout les thread se ferme
// * pthread_exit() : Cette fonction est utilisée spécifiquement pour quitter un seul et unique thread