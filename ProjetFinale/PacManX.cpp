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

int L;
int C;
int dir;

typedef struct
{
  int presence;
  pthread_t tid;
} S_CASE;

S_CASE tab[NB_LIGNE][NB_COLONNE];

// Différent thread
pthread_t ThreadPacMan;
pthread_t ThreadEvent;

// mutex
pthread_mutex_t mutexTab;

// Fonction Thread
void *FonctionPacMan();
void *FonctionEvent();

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

  // ? **************************  Mon code ****************************

  pthread_mutex_init(&mutexTab, NULL);

  /*
   ? Création des différents thread

  */

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
  { // Préparation du blocage des signaux de direction
    sigemptyset(&masque);
    sigaddset(&masque, SIGINT); 
    sigaddset(&masque, SIGHUP);
    sigaddset(&masque, SIGUSR1); 
    sigaddset(&masque, SIGUSR2); 
  }

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
  dir = GAUCHE;

  pthread_mutex_lock(&mutexTab);
  setTab(L, C, PACMAN, pthread_self());
  DessinePacMan(L, C, dir);
  pthread_mutex_unlock(&mutexTab);
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
    Attente(300); //Attente de 0.3s
    fprintf(stderr, "(PACMAN     -- %u.%d) \t FIN D'ATTENTE %d \n", pthread_self(), getpid(), dir);
    sigprocmask(SIG_SETMASK, &ancien_masque, NULL);

    fprintf(stderr, "(PACMAN     -- %u.%d) \t PACMAN dois se déplacé vers la %d \n", pthread_self(), getpid(), dir);

    pthread_mutex_lock(&mutexTab);
    setTab(L, C, VIDE);
    EffaceCarre(L, C);
    pthread_mutex_unlock(&mutexTab);

    if (L == 9 && C == 0 && dir == GAUCHE)
      C = NB_COLONNE-1;
    else 
      if (L == 9 && C == 16 && dir == DROITE)
        C = 0;
    else
      switch (dir)
      {
      case GAUCHE:
        if (tab[L][C - 1].presence == VIDE)
          C--;
        break;

      case DROITE:
        if (tab[L][C + 1].presence == VIDE)
          C++;
        break;

      case HAUT:
        if (tab[L - 1][C].presence == VIDE)
          L--;
        break;

      case BAS:
        if (tab[L + 1][C].presence == VIDE)
          L++;
        break;
      }


    sigprocmask(SIG_BLOCK, &masque, &ancien_masque);
    pthread_mutex_lock(&mutexTab);
    setTab(L, C, PACMAN, pthread_self());
    DessinePacMan(L, C, dir);
    pthread_mutex_unlock(&mutexTab);
    sigprocmask(SIG_SETMASK, &ancien_masque);


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
      }
    }
  }

  fprintf(stderr, "(EVENT      -- %u.%d) \t Clique sur le bouton Croix \n", pthread_self(), getpid()); //-7
  fprintf(stderr, "(EVENT      -- %u.%d) \t EVENT à bien quitté son Thread \n", pthread_self(), getpid());
  pthread_exit(NULL);
}

// ? **************************************************************************************************************
// ? *************************************** SIGNAUX ****************************************************
// ? **************************************************************************************************************

void HandlerSIGINT(int sig)
{
  fprintf(stderr, "\n(SIGINT     -- %u.%d) \t Vous etez bien entrer dans le SIGINT PACMAN VA A GAUCHE\n", pthread_self(), getpid());
  dir = GAUCHE;
}

void HandlerSIGHUP(int sig)
{
  dir = DROITE;
}

void HandlerSIGUSR1(int sig)
{
  dir = HAUT;
}

void HandlerSIGUSR2(int sig)
{
  dir = BAS;
}
