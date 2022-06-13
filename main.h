#ifndef GLOBAL_H
#define GLOBAL_H
#define _GNU_SOURCE
#include "process_queue.h"

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2
#define LOOSE_TIME 5

#define N_SALKA 1

#define ROOT 0

extern pthread_mutex_t stateMut;
extern pthread_mutex_t zegarMut;

/* stany procesu */
typedef enum {READY, REST, QUEUE, QUEUE_WAIT, GATHERING, LOCATING, FIGHT} state_t;
extern state_t stan;
extern int rank;
extern int size;
extern int zegar;

/* zmienne procesu */

extern process_queue_t waitQueueZ;
extern int ackCounterZ;
extern process_queue_t waitQueueS;
extern int ackCounterS;

extern int ackZPriority;
extern int ackSPriority;
extern int jestem;
extern int rezSalke;
extern int strength;
extern int opponentStrength;
extern int przeciwnik;
extern int opponentReady;
extern process_queue_t processQueue;
extern int pairCounter;

typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */
    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

/* Typy wiadomości */
#define REQ_SALKA 1
#define ACK_SALKA 2
#define REQ_GATHERING 3
#define ACK_GATHERING 4
#define PAIR_SZUKAM 5
#define PAIR_SYNC 6
#define PAIR_JESTEM 7
#define FIGHT_READY 8

#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d]:[%d] " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, zegar, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d]:[%d] " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, zegar, ##__VA_ARGS__, 27,0,37);


/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);
void changeState( state_t, const char * name, void(*fun)());

#endif
