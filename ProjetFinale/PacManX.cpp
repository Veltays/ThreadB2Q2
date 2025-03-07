#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE 0
#define MUR 1
#define PACMAN 2
#define PACGOM 3
#define SUPERPACGOM 4
#define BONUS 5
#define FANTOME 6

// Autres macros
#define LENTREE 15
#define CENTREE 8

typedef struct
{
  int L;
  int C;
  int couleur;
  int cache;
} S_FANTOME;

typedef struct
{
  int presence;
  pthread_t tid;
} S_CASE;

S_CASE tab[NB_LIGNE][NB_COLONNE];

// Variable Globale utile
int L;              //! Position de pacman sur les lignes
int C;              //! Position de pacman sur les colonnes
int dir;            //! Direction
int delais = 300;   //! Vitesse de PACMAN
int nbPacGom = 0;   //! Nombre de PacGom et de SuperPACGOM présente dans la grille
int NiveauJeu = 1;  //! Variable contenant le niveau du eu
int score = 0;
// Différent thread
pthread_t ThreadPacMan; //! Handler du thread PACMAN (notre personnage)
pthread_t ThreadEvent;  //! Handler du thread Event qui récupérera les different évenements
pthread_t ThreadPacGom; //! Handler du thread PACGOM, qui se chargera de placé les PACGOM et d'incrémenter le niveau

// mutex
pthread_mutex_t mutexTab;      //! Mutex pour éviter que plusieurs accède en même temp au tableau
pthread_mutex_t mutexDir;      //! Mutex pour éviter que plusieurs thread change dir en meme temp
pthread_mutex_t mutexNbPacGom; //! Mutex pour éviter que plusieurs thread incrémente/décremente la PacGom
pthread_mutex_t mutexDelais;   //! Mutex pour éviter que le délais sois modifé par plusieurs thread
// condition

pthread_cond_t condNbPacGom;

// Fonction Thread
void *FonctionPacMan();
void *FonctionEvent();
void *FonctionPacGom();

//Fonction Signaux
void HandlerSIGINT(int sig);  // <- gauche
void HandlerSIGHUP(int sig);  // -> droite
void HandlerSIGUSR1(int sig); // ^ up
void HandlerSIGUSR2(int sig); // v down

// FonctionUtile
void DessineGrilleBase();
void Attente(int milli);
void setTab(int l, int c, int presence = VIDE, pthread_t tid = 0);

void PacInfo();
const char *presence_nom(int);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

  EVENT_GRILLE_SDL event;
  sigset_t mask;
  struct sigaction sigAct;
  char ok;

  srand((unsigned)time(NULL));

  { // Ouverture de la fenetre graphique

    fprintf(stderr, "(PRINCIPALE -- %u.%d) \t Ouverture de la fenetre graphique\n", pthread_self(), getpid());
    fflush(stdout);
    if (OuvertureFenetreGraphique() < 0)
    {
      printf("Erreur de OuvrirGrilleSDL\n");
      fflush(stdout);
      exit(1);
    }
  }

  DessineGrilleBase();

  // Exemple d'utilisation de GrilleSDL et Ressources --> code a supprimer
  // DessinePacMan(17, 7, GAUCHE); // Attention !!! tab n'est pas modifie --> a vous de le faire !!!
  // DessineChiffre(14, 25, 9);
  // DessineFantome(5, 9, ROUGE, DROITE);
  // DessinePacGom(7, 4);
  // DessineSuperPacGom(9, 5);
  // DessineFantomeComestible(13, 15);
  // DessineBonus(5, 15);

  // ? **************************************************************************************************************
  // ? ***************************************MON CODE DANS MAIN ****************************************************
  // ? **************************************************************************************************************
  /*
   ? Mettre en place les données
    DessineChiffre(14, 25, 9);


  */
    DessineChiffre(14, 22, NiveauJeu);



  /*
    ? Initialisation des différents mutex et condition
  */

  pthread_mutex_init(&mutexTab, NULL);
  pthread_mutex_init(&mutexDir, NULL);
  pthread_mutex_init(&mutexNbPacGom, NULL);
  pthread_mutex_init(&mutexDelais, NULL);


  pthread_cond_init(&condNbPacGom, NULL);

  /*
   ? Création des différents thread

  */
  pthread_create(&ThreadPacGom, NULL, (void *(*)(void *))FonctionPacGom, NULL);
  pthread_create(&ThreadPacMan, NULL, (void *(*)(void *))FonctionPacMan, NULL);
  pthread_create(&ThreadEvent, NULL, (void *(*)(void *))FonctionEvent, NULL);

  /*
   ? Attente de la morts des threads
    * Attente de la mort du thread EVENT qui ne retourne rien

  */

  pthread_join(ThreadEvent, NULL);

  /*Fin du programme*/

  fprintf(stderr, "(PRINCIPALE -- %u.%d) \t Attente de 1500 millisecondes...\n", pthread_self(), getpid());
  //Attente(1500);
  // -------------------------------------------------------------------------

  // Fermeture de la fenetre
  fprintf(stderr, "(PRINCIPALE -- %u.%d) \t Fermeture de la fenetre graphique...\n", pthread_self(), getpid());
  fflush(stdout);
  FermetureFenetreGraphique();
  fprintf(stderr, "(PRINCIPALE -- %u.%d) \t OK\n", pthread_self(), getpid());
  fflush(stdout);

  exit(0);
}

// ? **************************************************************************************************************
// ? ***************************************FONCTION UTILE ****************************************************
// ? **************************************************************************************************************

//*********************************************************************************************
void Attente(int milli)
{
  struct timespec del;
  del.tv_sec = milli / 1000;
  del.tv_nsec = (milli % 1000) * 1000000;
  nanosleep(&del, NULL);
}

//*********************************************************************************************
void setTab(int l, int c, int presence, pthread_t tid)
{
  tab[l][c].presence = presence;
  tab[l][c].tid = tid;
}

//*********************************************************************************************
void DessineGrilleBase()
{
  int t[NB_LIGNE][NB_COLONNE] = {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
                                 {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                                 {1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1},
                                 {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

  for (int l = 0; l < NB_LIGNE; l++)
    for (int c = 0; c < NB_COLONNE; c++)
    {
      if (t[l][c] == VIDE)
      {
        setTab(l, c);
        EffaceCarre(l, c);
      }
      if (t[l][c] == MUR)
      {
        setTab(l, c, MUR);
        DessineMur(l, c);
      }
    }
}

void PacInfo()
{
  fprintf(stderr, "\n\n ---------------------------- PAC INFO ----------------------------------- \n");
  fprintf(stderr, "(PACMAN     -- %u.%d)\n", pthread_self(), getpid());
  fprintf(stderr, "Position  : tab[%d][%d]\n", L, C);
  fprintf(stderr, "Direction : %d \n", dir);
  fprintf(stderr, "Delais : %d \n", delais);
  fprintf(stderr, "score : %d \n", score);
  fprintf(stderr, "PacGomRestante : %d \n", nbPacGom);
  fprintf(stderr, "Obstacle  : \n");
  fprintf(stderr, "\t     %d\n", tab[L - 1][C].presence);
  fprintf(stderr, "\t %d   %d   %d\n", tab[L][C - 1].presence, tab[L][C].presence, tab[L][C + 1].presence);
  fprintf(stderr, "\t     %d\n", tab[L + 1][C].presence);
  fprintf(stderr, "\n");

  fprintf(stderr, "\t                   %-15s\n", presence_nom(tab[L - 1][C].presence));
  fprintf(stderr, "\t %-15s  %-15s   %-15s\n",
          presence_nom(tab[L][C - 1].presence),
          presence_nom(tab[L][C].presence),
          presence_nom(tab[L][C + 1].presence));
  fprintf(stderr, "\t                   %-15s\n", presence_nom(tab[L + 1][C].presence));
  fprintf(stderr, "\n\n ---------------------------- -------------------------------------------- \n");
  fprintf(stderr, "\n");
}

const char *presence_nom(int presence)
{
  switch (presence)
  {
  case VIDE:
    return "Vide";
  case MUR:
    return "Mur";
  case PACMAN:
    return "Pac-Man";
  case PACGOM:
    return "Pac-Gom";
  case SUPERPACGOM:
    return "Super Pac-Gom";
  case BONUS:
    return "Bonus";
  case FANTOME:
    return "Fantôme";
  default:
    return "Inconnu";
  }
}


void PlacerPacGom()
{
    sigset_t masque, ancien_masque;
    // Préparation du blocage des signaux de direction
    sigemptyset(&masque);
    sigaddset(&masque, SIGINT);
    sigaddset(&masque, SIGHUP);
    sigaddset(&masque, SIGUSR1);
    sigaddset(&masque, SIGUSR2);

    sigprocmask(SIG_BLOCK, &masque, &ancien_masque);

    // int VecteurSuperPacGOM[4][2] = {{2, 1}, {2, 15}, {15, 1}, {15, 15}};
    // pthread_mutex_lock(&mutexTab);
    // for (int k = 0; k < 4; k++)
    // {
    //   fprintf(stderr, "(PACGOM     -- %u.%d) \t Je place la SUPERPACGOM EN tab[%d][%d] \n", pthread_self(), getpid(), VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1]);
    //   setTab(VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1], SUPERPACGOM, pthread_self());
    //   DessineSuperPacGom(VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1]);

    //   pthread_mutex_lock(&mutexNbPacGom);
    //   nbPacGom++;
    //   pthread_mutex_unlock(&mutexNbPacGom);
    // }
    // pthread_mutex_unlock(&mutexTab);


    pthread_mutex_lock(&mutexTab);
    for (int i = 0; i < NB_LIGNE/4; i++)
      for (int j = 0; j < NB_COLONNE/4; j++)
        if (tab[i][j].presence == VIDE)
        {
          fprintf(stderr, "(PACGOM     -- %u.%d) \t Je place la PACGOM EN tab[%d][%d] \n", pthread_self(), getpid(), i, j);

          setTab(i, j, PACGOM, pthread_self());
          DessinePacGom(i, j);
    
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom++;
          pthread_mutex_unlock(&mutexNbPacGom);
        }

    pthread_mutex_unlock(&mutexTab);

    if(nbPacGom != 0)
    {
      DessineChiffre(12, 24, (nbPacGom % 10));
      DessineChiffre(12, 23, ((nbPacGom / 10) % 10));
      DessineChiffre(12, 22, ((nbPacGom / 100) % 10));
    }
    else
      DessineChiffre(12, 24, 0);
    sigprocmask(SIG_SETMASK,&ancien_masque,NULL);
}



// ? **************************************************************************************************************
// ? ***************************************FONCTION DE THREAD ****************************************************
// ? **************************************************************************************************************

void *FonctionPacMan()
{

  { // Armement des signaux

    /*
    ? Armement du signal SIGINT

      - Création d'une structure de type sigaction.
      - Association de la structure avec le Handler(Fonction à exécuter) SIGINT.
      - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
      - On n'active aucune option particulière lors de la réception d'un signal.
      - On lie notre signal à notre structure sigaction.
  */

    struct sigaction LEFT;
    LEFT.sa_handler = HandlerSIGINT;
    sigemptyset(&LEFT.sa_mask);
    LEFT.sa_flags = 0;

    if ((sigaction(SIGINT, &LEFT, NULL)) == -1)
    {
      fprintf(stderr, "(PACMAN     -- %u.%d) \t ERREUR lors de l'Armenent du SIGINT  \n", pthread_self(), getpid());
      exit(1);
    }
    fprintf(stderr, "(PACMAN     -- %u.%d) \t Armenent du SIGINT s'est déroulé sans soucis \n", pthread_self(), getpid());

    /*
    ? Armement du signal SIGHUP

      - Création d'une structure de type sigaction.
      - Association de la structure avec le Handler(Fonction à exécuter) SIGHUP.
      - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
      - On n'active aucune option particulière lors de la réception d'un signal.
      - On lie notre signal à notre structure sigaction.
  */

    struct sigaction RIGHT;
    RIGHT.sa_handler = HandlerSIGHUP;
    sigemptyset(&RIGHT.sa_mask);
    RIGHT.sa_flags = 0;

    if ((sigaction(SIGHUP, &RIGHT, NULL)) == -1)
    {
      fprintf(stderr, "(PACMAN     -- %u.%d) \t ERREUR lors de l'Armenent du SIGHUP  \n", pthread_self(), getpid());
      exit(1);
    }
    fprintf(stderr, "(PACMAN     -- %u.%d) \t Armenent du SIGHUP s'est déroulé sans soucis \n", pthread_self(), getpid());

    /*
    ? Armement du signal SIGUSR1

      - Création d'une structure de type sigaction.
      - Association de la structure avec le Handler(Fonction à exécuter) SIGUSR1.
      - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
      - On n'active aucune option particulière lors de la réception d'un signal.
      - On lie notre signal à notre structure sigaction.
  */

    struct sigaction UP;
    UP.sa_handler = HandlerSIGUSR1;
    sigemptyset(&UP.sa_mask);
    UP.sa_flags = 0;

    if ((sigaction(SIGUSR1, &UP, NULL)) == -1)
    {
      fprintf(stderr, "(PACMAN     -- %u.%d) \t ERREUR lors de l'Armenent du SIGUSR1  \n", pthread_self(), getpid());
      exit(1);
    }
    fprintf(stderr, "(PACMAN     -- %u.%d) \t Armenent du SIGUSR1 s'est déroulé sans soucis \n", pthread_self(), getpid());

    /*
    ? Armement du signal SIGUSR2

      - Création d'une structure de type sigaction.
      - Association de la structure avec le Handler(Fonction à exécuter) SIGUSR2.
      - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
      - On n'active aucune option particulière lors de la réception d'un signal.
      - On lie notre signal à notre structure sigaction.
  */

    struct sigaction DOWN;
    DOWN.sa_handler = HandlerSIGUSR2;
    sigemptyset(&DOWN.sa_mask);
    DOWN.sa_flags = 0;

    if ((sigaction(SIGUSR2, &DOWN, NULL)) == -1)
    {
      fprintf(stderr, "(PACMAN     -- %u.%d) \t ERREUR lors de l'Armenent du SIGUSR2  \n", pthread_self(), getpid());
      exit(1);
    }
    fprintf(stderr, "(PACMAN     -- %u.%d) \t Armenent du SIGUSR2 s'est déroulé sans soucis \n", pthread_self(), getpid());
  }
  sigset_t masque, ancien_masque;
  // Préparation du blocage des signaux de direction
  sigemptyset(&masque);
  sigaddset(&masque, SIGINT);
  sigaddset(&masque, SIGHUP);
  sigaddset(&masque, SIGUSR1);
  sigaddset(&masque, SIGUSR2);

  
  /*
    ? Position initiale de PACMAN on protégé bien avec un mutex 
      *On commence en 15,8 Position gauche
         ! On lock le mutex
      * on met a la case 15,8 qu'il y a un PACMAN avec notre threadId
         ! on unlock le mutec
      * On Dessine PACMAN a la case et a la directions désiré 
  */
  fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN est bien rentré dans son thread. \n", pthread_self(), getpid());
  L = 15;
  C = 8;
  pthread_mutex_lock(&mutexDir);
  dir = GAUCHE;
  pthread_mutex_lock(&mutexTab);


  setTab(L, C, PACMAN, pthread_self());
  nbPacGom--;
  DessinePacMan(L, C, dir);
  pthread_mutex_unlock(&mutexTab);
  pthread_mutex_unlock(&mutexDir);
  fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN à été placé en (%d,%d) vers %d dans TAB \n", pthread_self(), getpid(), L, C, dir);

  /*
    ? Boucle d'avance automatique
      * On attend 0.3S
      * On vérifié qu'il n'y a personne au coordonné tab[Ligne][COlonne] grace a l'attribut présence 
        * S'il n'y a personne, 
          * On efface PACMAN
          * On vérifie qu'on est bien dans notre tableau
          * Si on est supérieur a 0 
            * alors on décremente   
            * Sinon on se déplace a l'autre bout du tableau 
          ! On lock le mutex
          * on met a la case désiré qu'il y a un PACMAN avec notre threadId
          ! on unlock le mutec
          * On Dessine PACMAN a la case et a la directions désiré 
      Et on recommence..
  */

  fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN rentre dans la boucle d'avance auto. \n", pthread_self(), getpid());

  while (1)
  {
    PacInfo();

    sigprocmask(SIG_BLOCK, &masque, &ancien_masque);

    pthread_mutex_lock(&mutexDelais);
    Attente(delais); //Attente de 0.3s
    pthread_mutex_unlock(&mutexDelais);

    sigprocmask(SIG_SETMASK, &ancien_masque, NULL);



    pthread_mutex_lock(&mutexTab);
    setTab(L, C);
    EffaceCarre(L, C);
    pthread_mutex_unlock(&mutexTab);


    if (L == 9 && C == 0 && dir == GAUCHE)
      C = NB_COLONNE - 1;
    else if (L == 9 && C == 16 && dir == DROITE)
      C = 0;
    else
      switch (dir)
      {
      case GAUCHE:
        if (tab[L][C - 1].presence == PACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);
          
          score +=1;
          pthread_cond_signal(&condNbPacGom);
        }

        if(tab[L][C - 1].presence == SUPERPACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=5;
          pthread_cond_signal(&condNbPacGom);
        }

        if (tab[L][C - 1].presence != MUR)
          C--;
        break;



      case DROITE:
        if (tab[L][C + 1].presence == PACGOM )
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=1;

          pthread_cond_signal(&condNbPacGom);
        }


        if(tab[L][C + 1].presence == SUPERPACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=5;
          pthread_cond_signal(&condNbPacGom);
        }


        if (tab[L][C + 1].presence != MUR)
          C++;
        break;



      case HAUT:
        if (tab[L - 1][C].presence == PACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);
          score +=1;
          pthread_cond_signal(&condNbPacGom);
        }


        if(tab[L-1][C].presence == SUPERPACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=5;
          pthread_cond_signal(&condNbPacGom);
        }


        if (tab[L - 1][C].presence != MUR)
          L--;
        break;



      case BAS:
        if (tab[L + 1][C].presence == PACGOM || tab[L + 1][C].presence == SUPERPACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=1;
          pthread_cond_signal(&condNbPacGom);
        }


        if(tab[L+1][C].presence == SUPERPACGOM)
        {
          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom--;
          pthread_mutex_unlock(&mutexNbPacGom);

          score +=5;
          pthread_cond_signal(&condNbPacGom);
        }

        if (tab[L + 1][C].presence != MUR)
          L++;
        break;
      }

    sigprocmask(SIG_BLOCK, &masque, &ancien_masque);

    pthread_mutex_lock(&mutexTab);
    setTab(L, C, PACMAN, pthread_self());
    DessinePacMan(L, C, dir);
    pthread_mutex_unlock(&mutexTab);

    sigprocmask(SIG_SETMASK, &ancien_masque, NULL);

    fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN à été placé en (%d,%d) vers %d dans TAB \n", pthread_self(), getpid(), L, C, dir);

  }
  fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN à bien quitté son Thread. \n", pthread_self(), getpid());
  pthread_exit(NULL);
}

void *FonctionEvent()
{
  fprintf(stderr, "(EVENT      -- %u.%d) \t EVENT est bien rentré dans son Thread\n", pthread_self(), getpid());
  char ok;
  EVENT_GRILLE_SDL event;

  ok = 0;
  while (!ok)
  {
    event = ReadEvent();
    if (event.type == CROIX)
      ok = 1;
    if (event.type == CLAVIER)
    {
      switch (event.touche)
      {
      case 'q':
        ok = 1;
        break;
      case KEY_RIGHT:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Flèche droite !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGHUP);
        break;
      case KEY_LEFT:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Flèche gauche !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGINT);
        break;
      case KEY_UP:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Flèche De haut !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGUSR1);
        break;
      case KEY_DOWN:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Flèche De bas !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGUSR2);
        break;
      case KEY_P:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Touche +  !\n", pthread_self(), getpid());
        pthread_mutex_lock(&mutexDelais);
        delais = delais - 20;
        pthread_mutex_unlock(&mutexDelais);
        break;
      case KEY_M:
        fprintf(stderr, "(EVENT      -- %u.%d) \t - Touche +  !\n", pthread_self(), getpid());
        pthread_mutex_lock(&mutexDelais);
        delais = delais + 20;
        pthread_mutex_unlock(&mutexDelais);
      }
    }
  }

  fprintf(stderr, "(EVENT      -- %u.%d) \t Clique sur le bouton Croix \n", pthread_self(), getpid()); //-7
  fprintf(stderr, "(EVENT      -- %u.%d) \t EVENT à bien quitté son Thread \n", pthread_self(), getpid());
  pthread_exit(NULL);
}

void *FonctionPacGom()
{
  while(true)
  {

      // Mise en place des PACGOM ET SUPERPACGOM

      /*  
        ? Boucle de placement de PACGOM et de SUPERPACGOM
          * On place toute les PACGOM et SUPERPACGOM

      */

      PlacerPacGom();

      while (nbPacGom > 0)
      {
        pthread_cond_wait(&condNbPacGom, &mutexNbPacGom);
        fprintf(stderr, "(PACGOM     -- %u.%d) \t La condition a été activée \n", pthread_self(), getpid());
        pthread_mutex_lock(&mutexTab);
        if(nbPacGom != 0)
        {
          DessineChiffre(12, 24, (nbPacGom % 10));
          DessineChiffre(12, 23, ((nbPacGom / 10) % 10));
          DessineChiffre(12, 22, ((nbPacGom / 100) % 10));
        }
        else
          DessineChiffre(12, 24, 0);
         pthread_mutex_unlock(&mutexTab);
      }



      fprintf(stderr, "(PACGOM     -- %u.%d) \t Toute les PACGOM on été récupéré \n", pthread_self(), getpid());
      NiveauJeu++;

      pthread_mutex_lock(&mutexTab);
      DessineChiffre(14, 22, NiveauJeu);
      pthread_mutex_unlock(&mutexTab);

      // On met un nouveau delais

      pthread_mutex_lock(&mutexDelais);
      delais /= 2;
      fprintf(stderr, "(PACGOM     -- %u.%d) \t Nouveau délais : %d \n", pthread_self(), getpid(), delais);
      pthread_mutex_unlock(&mutexDelais);
  }

  

    


    pthread_exit(NULL);
}

// ? **************************************************************************************************************
// ? *************************************** SIGNAUX **************************************************************
// ? **************************************************************************************************************

void HandlerSIGINT(int sig)
{
  fprintf(stderr, "\n(SIGINT     -- %u.%d) \t Vous etez bien entrer dans le SIGINT PACMAN VA A GAUCHE\n", pthread_self(), getpid());
  pthread_mutex_lock(&mutexDir);
  dir = GAUCHE;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGHUP(int sig)
{
  pthread_mutex_lock(&mutexDir);
  dir = DROITE;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGUSR1(int sig)
{
  pthread_mutex_lock(&mutexDir);
  dir = HAUT;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGUSR2(int sig)
{
  pthread_mutex_lock(&mutexDir);
  dir = BAS;
  pthread_mutex_unlock(&mutexDir);
}
