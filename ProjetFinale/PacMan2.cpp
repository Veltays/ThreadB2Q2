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
  int L;
  int C;
  int dir;
} S_PAC;

typedef struct
{
  int presence;
  pthread_t tid;
} S_CASE;

S_CASE tab[NB_LIGNE][NB_COLONNE];

// Différent thread
pthread_t threadPacMan;

//
pthread_mutex_t mutexTab;

// Fonction Thread
void *FonctionPacMan();

//Fonction Signaux
void HandlerSIGINT(int sig);

void DessineGrilleBase();
void Attente(int milli);
void setTab(int l, int c, int presence = VIDE, pthread_t tid = 0);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  EVENT_GRILLE_SDL event;
  sigset_t mask;
  struct sigaction sigAct;
  char ok;

  srand((unsigned)time(NULL));

  // Ouverture de la fenetre graphique
  printf("(MAIN%p) Ouverture de la fenetre graphique\n", pthread_self());
  fflush(stdout);
  if (OuvertureFenetreGraphique() < 0)
  {
    printf("Erreur de OuvrirGrilleSDL\n");
    fflush(stdout);
    exit(1);
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


  /*
    ? Armement du signal SIGINT

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
    perror("(Thread pincipale %u) \t Erreur lors de l'armement du signal SINGINT\n");
    exit(1);
  }
  fprintf(stderr, "(Thread principale%u) \t le signal SIGINT à bien été armé \n", pthread_self());



  /*
   ? Initialisation des mutex, des conditions etc.

  */

  pthread_mutex_init(&mutexTab, NULL);



  /*
   ? Création des différents thread


  */
  pthread_create(&threadPacMan, NULL, (void *(*)(void *))FonctionPacMan, NULL);


  /*
   ? Attente de la morts des threads


  */
  pthread_join(threadPacMan, NULL);

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
        printf("Fleche droite !\n");
        break;
      case KEY_LEFT:
        printf("Fleche gauche !\n");
        break;
      case KEY_UP:
        printf("Fleche De haut !\n");
        break;
      case KEY_DOWN:
        printf("Fleche De bas !\n");
        break;
      }
    }
  }
  printf("Attente de 1500 millisecondes...\n");
  //Attente(1500);
  // -------------------------------------------------------------------------

  // Fermeture de la fenetre
  printf("(MAIN %p) Fermeture de la fenetre graphique...", pthread_self());
  fflush(stdout);
  FermetureFenetreGraphique();
  printf("OK\n");
  fflush(stdout);

  exit(0);
}

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

//*********************************************************************************************

void *FonctionPacMan()
{
  S_PAC PacPosi;

  /*
    ? Position initiale de PACMAN on protégé bien avec un mutex 
      *On commence en 15,8 Position gauche
         ! On lock le mutex
      * on met a la case 15,8 qu'il y a un PACMAN avec notre threadId
         ! on unlock le mutec
      * On Dessine PACMAN a la case et a la directions désiré 
  */

  fprintf(stderr, " (PACMAN %d.%u) PACMAN est bien rentré dans son thread. \n", pthread_self(), getpid());
  PacPosi.L = 15;
  PacPosi.C = 8;
  PacPosi.dir = GAUCHE;

  pthread_mutex_lock(&mutexTab);
  setTab(PacPosi.L, PacPosi.C, PACMAN, pthread_self());
  pthread_mutex_unlock(&mutexTab);
  DessinePacMan(PacPosi.L, PacPosi.C, PacPosi.dir);


    /*
    ? Boucle d'avance automatique
      * On attend 0.3S
      * On vérifié qu'il n'y a personne au coordonné tab[Ligne][COlonne] grace a l'attribut présence 
        * S'il n'y a personne, on efface PACMAN
        * On vérifie qu'on est bien dans notre tableau
          * Si on est supérieur a 0 alors on décemente   
          * Sinon on se déplace a l'autre bout du tableau 
          ! On lock le mutex
        * on met a la case désiré qu'il y a un PACMAN avec notre threadId
          ! on unlock le mutec
        * On Dessine PACMAN a la case et a la directions désiré 
      Et on recommence..
  */


  while (1)
  {
    Attente(300); //Attente de 0.3s

    if (tab[PacPosi.L][PacPosi.C - 1].presence == VIDE)
    {
      EffaceCarre(PacPosi.L, PacPosi.C); // On efface PACMAN

      if (PacPosi.C > 0)                // On le decalle
        PacPosi.C--;
      else
        PacPosi.C = NB_COLONNE;


      pthread_mutex_lock(&mutexTab);                         
      setTab(PacPosi.L, PacPosi.C, PACMAN, pthread_self());
      pthread_mutex_unlock(&mutexTab);
      DessinePacMan(PacPosi.L, PacPosi.C, PacPosi.dir);
    }
  }

  
  pthread_exit(NULL);
}

void HandlerSIGINT(int sig)
{
  fprintf(stderr, "Vous etez bien entrer dans le SIGINT");
  exit(0);
}