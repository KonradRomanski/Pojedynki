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


#include <mpi.h>
#include "process_queue.h"
#include "main.h"
#include "watek_komunikacyjny.h"
#include "watek_glowny.h"
/* wątki */
#include <pthread.h>

state_t stan = READY;
int strength, opponentStrength;
int size, rank, zegar, ackCounterZ, ackCounterS, przeciwnik, opponentReady, pairCounter, ackZPriority, ackSPriority, jestem, rezSalke; /* nie trzeba zerować, bo zmienna globalna statyczna */
process_queue_t waitQueueS, waitQueueZ, processQueue;

MPI_Datatype MPI_PAKIET_T;
volatile char end = FALSE;

pthread_t threadKom;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zegarMut = PTHREAD_MUTEX_INITIALIZER;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

/* srprawdza, czy są wątki, tworzy typ MPI_PAKIET_T
*/
void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems = 3; /* bo packet_t ma trzy pola */
    int       blocklengths[3] = {1,1,1};
    MPI_Datatype typy[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[3]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    initQueue(&waitQueueS, size);
    initQueue(&waitQueueZ, size);
    initQueue(&processQueue, size);

    pthread_create( &threadKom, NULL, startKomWatek , 0);
    debug("Zostałem zainicjalizowany");
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    pthread_mutex_destroy( &zegarMut);
    /* Czekamy, aż wątek potomny się zakończy */
    debug("czekam na wątek \"komunikacyjny\"" );
    pthread_join(threadKom,NULL);
    freeQueue(&waitQueueZ);
    freeQueue(&waitQueueS);
    freeQueue(&processQueue);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}


/* opis patrz main.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;
    pthread_mutex_lock(&zegarMut);
    pkt->ts = ++zegar;
    pthread_mutex_unlock(&zegarMut);
    //sleep(1);
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt) free(pkt);
}

void changeState( state_t newState, const char * name, void(*fun)())
{ 
    stan = newState;
    debug("Zmieniam stan na {%s}", name);
    fun();
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc,&argv); // tworzy wątek komunikacyjny w "watek_komunikacyjny.c"
    mainLoop();          // w pliku "watek_glowny.c"
    finalizuj();
    return 0;
}

