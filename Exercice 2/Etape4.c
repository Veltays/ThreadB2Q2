#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

pthread_t Slave[4];            //? Handle de tout les threads slave
pthread_t Master;              //? Handle du thread master


void *SlaveFonction();         //? Fonction qui sera exécutée par les threadSlaves
void *MasterFonction();        //? Fonction qui sera exécutée par le  threadMaster

void HandlerSIGINT(int);       //? Fonction qui sera exécutée lors de la réception d'un signal SIGINT
void HandlerSIGUSR1(int);      //? Fonction qui sera exécutée lors de la réception d'un signal SIGUSR1


void FonctionMasterFin(void *param);

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
            printf("(Principal %u) \t Il y a eu une erreur lors de la création de l'esclave numéro %d \n",pthread_self(), i);
            exit(0);
        }
    }
    printf("(Principal %u) \t Les 4 Threads Esclaves on bien été crée \n",pthread_self());





    /*
     ? - Création des Threads Master
            - Création du threads master en utilisant pthread_create(), thread qui va exécuter la fonction MasterFonction sans prendre aucun paramètre
            - Vérification que le thread master a bien été crée
    */

    CreationOK = pthread_create(&Master,NULL,(void*(*)(void*))MasterFonction,NULL);
    if (CreationOK != 0)
        {
            printf("(Principal %u) \t Il y a eu une erreur lors de la création du thread Master \n",pthread_self());
            exit(0);
        }
    printf("(Principal %u) \t Le Thread Master à bien été crée \n\n",pthread_self());

    /*
        ? -  attendant un signal et Terminaison
            - Récupération des 4 threads slaves en attendant aucun paramètre de retour
            - Annulation du thread master
            - Attente de l'annulation du thread master
            - Quitte le programme
    */

    int ValRetour;
    for (int i = 0; i < 4; i++)
    {
        pthread_join(Slave[i],NULL);
        printf("(Principal %u) \t Thread esclave %d terminé.\n\n",pthread_self() , i);
    }

    printf("(Principal %u) \t Envoie d'une requète CANCEL au master\n", pthread_self());
    if(pthread_cancel(Master))
        printf("(Principal %u) \t ERREUR d'envoie du cancel au master\n", pthread_self());

    pthread_join(Master,NULL);


    exit(0);
}




void *SlaveFonction()
{
    /*
        ? - Démasquage de SIGUSR1
            - On crée une variable de type SIGSET dans laquels on aura un vecteur pour tous nos signaux
            - On remplis notre vecteur en sélectionnant tout les signaux
            - On supprime le SIGUSR1 de nos signaux permettant de ne pas le cacher dans la fonction SlaveThread
            - On applique le nouveau masque
    */

    sigset_t maskNoSIGUSR1;
    sigfillset(&maskNoSIGUSR1);   
    sigdelset(&maskNoSIGUSR1, SIGUSR1);   

    sigprocmask(SIG_SETMASK, &maskNoSIGUSR1, NULL);



    /*
      ? - Armement du signal SIGUSR1
            - Création d'une structure de type sigaction.
            - Association de la structure avec le Handler(Fonction à exécuter) SIGUSR1.
            - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
            - On n'active aucune option particulière lors de la réception d'un signal.
            - On lie notre signal à notre structure sigaction.
    */

   struct sigaction B;

   B.sa_handler = HandlerSIGUSR1;
   sigemptyset(&B.sa_mask);
   B.sa_flags = 0;


    if ((sigaction(SIGUSR1, &B, NULL)) == -1)
    {
        perror("(Slave) \t Erreur lors de l'armement du signal SIGUSR1\n");
        exit(1);
    }
    printf("(Slave     %u) \t le signal SIGUSR1 à bien été armé \n",pthread_self());




    /*
        ? - Affiche d'identité
            - Création d'une variable nommé NumThreadCrée qui va permettre de savoir qu'elle est le numéro du thread crée
            - Affichage du t_id et du Numéro du thread crée
    */

    static int NumThreadCree = 0;
    NumThreadCree++;
    printf("(Slave     %u) \t Thread numéro %d à bien été crée et se met en attente d'un signale\n\n", pthread_self(), NumThreadCree);



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
     ? Définition du paramètre de CANCEL
        - phtread_setcancelstate en ENABLE pour autoriser le meurtre de thread
        - phtread_setcanceltype pour définir si la mort doit être de suite ou différé
        - création d'une fonction de terminaison avec  pthread_cleanup_push() et pthread_cleanup_pop().


    */

    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
        printf("(Master    %u) \t erreur lors de la définition du CANCEL forcé \n", pthread_self());

    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL))
        printf("(Master    %u) \t erreur lors de la définition du type de CANCEL en differé \n", pthread_self());

    pthread_cleanup_push(FonctionMasterFin,0);
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
        perror("(Master) \t Erreur lors de l'armement du signal SINGINT\n");
        exit(1);
    }
    printf("(Master    %u) \t le signal SIGINT à bien été armé \n",pthread_self());

    /*
     ? - Boucle de réception de signal
        - Création d'une boucle infini qui, quand elle va recevoir un signal SIGINT va afficher son idéntité
        - lorsqu'elle recoit un signal de type SIGINT, elle va rentré dans le handlerSIGINT et puis, continué sont éxécution et se remettre en attente d'un signal
        - Vérifie qu'aucune requète d'annulation n'a été recu

    */

   int i = 0;
    printf("(Master    %u) \t Le thread master a bien été crée \n\n",pthread_self());
    while(1)
    {
        pthread_testcancel();
        pause();    
        printf("(Master    %u) \t Un signal SIGINT à bien été recu \n",pthread_self());

    };

    pthread_cleanup_pop(1);
}

void HandlerSIGINT(int sig)
{
    /*
     ? Réception du SIGINT
        - Affichage de l'identité
        - envoie d'une requete kill avec SIGUSR1 au getpid() (seul les slaves peuvent recevoir ce signal);
        - Fermeture du thread
        
    */

    printf("\n\n(Master    %u) \t Thread a bien recu le signale\n", pthread_self());
    kill(getpid(),SIGUSR1);
    printf("(Master    %u) \t Requéte kill bien envoyé \n",pthread_self());
  
}


void HandlerSIGUSR1(int sig)
{
    printf("(Slave     %u) \t Réception d'un signal de type SIGUSR1 \n", pthread_self());
    pthread_exit(NULL);
}

void FonctionMasterFin(void *param)
{
    printf("\n\n(Master    %u) \t On est rentrè dans la fonction de terminaison, le thread se termine correctement \n", pthread_self());
}



// ? exit() ou pthread_exit()

// * - exit()       : Cette fonction est utilisée pour quitter l'ensemble du programme, lorsqu'elle est appelé tout les thread se ferme
// * pthread_exit() : Cette fonction est utilisée spécifiquement pour quitter un seul et unique thread