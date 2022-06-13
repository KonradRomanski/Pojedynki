#ifndef WATEK_KOMUNIKACYJNY_H
#define WATEK_KOMUNIKACYJNY_H
#include "process_queue.h"

/* wątek komunikacyjny: odbieranie wiadomości i reagowanie na nie poprzez zmiany stanu */
void *startKomWatek(void *ptr);

void onStartLocating();
void onStartFight();
void onStartQueueWait();
void onStartGathering();
void onStartResting();
void onStartReady();
void case_PAIR_SZUKAM(packet_t pakiet);
void case_PAIR_SYNC(packet_t pakiet);
void case_PAIR_JESTEM(packet_t pakiet);
void case_REQ_SALKA(packet_t pakiet);
void case_ACK_SALKA(packet_t pakiet);
void case_FIGHT_READY(packet_t pakiet);

void reqZ(packet_t pakiet, int str, int msgT);
void ackZ(packet_t pakiet, int str, int N);


/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
  MPI_Status status;
  int is_message = FALSE;
  packet_t pakiet;
  while (TRUE)
  {
    MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    pthread_mutex_lock(&zegarMut);
    if (zegar > pakiet.ts)
      zegar++;
    else
      zegar = pakiet.ts + 1;
    pthread_mutex_unlock(&zegarMut);
    pthread_mutex_lock(&stateMut);

    switch (status.MPI_TAG)
    {
      case PAIR_SZUKAM:
        debug("Otrzymałem PAIR_SZUKAM od %d ", pakiet.src);
        case_PAIR_SZUKAM(pakiet);
        break;
      case PAIR_SYNC:
        debug("Otrzymałem PAIR_SYNC od %d ", pakiet.src);
        case_PAIR_SYNC(pakiet);
        break;
      case PAIR_JESTEM:
        debug("Otrzymałem PAIR_JESTEM od %d ", pakiet.src);
        case_PAIR_JESTEM(pakiet);
        break;
      case REQ_SALKA:
        debug("Otrzymałem REQ_SALKA od %d", pakiet.src);
        case_REQ_SALKA(pakiet);
        break;
      case ACK_SALKA:
        debug("Otrzymałem ACK_SALKA od %d ", pakiet.src);
        case_ACK_SALKA(pakiet);
        break;
      case FIGHT_READY:
        debug("Otrzymałem FIGHT_READY od %d", pakiet.src);
        case_FIGHT_READY(pakiet);
        break;
      default:
        break;
    }
    pthread_mutex_unlock(&stateMut);
  }
}

void case_PAIR_SZUKAM(packet_t pakiet)
{
  queue_element_t el;
  el.priority = pakiet.data;
  el.process = pakiet.src;
  insertElement(&processQueue, el);
  if (size >= 4)
    debug("Moja kolejka procesów - początek: [%d, %d, %d, %d, ...",
          processQueue.data[0].process, processQueue.data[1].process, processQueue.data[2].process, processQueue.data[3].process);
  // sendPacket(0, pakiet.src, PAIR_SYNC);
}

void case_PAIR_SYNC(packet_t pakiet)
{
  if (stan == QUEUE)
  {
    pairCounter++;
    if (pairCounter == size - 1)
    {
      pairCounter = 0;
      int mPos = findProcess(&processQueue, rank);
      int d_size = sizeof(processQueue.data);
      //printf("mpos %d", mPos);
      //printf("size %d tab %d %d %d %d %d", d_size, processQueue.data[0].process, processQueue.data[1].process, processQueue.data[2].process, processQueue.data[3].process, processQueue.data[4].process);
      if (mPos % 2 == 1)
      {
        przeciwnik = processQueue.data[mPos - 1].process;
        debug("Moim przeciwnikiem jest %d", przeciwnik);
        removeNFirstElements(&processQueue, mPos + 1);
        sendPacket(0, pakiet.src, PAIR_SYNC);
        changeState(GATHERING, "GATHERING", onStartGathering);
      }
      else
      {
        if (jestem)
        {
          jestem = FALSE;
          przeciwnik = processQueue.data[mPos + 1].process;
          debug("Moim przeciwnikiem jest %d", przeciwnik);
          removeNFirstElements(&processQueue, mPos + 2);
          changeState(GATHERING, "GATHERING", onStartGathering);
          sendPacket(0, przeciwnik, PAIR_JESTEM);
          // sendPacket(0, pakiet.src, PAIR_SYNC);

          // changeState(FIGHT, "FIGHT", onStartFight);

        }
        else
        {
          removeNFirstElements(&processQueue, mPos);
          changeState(QUEUE_WAIT, "QUEUE_WAIT", onStartQueueWait);
        }
      }
    }
  }
}

void case_PAIR_JESTEM(packet_t pakiet)
{
  if (stan == QUEUE_WAIT)
  {
    przeciwnik = pakiet.src;
    debug("Moim przeciwnikiem jest %d", przeciwnik);
    removeProcess(&processQueue, rank);
    removeProcess(&processQueue, przeciwnik);
    changeState(FIGHT, "FIGHT", onStartFight);

  }
  else if (stan == QUEUE)
  {
    jestem = TRUE;
  }
}


void case_REQ_SALKA(packet_t pakiet)
{
  queue_element_t el;
  if ((stan == LOCATING && pakiet.data > ackSPriority) || (stan == LOCATING && pakiet.data == ackSPriority) && rank < pakiet.src)
  {
    el.priority = pakiet.data;
    el.process = pakiet.src;
    insertElement(&waitQueueS, el);
  }
  else
  {
    packet_t *pkt = malloc(sizeof(packet_t));
    pkt->data = pakiet.data;
    sendPacket(pkt, pakiet.src, ACK_SALKA);
  }
}

void case_ACK_SALKA(packet_t pakiet)
{
  if (stan == LOCATING && pakiet.data == ackSPriority)
  {
    ackCounterS++;
    if (ackCounterS == size - N_SALKA)
    {
      ackCounterS = 0;
      changeState(REST, "REST", onStartResting);
    }
  }
}

void case_FIGHT_READY(packet_t pakiet)
{
  opponentReady = TRUE;
}

void onStartLocating()
{
  rezSalke = TRUE;
  ackCounterS = 0;
  int pr = zegar;
  for (int i = 0; i < size; i++)
  {
    if (i != rank)
    {
      packet_t *pkt = malloc(sizeof(packet_t));
      pkt->data = pr;
      sendPacket(pkt, i, REQ_SALKA);
    }
  }
  ackSPriority = pr;
}

void onStartGathering()
{
  strength = rand() % 100;
  int msgType;
  ackCounterZ = 0;
  debug("Moja siła: %d*", strength);
  int pr = zegar;
  for (int i = 0; i < size; i++)
  {
    if (i != rank)
    {
      packet_t *pkt = malloc(sizeof(packet_t));
      pkt->data = pr;
      sendPacket(pkt, i, msgType);
    }
  }
  ackZPriority = pr;
}

void onStartResting()
{
  sleep( LOOSE_TIME );
  changeState(READY, "READY", onStartReady);
}

void onStartFight()
{
  packet_t *pkt = malloc(sizeof(packet_t));
  sendPacket(0, przeciwnik, FIGHT_READY);
}

void onStartQueueWait()
{
}

void onStartReady()
{
  removeNFirstElements(&waitQueueZ, waitQueueZ.size);
  for(int i = 0; i < waitQueueS.size; i++)
  {
    packet_t * pkt = malloc(sizeof(packet_t));
    pkt->data = waitQueueS.data[i].priority;
    sendPacket( pkt, waitQueueS.data[i].process, ACK_SALKA);
  }
  removeNFirstElements(&waitQueueS, waitQueueS.size);
  opponentReady = FALSE;
  rezSalke = FALSE;
}

#endif
