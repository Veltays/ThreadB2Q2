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
bool gameRunning = true;

int L = 15;        //! Position de pacman sur les lignes
int C = 8;         //! Position de pacman sur les colonnes
int dir = GAUCHE;  //! Direction
int delais = 300;  //! Vitesse de PACMAN
int nbPacGom = 0;  //! Nombre de PacGom et de SuperPACGOM présente dans la grille
int NiveauJeu = 0; //! Variable contenant le niveau du eu
int score = 0;
bool MAJScore = false;
int mode = 2;
int vie = 3;
int nbFantome = 0;
pthread_key_t cle;

int nbRouge = 2;
int nbVert = 2;
int nbOrange = 2;
int nbMauve = 2;

// Différent thread
pthread_t ThreadPacMan;           //! Handler du thread PACMAN (notre personnage)
pthread_t ThreadEvent;            //! Handler du thread Event qui récupérera les different évenements
pthread_t ThreadPacGom;           //! Handler du thread PACGOM, qui se chargera de placé les PACGOM et d'incrémenter le niveau
pthread_t ThreadScore;            //!Handler du thread Score, qui se chargera d'afficher le score
pthread_t ThreadBonus;            //! Handler du thread bonus qui insera aléatoirement un bonus
pthread_t threadCompteurFantomes; //! Handler du thread qui permet de compter les fantomes
pthread_t ThreadFantome[8];       //! Handler du thread Fantome crée par le ThreadCOmpteurFantome
pthread_t ThreadPacInfo;

// mutex
pthread_mutex_t mutexTab;        //! Mutex pour éviter que plusieurs accède en même temp au tableau
pthread_mutex_t mutexDir;        //! Mutex pour éviter que plusieurs thread change dir en meme temp
pthread_mutex_t mutexNbPacGom;   //! Mutex pour éviter que plusieurs thread incrémente/décremente la PacGom
pthread_mutex_t mutexDelais;     //! Mutex pour éviter que le délais sois modifé par plusieurs thread
pthread_mutex_t mutexScore;      //! Mutex pour éviter que le score sois modifier a plusieurs thread
pthread_mutex_t mutexNbFantomes; //! Mutex pour éviter que le score sois modifier a plusieurs thread
pthread_mutex_t mutexCaseDispo;


pthread_mutex_t mutexNbRouge;
pthread_mutex_t mutexNbVert;
pthread_mutex_t mutexNbOrange;
pthread_mutex_t mutexNbMauve;



// condition

pthread_cond_t condNbPacGom;
pthread_cond_t condScore;
pthread_cond_t condNbFantomes;
pthread_cond_t condPosiFantom;
pthread_cond_t condPacToutePlacer;
// Fonction Thread
void *FonctionPacMan();
void *FonctionEvent();
void *FonctionPacGom();
void *FonctionScore();
void *FonctionBonus();
void *FonctionCompteurFantome();
void *FonctionFantome(void *);
void FonctionFinFantome(void *);
void FonctionFinPacman(void *);

//Fonction Signaux
void HandlerSIGCHLD(int sig);  // <- gauche
void HandlerSIGHUP(int sig);  // -> droite
void HandlerSIGUSR1(int sig); // ^ up
void HandlerSIGUSR2(int sig); // v down

// FonctionUtile
void DessineGrilleBase();
void Attente(int milli);
void setTab(int l, int c, int presence = VIDE, pthread_t tid = 0);

void *PacInfo();
const char *presence_nom(int);
const char *posiPac(int);

void MangerPacGom(int);

S_FANTOME *InitFantom(int, int, int, int);

int CaseAleatoire(S_FANTOME *);
void set_signal_handler(int signal, void (*handler)(int));
int MovePacMan(int newL,int newC);


///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

  EVENT_GRILLE_SDL event;
  sigset_t mask;
  struct sigaction sigAct;
  char ok;

  srand((unsigned)time(NULL));

  { // Ouverture de la fenetre graphique

    fprintf(stderr, "(PRINCIPALE -- %lu.%d) \t Ouverture de la fenetre graphique\n", pthread_self(), getpid());
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
  */
  DessineChiffre(14, 22, NiveauJeu);

  DessineChiffre(16, 22, score);
  DessineChiffre(16, 23, score);
  DessineChiffre(16, 24, score);
  DessineChiffre(16, 25, score);

  DessineChiffre(12, 22, nbPacGom);
  DessineChiffre(12, 23, nbPacGom);
  DessineChiffre(12, 24, nbPacGom);

  /*
    ? Initialisation des différents mutex et condition
  */

  pthread_mutex_init(&mutexTab, NULL);
  pthread_mutex_init(&mutexDir, NULL);
  pthread_mutex_init(&mutexNbPacGom, NULL);
  pthread_mutex_init(&mutexDelais, NULL);
  pthread_mutex_init(&mutexScore, NULL);
  pthread_mutex_init(&mutexCaseDispo, NULL);

  pthread_mutex_init(&mutexNbRouge, NULL);
  pthread_mutex_init(&mutexNbVert, NULL);
  pthread_mutex_init(&mutexNbOrange, NULL);
  pthread_mutex_init(&mutexNbMauve, NULL);

  pthread_cond_init(&condNbPacGom, NULL);
  pthread_cond_init(&condScore, NULL);
  pthread_cond_init(&condPosiFantom, NULL);
  pthread_cond_init(&condPacToutePlacer, NULL);


  /*
  ? Création des différentes clé

  */
  pthread_key_create(&cle, free);

  /*
   ? Création des différents thread

  */

  pthread_create(&ThreadPacGom, NULL, (void *(*)(void *))FonctionPacGom, NULL);
  pthread_create(&ThreadPacMan, NULL, (void *(*)(void *))FonctionPacMan, NULL);
  pthread_create(&ThreadBonus, NULL, (void *(*)(void *))FonctionBonus, NULL);
  pthread_create(&threadCompteurFantomes, NULL, (void *(*)(void *))FonctionCompteurFantome, NULL);
  pthread_create(&ThreadScore, NULL, (void *(*)(void *))FonctionScore, NULL);
  pthread_create(&ThreadEvent, NULL, (void *(*)(void *))FonctionEvent, NULL);


  pthread_create(&ThreadPacInfo, NULL, (void *(*)(void *))PacInfo, NULL);


  /*
   ? Attente de la morts des threads
    * Attente de la mort du thread EVENT qui ne retourne rien

  */

  pthread_join(ThreadEvent, NULL);

  /*Fin du programme*/

  fprintf(stderr, "(PRINCIPALE -- %lu.%d) \t Attente de 1500 millisecondes...\n", pthread_self(), getpid());
  //Attente(1500);
  // -------------------------------------------------------------------------

  // Fermeture de la fenetre
  fprintf(stderr, "(PRINCIPALE -- %lu.%d) \t Fermeture de la fenetre graphique...\n", pthread_self(), getpid());
  fflush(stdout);
  FermetureFenetreGraphique();
  fprintf(stderr, "(PRINCIPALE -- %lu.%d) \t OK\n", pthread_self(), getpid());
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

void *PacInfo()
{
    while(gameRunning)
    {
      int locked;
      fprintf(stderr, "\n\n\n\n\n ---------------------------- PAC INFO ----------------------------------- \n");
      fprintf(stderr, "(PACMAN     -- %lu.%d)\n", pthread_self(), getpid());
      fprintf(stderr, "Donnée PACMAN \n");
      fprintf(stderr, "  - TID            : %lu \n", tab[L][C].tid);
      fprintf(stderr, "  - présence       : %s \n",presence_nom(tab[L][C].presence));
      fprintf(stderr, "  - Position            tab[%d][%d] \n", L, C);
      fprintf(stderr, "  - Direction          : %s \n", posiPac(dir));
      fprintf(stderr, "Niveau             : %d \n", NiveauJeu);
      fprintf(stderr, "nbVie              : %d \n", vie);
      fprintf(stderr, "Delais             : %d \n", delais);
      fprintf(stderr, "score              : %d \n", score);
      fprintf(stderr, "PacGomRestante     : %d \n", nbPacGom);
      fprintf(stderr, "Nombre De Fantome  : %d \n", nbFantome);
      fprintf(stderr, "GameRunning        : %d \n", gameRunning);
      fprintf(stderr, "  - NbVert           : %d \n", nbVert);
      fprintf(stderr, "  - NbMauve          : %d \n", nbMauve);
      fprintf(stderr, "  - NbOrange         : %d \n", nbOrange);
      fprintf(stderr, "  - nbRouge          : %d \n", nbRouge);
      fprintf(stderr, "État des Mutex :\n");
  
      locked = pthread_mutex_trylock(&mutexTab);
      fprintf(stderr, "  - mutexTab          : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexTab);
      
      locked = pthread_mutex_trylock(&mutexDir);
      fprintf(stderr, "  - mutexDir          : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexDir);
      
      locked = pthread_mutex_trylock(&mutexNbPacGom);
      fprintf(stderr, "  - mutexNbPacGom     : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexNbPacGom);
      
      locked = pthread_mutex_trylock(&mutexDelais);
      fprintf(stderr, "  - mutexDelais       : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexDelais);
      
      locked = pthread_mutex_trylock(&mutexScore);
      fprintf(stderr, "  - mutexScore        : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexScore);
      
      locked = pthread_mutex_trylock(&mutexNbFantomes);
      fprintf(stderr, "  - mutexNbFantomes   : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexNbFantomes);
      
      locked = pthread_mutex_trylock(&mutexCaseDispo);
      fprintf(stderr, "  - mutexCaseDispo    : %s\n", (locked == 0) ? "Libre" : "Verrouillé");
      if (locked == 0) 
          pthread_mutex_unlock(&mutexCaseDispo);
    
      
      fprintf(stderr, "Obstacle  : \n");
      fprintf(stderr, "------------------------------------------------------------------------- \n");
      fprintf(stderr, "\t                   %-15s\n", presence_nom(tab[L - 1][C].presence));
      fprintf(stderr, "\t %-15s  %-15s   %-15s\n", presence_nom(tab[L][C - 1].presence), presence_nom(tab[L][C].presence), presence_nom(tab[L][C + 1].presence));
      fprintf(stderr, "\t                   %-15s\n", presence_nom(tab[L + 1][C].presence));
      fprintf(stderr, "------------------------------------------------------------------------- \n");
      Attente(1000);
    }

  
    pthread_exit(NULL);
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

const char *posiPac(int direction)
{
  switch (direction)
  {
  case GAUCHE:
    return "Gauche";
  case DROITE:
    return "Droite";
  case HAUT:
    return "Haut";
  case BAS:
    return "Bas";
  default:
    return "Inconnu";
  }
}

void PlacerPacGom()
{
  sigset_t masque, ancien_masque;
  // Préparation du blocage des signaux de direction
  sigemptyset(&masque);
  sigaddset(&masque, SIGCHLD);
  sigaddset(&masque, SIGHUP);
  sigaddset(&masque, SIGUSR1);
  sigaddset(&masque, SIGUSR2);

  sigprocmask(SIG_BLOCK, &masque, &ancien_masque);

  int VecteurSuperPacGOM[4][2] = {{2, 1}, {2, 15}, {15, 1}, {15, 15}};
  pthread_mutex_lock(&mutexTab);
  for (int k = 0; k < 4; k++)
  {
    //fprintstderr, "(PACGOM     -- %lu.%d) \t Je place la SUPERPACGOM EN tab[%d][%d] \n", pthread_self(), getpid(), VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1]);
    setTab(VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1], SUPERPACGOM, pthread_self());
    DessineSuperPacGom(VecteurSuperPacGOM[k][0], VecteurSuperPacGOM[k][1]);
    pthread_mutex_lock(&mutexNbPacGom);
    nbPacGom++;
    pthread_mutex_unlock(&mutexNbPacGom);
  }

  pthread_mutex_unlock(&mutexTab);

  pthread_mutex_lock(&mutexTab);
  for (int i = 0; i < NB_LIGNE; i++)
    for (int j = 0; j < NB_COLONNE; j++)
      if (tab[i][j].presence == VIDE)
      {
        if (((i == 8 || i == 9) && j == 8) || (i == 15 && j == 8))
        {
          //fprintstderr, "(PACGOM     -- %lu.%d) \t Pas de PACGOM placé en tab[%d][%d] \n", pthread_self(), getpid(), i, j);

        }
        else
        {
          //fprintstderr, "(PACGOM     -- %lu.%d) \t Je place la PACGOM EN tab[%d][%d] \n", pthread_self(), getpid(), i, j);

          setTab(i, j, PACGOM);
          DessinePacGom(i, j);

          pthread_mutex_lock(&mutexNbPacGom);
          nbPacGom++;
          pthread_mutex_unlock(&mutexNbPacGom);
        }
      }
      else
        //fprintstderr, "(PACGOM     -- %lu.%d) \t Pas de PACGOM placé car %s est placé en tab[%d][%d] \n", pthread_self(), getpid(), presence_nom(tab[i][j].presence), i, j);

  pthread_mutex_unlock(&mutexTab);

  DessineChiffre(12, 24, (nbPacGom % 10));
  DessineChiffre(12, 23, ((nbPacGom / 10) % 10));
  DessineChiffre(12, 22, ((nbPacGom / 100) % 10));

  sigprocmask(SIG_SETMASK, &ancien_masque, NULL);

}

void MangerPacGom(int Type)
{
  if (Type == PACGOM)
  {
    //fprintstderr, "(PACMAN     -- %lu.%d) \t PACMAN vient de mangé une PACGOM \n", pthread_self(), getpid());
    pthread_mutex_lock(&mutexNbPacGom);
    nbPacGom--;
    pthread_cond_signal(&condNbPacGom);
    pthread_mutex_unlock(&mutexNbPacGom);

    pthread_mutex_lock(&mutexScore);
    score += 1;
    MAJScore = true;
    pthread_cond_signal(&condScore);
    pthread_mutex_unlock(&mutexScore);
  }

  if (Type == SUPERPACGOM)
  {
    //fprintstderr, "(PACMAN     -- %lu.%d) \t PACMAN vient de mangé une SUPERPACGOM \n", pthread_self(), getpid());
    pthread_mutex_lock(&mutexNbPacGom);
    nbPacGom--;
    pthread_cond_signal(&condNbPacGom);
    pthread_mutex_unlock(&mutexNbPacGom);

    pthread_mutex_lock(&mutexScore);
    score += 5;
    MAJScore = true;
    pthread_cond_signal(&condScore);
    pthread_mutex_unlock(&mutexScore);
  }
  if (Type == BONUS)
  {
    //fprintstderr, "(PACMAN     -- %lu.%d) \t PACMAN vient de mangé un BONUS \n", pthread_self(), getpid());
    pthread_mutex_lock(&mutexScore);
    score += 30;
    MAJScore = true;
    pthread_cond_signal(&condScore);
    pthread_mutex_unlock(&mutexScore);
  }
}

S_FANTOME *InitFantom(int l, int c, int cache, int couleur)
{
  S_FANTOME *fantome = (S_FANTOME *)malloc(sizeof(S_FANTOME));

  fantome->L = l; // Position initiale dans le "nid"
  fantome->C = c;
  fantome->cache = cache;     // Valeur en dessous du fantome
  fantome->couleur = couleur; // Couleur du fantome

  return fantome;
}

int CaseAleatoire(S_FANTOME *fantome)
{

  int vect[4];
  int cpt = 0;

  ////fprintstderr, "(DEBUG) DROITE  = %s\n", presence_nom(tab[fantome->L][fantome->C + 1].presence));
  ////fprintstderr, "(DEBUG) GAUCHE  = %s\n", presence_nom(tab[fantome->L][fantome->C - 1].presence));
  ////fprintstderr, "(DEBUG) BAS     = %s\n", presence_nom(tab[fantome->L + 1][fantome->C].presence));
  fprintf(stderr, "(DEBUG) HAUT    = %s\n", presence_nom(tab[fantome->L - 1][fantome->C].presence));

  if (tab[fantome->L][fantome->C + 1].presence != MUR)
  {
    vect[cpt] = DROITE;
    cpt++;
  }

  if (tab[fantome->L][fantome->C - 1].presence != MUR)
  {
    vect[cpt] = GAUCHE;
    cpt++;
  }

  if (tab[fantome->L + 1][fantome->C].presence != MUR)
  {
    vect[cpt] = BAS;
    cpt++;
  }
  if (tab[fantome->L - 1][fantome->C].presence != MUR)
  {
    vect[cpt] = HAUT;
    cpt++;
  }

  if (cpt == 0)
  {
    fprintf(stderr, "(DEBUG) Aucune case disponible, retour direction par défaut.\n");
    return -1; // Ou une direction par défaut
  }

  int choix = rand() % cpt;
  fprintf(stderr, "(DEBUG) Direction choisie:  %s\n", posiPac(vect[choix]));

  return vect[choix];
}


void set_signal_handler(int signal, void (*handler)(int))    //Prend un parametre un type de signal (sigint,sighup etc) et un pointeur de fonction
{
// Armement des signaux SIGCHLD, SIGHUP, SIGUSR1, SIGUSR2
  /*
    ? Armement des signaux
      - Création d'une structure de type sigaction.
      - Association de la structure avec le Handler(Fonction à exécuter) SIGCHLD.
      - On vide le set de la structure, permettant ainsi la réception de tous les signaux dans le handler.
      - On n'active aucune option particulière lors de la réception d'un signal.
      - On lie notre signal à notre structure sigaction.
  */

  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(signal, &sa, NULL) == -1) {
      fprintf(stderr,"(PACMAN -- %lu.%d) \t ERREUR lors de l'Armenent du signal %d\n", pthread_self(), getpid(), signal);
      exit(1);
  }
  fprintf(stderr,"(PACMAN -- %lu.%d) \t Armement du signal %d s'est bien déroulé\n", pthread_self(), getpid(), signal);
}

int MovePacMan(int newL,int newC)
{
  if (tab[newL][newC].presence != MUR)
  {
    switch (tab[newL][newC].presence)
    {
    case PACGOM:
      MangerPacGom(PACGOM);
      break;
    case SUPERPACGOM:
      MangerPacGom(SUPERPACGOM);
      break;
    case BONUS:
      MangerPacGom(BONUS);
      break;
    case FANTOME:
      if (mode == 1)
        pthread_cancel(ThreadPacMan);
      if (mode == 2)
      {
        pthread_cancel(tab[newL][newC].tid);
      }
    }

    return 1;
  }
  return 0;

}


// ? **************************************************************************************************************
// ? ***************************************FONCTION DE THREAD ****************************************************
// ? **************************************************************************************************************

void *FonctionPacMan()
{

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); // il attend avant de mourir
  pthread_cleanup_push(FonctionFinPacman, NULL);



  // Armement des signaux
  set_signal_handler(SIGCHLD, HandlerSIGCHLD);
  set_signal_handler(SIGHUP, HandlerSIGHUP);
  set_signal_handler(SIGUSR1, HandlerSIGUSR1);
  set_signal_handler(SIGUSR2, HandlerSIGUSR2);

  // On prépare un masque pour bloqué les signaux de directions pendant un certain temp
  sigset_t masque, ancien_masque;
  sigemptyset(&masque);
  sigaddset(&masque, SIGCHLD);
  sigaddset(&masque, SIGHUP);
  sigaddset(&masque, SIGUSR1);
  sigaddset(&masque, SIGUSR2);

  /*
    ? Position initiale de PACMAN on protégé bien avec un mutex 
      *On commence en 15,8 Position gauche
         ! On lock le mutex
      * on met a la case 15,8 qu'il y a un PACMAN avec notre threadId
         ! on unlock le mutex
      * On Dessine PACMAN a la case et a la directions désiré 
  */
  //fprintstderr, "(PACMAN     -- %lu.%d) \t PACMAN est bien rentré dans son thread. \n", pthread_self(), getpid());

  pthread_mutex_lock(&mutexTab);
  setTab(L, C, PACMAN, pthread_self());
  DessinePacMan(L, C, dir);
  pthread_mutex_unlock(&mutexTab);

  //fprintstderr, "(PACMAN     -- %lu.%d) \t PACMAN à été placé en (%d,%d) vers %d dans TAB \n", pthread_self(), getpid(), L, C, dir);

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

  fprintf(stderr, "(PACMAN     -- %lu.%d) \t PACMAN rentre dans la boucle d'avance auto. \n", pthread_self(), getpid());


  Attente(300);
  while (gameRunning)
  {
    //PacInfo();
    sigprocmask(SIG_BLOCK, &masque, &ancien_masque);

    pthread_mutex_lock(&mutexDelais);
    int nvxdelais = delais;
    pthread_mutex_unlock(&mutexDelais);
    Attente(nvxdelais); //Attente de 0.3s
    sigprocmask(SIG_SETMASK, &ancien_masque, NULL);


    pthread_mutex_lock(&mutexTab);
    setTab(L, C, VIDE);
    EffaceCarre(L, C);

    if (L == 9 && C == 0 && dir == GAUCHE)
      C = NB_COLONNE - 1;
    else if (L == 9 && C == 16 && dir == DROITE)
      C = 0;
    else
      switch (dir)
      {
      case GAUCHE:
        if (MovePacMan(L,C-1)) 
          C--;
        break;

      case DROITE:
        if (MovePacMan(L, C + 1))
          C++;
        break;

      case HAUT:
        if (MovePacMan(L - 1, C))
          L--;
        break;

      case BAS:
        if (MovePacMan(L + 1, C))
          L++;
        break;
      }
    pthread_mutex_unlock(&mutexTab);

    fprintf(stderr, "(PACMAN     -- %lu.%d) \t Pacman test si il est mort", pthread_self(), getpid());
    pthread_testcancel();



    if (nbPacGom > 0)
    {
      pthread_mutex_lock(&mutexTab);
      setTab(L, C, PACMAN, pthread_self());
      DessinePacMan(L, C, dir);
      pthread_mutex_unlock(&mutexTab);
    }
    else
    {
      sigprocmask(SIG_BLOCK, &masque, &ancien_masque);
      pthread_mutex_lock(&mutexNbPacGom);
      pthread_cond_wait(&condPacToutePlacer,&mutexNbPacGom);
      pthread_mutex_unlock(&mutexNbPacGom);
      sigprocmask(SIG_SETMASK, &ancien_masque, NULL);

    }

    fprintf(stderr, "(PACMAN     -- %lu.%d) \t PACMAN à été placé en (%d,%d) vers %d dans TAB \n", pthread_self(), getpid(), L, C, dir);
  }

  fprintf(stderr, "(PACMAN     -- %lu.%d) \t PACMAN à bien quitté son Thread. \n", pthread_self(), getpid());
  
  pthread_cleanup_pop(1);
  pthread_exit(NULL);




}

void *FonctionEvent()
{
  fprintf(stderr, "(EVENT      -- %lu.%d) \t EVENT est bien rentré dans son Thread\n", pthread_self(), getpid());
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
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Flèche droite !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGHUP);
        break;
      case KEY_LEFT:
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Flèche gauche !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGCHLD);
        break;
      case KEY_UP:
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Flèche De haut !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGUSR1);
        break;
      case KEY_DOWN:
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Flèche De bas !\n", pthread_self(), getpid());
        pthread_kill(ThreadPacMan, SIGUSR2);
        break;
      case KEY_P:
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Touche +  !\n", pthread_self(), getpid());
        pthread_mutex_lock(&mutexDelais);
        delais = delais - 20;
        pthread_mutex_unlock(&mutexDelais);
        break;
      case KEY_M:
        fprintf(stderr, "(EVENT      -- %lu.%d) \t - Touche -  !\n", pthread_self(), getpid());
        pthread_mutex_lock(&mutexDelais);
        delais = delais + 20;
        pthread_mutex_unlock(&mutexDelais);
      }
    }
  }

  fprintf(stderr, "(EVENT      -- %lu.%d) \t Clique sur le bouton Croix \n", pthread_self(), getpid());
  fprintf(stderr, "(EVENT      -- %lu.%d) \t EVENT à bien quitté son Thread \n", pthread_self(), getpid());
  gameRunning = false;
  pthread_exit(NULL);
}

void *FonctionPacGom()
{

  // Mise en place des PACGOM ET SUPERPACGOM

  /*  
        ? Boucle de placement de PACGOM et de SUPERPACGOM
          * On place toute les PACGOM et SUPERPACGOM

      */

  while (gameRunning)
  {

    NiveauJeu++;
    pthread_mutex_lock(&mutexTab);
    DessineChiffre(14, 22, NiveauJeu);
    pthread_mutex_unlock(&mutexTab);

    PlacerPacGom();

    L = 15;
    C = 8;
    pthread_mutex_lock(&mutexDir);
    dir = GAUCHE;
    pthread_mutex_unlock(&mutexDir);
    MangerPacGom(PACGOM);

  pthread_cond_signal(&condPacToutePlacer);



    fprintf(stderr, "(PACMAN     -- %lu.%d) \t PACMAN à été placé en (%d,%d) vers %d dans TAB \n", pthread_self(), getpid(), L, C, dir);
    
    pthread_mutex_lock(&mutexNbPacGom);
    while (nbPacGom > 0)
    {
      pthread_cond_wait(&condNbPacGom, &mutexNbPacGom);
      fprintf(stderr, "(PACGOM     -- %lu.%d) \t La condition a été activée \n", pthread_self(), getpid());

      pthread_mutex_lock(&mutexTab);
      DessineChiffre(12, 24, (nbPacGom % 10));
      DessineChiffre(12, 23, ((nbPacGom / 10) % 10));
      DessineChiffre(12, 22, ((nbPacGom / 100) % 10));
      pthread_mutex_unlock(&mutexTab);

      fprintf(stderr, "(PACGOM     -- %lu.%d) \t FIn de la condition \n", pthread_self(), getpid());
    }
    pthread_mutex_unlock(&mutexNbPacGom);


    fprintf(stderr, "(PACGOM     -- %lu.%d) \t Toute les PACGOM on été récupéré \n", pthread_self(), getpid());

    pthread_mutex_lock(&mutexTab);
    EffaceCarre(L, C);
    setTab(L, C, VIDE);
    pthread_mutex_unlock(&mutexTab);

    for (int i = 0; i < 8; i++)
    {
      fprintf(stderr, "(PACGOM     -- %lu.%d) \t Le thread FANTOME %d à bien été terminé \n", pthread_self(), getpid(), i);
      pthread_cancel(ThreadFantome[i]);

    }
    // On met un nouveau delais

    pthread_mutex_lock(&mutexDelais);
    delais /= 2;
    fprintf(stderr, "(PACGOM     -- %lu.%d) \t Nouveau délais : %d \n", pthread_self(), getpid(), delais);
    pthread_mutex_unlock(&mutexDelais);
  }

  pthread_exit(NULL);
}

void *FonctionScore()
{
  fprintf(stderr, "(SCORE      -- %lu.%d) \t Vous êtes bien rentrés dans le Thread score\n", pthread_self(), getpid());

  while (gameRunning)
  {
    pthread_mutex_lock(&mutexScore); // Verrouille le mutex avant d'attendre
    fprintf(stderr, "(SCORE      -- %lu.%d) \t Attente d'une Mise à jour du score\n", pthread_self(), getpid());
    while (MAJScore == false)
    {
      pthread_cond_wait(&condScore, &mutexScore); // Attend la mise à jour du score
    }

    fprintf(stderr, "(SCORE      -- %lu.%d) \t Une Mise a jour a été effectué\n", pthread_self(), getpid());
    //Affichage du score

    DessineChiffre(16, 25, (score % 10));
    DessineChiffre(16, 24, (score / 10 % 10));
    DessineChiffre(16, 23, (score / 100 % 10));
    DessineChiffre(16, 22, (score / 1000 % 10));

    MAJScore = false;                  // Réinitialisation de la variable
    pthread_mutex_unlock(&mutexScore); // Déverrouille après modification
  }

  fprintf(stderr, "Le score a bien été incrémenté\n");

  pthread_exit(NULL);
}

void *FonctionBonus()
{
  fprintf(stderr, "(BONUS      -- %lu.%d) \t Vous êtes bien rentrés dans le Thread Bonus \n", pthread_self(), getpid());

  int alea;
  int choix;
  while (gameRunning)
  {
    int cpt = 0;
    int VecVide[150][2] = {}; // Stocke les cases vides
    alea = rand() % 10 + 10;

    fprintf(stderr, "(BONUS      -- %lu.%d) \t Un bonus apparaitra dans %d secondes \n", pthread_self(), getpid(), alea);

    Attente(alea * 1000);

    fprintf(stderr, "(BONUS      -- %lu.%d) \t Fin d'attente de %d secondes \n", pthread_self(), getpid(), alea);

    // Recherche des cases vides
    for (int i = 0; i < NB_LIGNE; i++)
      for (int j = 0; j < NB_COLONNE; j++)
        if (tab[i][j].presence == VIDE)
        {
          if (cpt < 150)
          { // Empêcher le dépassement de VecVide
            VecVide[cpt][0] = i;
            VecVide[cpt][1] = j;
            cpt++;
          }
        }

    fprintf(stderr, "(BONUS      -- %lu.%d) \t Voici votre vecteur de cases vides\n", pthread_self(), getpid());

    // Affichage des cases vides
    for (int i = 0; i < cpt; i++)
      fprintf(stderr, "(BONUS      -- %lu.%d) \t   -   %d    =    tab[%d,%d] \n", pthread_self(), getpid(), i, VecVide[i][0], VecVide[i][1]);

    // Placer un bonus aléatoire dans l'une des cases vides
    if (cpt > 0)
    {
      int choix = rand() % cpt; // Prendre un indice aléatoire parmi les cases vides
      pthread_mutex_lock(&mutexTab);
      setTab(VecVide[choix][0], VecVide[choix][1], BONUS);
      DessineBonus(VecVide[choix][0], VecVide[choix][1]);
      pthread_mutex_unlock(&mutexTab);

      Attente(10 * 1000);

      if (tab[VecVide[choix][0]][VecVide[choix][1]].presence == BONUS)
      {
        pthread_mutex_lock(&mutexTab);
        EffaceCarre(VecVide[choix][0], VecVide[choix][1]);
        setTab(VecVide[choix][0], VecVide[choix][1], VIDE);
        pthread_mutex_unlock(&mutexTab);
      }
    }
    else
    {
      fprintf(stderr, "(BONUS      -- %lu.%d) \t Aucune case vide disponible pour placer un bonus\n", pthread_self(), getpid());
    }
  }
  pthread_exit(NULL);
}

void *FonctionCompteurFantome()
{
  fprintf(stderr, "(CPTFANTOME -- %lu.%d) \t Vous êtes bien rentrés dans le Thread Compteur de Fantome \n", pthread_self(), getpid());

  int couleurs[8] = {ROUGE, ROUGE, VERT, VERT, ORANGE, ORANGE, MAUVE, MAUVE};

  for (int i = 0; i < 8; i++)
  {

    S_FANTOME *fantome = InitFantom(9, 8, 0, couleurs[i]);

    fprintf(stderr, "(CPTFANTOME -- %lu.%d) \t Fantôme de couleur %d créé à la position (%d, %d)\n", pthread_self(), getpid(), fantome->couleur, fantome->L, fantome->C);
    pthread_create(&ThreadFantome[i], NULL, (void *(*)(void *))FonctionFantome, (void *)fantome);
    Attente(delais);
  }

  while (gameRunning)
  {
    while (nbRouge == 2 && nbVert == 2 && nbOrange == 2 && nbMauve == 2)
    {
      pthread_mutex_lock(&mutexNbFantomes); // Verrouille le mutex avant d'attendre
      pthread_cond_wait(&condNbFantomes, &mutexNbFantomes);
      fprintf(stderr,"(CPTFANTOME -- %lu.%d) \t J'ai bien été reveillé a la mort d'un fantome \n");
      pthread_mutex_unlock(&mutexNbFantomes); // Verrouille le mutex avant d'attendre
    }

    if (nbRouge < 2)
    {
      S_FANTOME *fantome = InitFantom(9, 8, 0, ROUGE);
      pthread_create(&ThreadFantome[7], NULL, (void *(*)(void *))FonctionFantome, (void *)fantome);
      pthread_mutex_lock(&mutexNbRouge);
      nbRouge++;
      pthread_mutex_unlock(&mutexNbRouge);
    }
    if (nbVert < 2)
    {
      S_FANTOME *fantome = InitFantom(9, 8, 0, VERT);
      pthread_create(&ThreadFantome[7], NULL, (void *(*)(void *))FonctionFantome, (void *)fantome);
      pthread_mutex_lock(&mutexNbVert);
      nbVert++;
      pthread_mutex_unlock(&mutexNbVert);
    }
    if (nbOrange < 2)
    {
      S_FANTOME *fantome = InitFantom(9, 8, 0, ORANGE);
      pthread_create(&ThreadFantome[7], NULL, (void *(*)(void *))FonctionFantome, (void *)fantome);
      pthread_mutex_lock(&mutexNbOrange);
      nbOrange++;
      pthread_mutex_unlock(&mutexNbOrange);
    }
    if (nbMauve < 2)
    {
      S_FANTOME *fantome = InitFantom(9, 8, 0, MAUVE);
      pthread_create(&ThreadFantome[7], NULL, (void *(*)(void *))FonctionFantome, (void *)fantome);
      pthread_mutex_lock(&mutexNbMauve);
      nbMauve++;
      pthread_mutex_unlock(&mutexNbMauve);
    }
    fprintf(stderr,"(CPTFANTOME -- %lu.%d) \t J'attend 4sec \n");
    
    sleep(4);
    fprintf(stderr,"(CPTFANTOME -- %lu.%d) \t J'ai fini d'attendre 4sec \n");
  }


  pthread_exit(NULL);
}

void *FonctionFantome(void *arg)
{
  fprintf(stderr, "(FANTOME   -- %lu.%d) \t Vous êtes bien rentrés dans le Thread du Fantome \n", pthread_self(), getpid());

  S_FANTOME *fantome = (S_FANTOME *)arg;

  pthread_mutex_lock(&mutexNbFantomes);
  nbFantome++;
  pthread_mutex_unlock(&mutexNbFantomes);

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); // il attend avant de mourir
  pthread_cleanup_push(FonctionFinFantome, fantome);

  int dirFantome = HAUT;

  fprintf(stderr, "(FANTOME   -- %lu.%d) \t Fantôme de couleur %d créé à la position (%d, %d) et en direction de %s\n", pthread_self(), getpid(), fantome->couleur, fantome->L, fantome->C, posiPac(dirFantome));
  pthread_setspecific(cle, fantome);

  pthread_mutex_lock(&mutexDelais);
  int delaisFantome = (2 * delais * 5) / 3;
  pthread_mutex_unlock(&mutexDelais);



  while (true)
  {
    
    pthread_mutex_lock(&mutexTab);
    setTab(fantome->L, fantome->C, FANTOME, pthread_self());
    DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);
    pthread_mutex_unlock(&mutexTab);

    Attente(delaisFantome);


    pthread_mutex_lock(&mutexTab);
    setTab(fantome->L, fantome->C, fantome->cache); //On remet l'ancienne case pour pas bouffer les PACGOM
    EffaceCarre(fantome->L, fantome->C);
    pthread_mutex_unlock(&mutexTab);


    pthread_testcancel();

    pthread_mutex_lock(&mutexTab);
    switch (fantome->cache)
    {
    case PACGOM:
      DessinePacGom(fantome->L, fantome->C);
      break;
    case SUPERPACGOM:
      DessineSuperPacGom(fantome->L, fantome->C);
      break;
    case BONUS:
      DessineBonus(fantome->L, fantome->C);
      break;
    }
    pthread_mutex_unlock(&mutexTab);


    switch (dirFantome)
    {

    case (GAUCHE):
      if (tab[fantome->L][fantome->C - 1].presence != MUR && tab[fantome->L][fantome->C - 1].presence != FANTOME)
      {
        fantome->cache = tab[fantome->L][fantome->C - 1].presence; //On sauvegarde la case d'après
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t On avait dans la case tab[%d][%d] un %s \n", pthread_self(), getpid(), fantome->L, fantome->C, presence_nom(tab[fantome->L][fantome->C - 1].presence));
        fantome->C--;
      }
      else
      {
        dirFantome = CaseAleatoire(fantome);
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t Nouvelle direction %d - %s \n", pthread_self(), getpid(), dirFantome, posiPac(dirFantome));
      }
      break;


    case (DROITE):
      if (tab[fantome->L][fantome->C + 1].presence != MUR && tab[fantome->L][fantome->C + 1].presence != FANTOME)
      {
        fantome->cache = tab[fantome->L][fantome->C + 1].presence; //On sauvegarde la case d'après
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t On avait dans la case tab[%d][%d] un %s \n", pthread_self(), getpid(), fantome->L, fantome->C + 1, presence_nom(tab[fantome->L][fantome->C + 1].presence));

        fantome->C++;
      }
      else
      {
        dirFantome = CaseAleatoire(fantome);
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t Nouvelle direction %d - %s \n", pthread_self(), getpid(), dirFantome, posiPac(dirFantome));
      }
      break;


    case (HAUT):
      if (tab[fantome->L - 1][fantome->C].presence != MUR && tab[fantome->L - 1][fantome->C].presence != FANTOME)
      {
        fantome->cache = tab[fantome->L - 1][fantome->C].presence; //On sauvegarde la case d'après
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t On avait dans la case tab[%d][%d] un %s \n", pthread_self(), getpid(), fantome->L - 1, fantome->C, presence_nom(tab[fantome->L - 1][fantome->C].presence));

        fantome->L--;
      }
      else
      {
        dirFantome = CaseAleatoire(fantome);
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t Nouvelle direction %d - %s \n", pthread_self(), getpid(), dirFantome, posiPac(dirFantome));
      }
      break;


    case (BAS):
      if (tab[fantome->L + 1][fantome->C].presence != MUR && tab[fantome->L + 1][fantome->C].presence != FANTOME)
      {
        fantome->cache = tab[fantome->L + 1][fantome->C].presence; //On sauvegarde la case d'après
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t On avait dans la case tab[%d][%d] un %s \n", pthread_self(), getpid(), fantome->L + 1, fantome->C, presence_nom(tab[fantome->L + 1][fantome->C].presence));
        fantome->L++;
      }
      else
      {
        dirFantome = CaseAleatoire(fantome);
        fprintf(stderr, "(FANTOME   -- %lu.%d) \t Nouvelle direction %d - %s \n", pthread_self(), getpid(), dirFantome, posiPac(dirFantome));
      }
    }
  }

    pthread_cleanup_pop(0);
    pthread_exit(NULL);
}


// ? **************************************************************************************************************
// ? *************************************** FONCTION DE FIN **************************************************************
// ? **************************************************************************************************************

void FonctionFinFantome(void *param)
{
  S_FANTOME *fantome = (S_FANTOME *)param;
  fprintf(stderr, "\n (FINFANTOME) Donnée recu \n L = %d, \n C = %d, \n Couleur = %d, \n cache = %s \n", fantome->L, fantome->C, fantome->couleur, presence_nom(fantome->cache));




  pthread_mutex_lock(&mutexNbPacGom);

  switch (fantome->cache)
  {
  case VIDE:
    fprintf(stderr, "(FINFANTOME) Le fantome est mort sur une caseVide\n");
    break;
  case PACGOM:
    nbPacGom--;
    fprintf(stderr, "(FINFANTOME) Une Pacgom a été décrementé \n");
    break;
  case SUPERPACGOM:
    fprintf(stderr, "(FINFANTOME) Une SUPERPACGOM a été décrementé\n");
    nbPacGom = nbPacGom - 5;
    break;
  }
  pthread_mutex_unlock(&mutexNbPacGom);


  switch (fantome->couleur)
  {
  case MAUVE:
    nbMauve--;
    break;
  case ORANGE:
    nbOrange--;
    break;
  case VERT:
    nbVert--;
    break;
  case ROUGE:
    nbRouge--;
    break;
  }

  fprintf(stderr,"(FINFANTOME) On a bien décrementé la couleurs %d\n",fantome->couleur);

  fprintf(stderr,"(DEBUG)  NbFantome avant = %d\n",nbFantome);
  pthread_mutex_lock(&mutexNbFantomes);
  nbFantome--;
  pthread_mutex_unlock(&mutexNbFantomes);
  fprintf(stderr,"(DEBUG)  NbFantome après = %d\n",nbFantome);



  pthread_cond_signal(&condNbFantomes);

}

void FonctionFinPacman(void *)
{

  fprintf(stderr, "(FINPACMAN) PACMAN EST MORT  \n");
  pthread_mutex_lock(&mutexTab);
  setTab(L, C, VIDE);
  EffaceCarre(L, C);
  pthread_mutex_unlock(&mutexTab);

   if (pthread_mutex_trylock(&mutexDelais) == 0)
     {
        fprintf(stderr, "(PACMAN -- %lu) 🔓 J'ai réussi à lock mutexDelais avec trylock !\n", pthread_self());

    } else {
        fprintf(stderr, "(PACMAN -- %lu) 🚨 mutexDelais est déjà verrouillé par un autre thread !\n", pthread_self());
        pthread_mutex_unlock(&mutexDelais);
    }
  
}

// ? **************************************************************************************************************
// ? *************************************** SIGNAUX **************************************************************
// ? **************************************************************************************************************

void HandlerSIGCHLD(int sig)
{
  fprintf(stderr, "\n(SIGCHLD     -- %lu.%d) \t Vous etez bien entrer dans le SIGCHLD PACMAN VA A GAUCHE\n", pthread_self(), getpid());
  pthread_mutex_lock(&mutexDir);
  dir = GAUCHE;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGHUP(int sig)
{
  fprintf(stderr, "\n(SIGHUP     -- %lu.%d) \t Vous etez bien entrer dans le SIGHUP PACMAN VA A DROITE\n", pthread_self(), getpid());
  pthread_mutex_lock(&mutexDir);
  dir = DROITE;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGUSR1(int sig)
{
  fprintf(stderr, "\n(SIGUSR1    -- %lu.%d) \t Vous etez bien entrer dans le SIGUSR1 PACMAN VA EN HAUT\n", pthread_self(), getpid());
  pthread_mutex_lock(&mutexDir);
  dir = HAUT;
  pthread_mutex_unlock(&mutexDir);
}

void HandlerSIGUSR2(int sig)
{
  fprintf(stderr, "\n(SIGSIGURS2     -- %lu.%d) \t Vous etez bien entrer dans le SIGUSR1 PACMAN VA EN BAS\n", pthread_self(), getpid());
  pthread_mutex_lock(&mutexDir);
  dir = BAS;
  pthread_mutex_unlock(&mutexDir);
}

