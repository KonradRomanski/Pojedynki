#ifndef WATEK_GLOWNY_H
#define WATEK_GLOWNY_H
#include "process_queue.h"


/* pętla główna aplikacji: zmiany stanów itd */
void mainLoop();
void onStartGathering();
void onStartQueueing();
// void onStartResting();
void onStartReady();
int calculateWinner();


void mainLoop()
{
  srandom(rank);
  while (TRUE) {
    if(stan == READY)
    {
      sleep( SEC_IN_STATE);
      int perc = random()%100;
      pthread_mutex_lock( &stateMut );
      if (perc<STATE_CHANGE_PROB)
      {
        changeState(QUEUE, "QUEUE", onStartQueueing);
        if(size >= 4)
          debug("\nMoja kolejka procesów - początek: [%d, %d, %d, %d, ... ",
                processQueue.data[0].process, processQueue.data[1].process, processQueue.data[2].process, processQueue.data[3].process);
      }

      pthread_mutex_unlock( &stateMut );
    }
    else if(stan == FIGHT)
    {
      if(opponentReady)
      {
        debug("\nRozpoczynam walkę z %d ", przeciwnik);
        sleep( SEC_IN_STATE);
        pthread_mutex_lock( &stateMut );
        if(calculateWinner())
        {
          debug("\nKończę walkę z %d. WYGRAŁEM ", przeciwnik);
          changeState(READY, "READY", onStartReady);
          pthread_mutex_unlock( &stateMut );
        }
        else
        {
          debug("\nKończę walkę z %d. PRZEGRAŁEM ", przeciwnik);
          changeState(LOCATING, "LOCATING", onStartLocating);
          // changeState(REST, "REST", onStartResting);
          pthread_mutex_unlock( &stateMut );
        }
      }
    }
  }
}

void onStartQueueing()
{
  int pr = zegar;
  for(int i = 0; i < size; i++)
  {
    if(i!=rank)
    {
      packet_t * pkt = malloc(sizeof(packet_t));
      pkt->data = pr;
      sendPacket( pkt, i, PAIR_SZUKAM);
    }
  }

  queue_element_t el;
  el.priority = pr;
  el.process = rank;
  insertElement(&processQueue, el);
  pairCounter = 0;
}


void onStartReadyW()
{
  removeNFirstElements(&waitQueueZ, waitQueueZ.size);
  for(int i = 0; i < waitQueueS.size; i++)
  {
    packet_t * pkt = malloc(sizeof(packet_t));
    pkt->data = waitQueueS.data[i].priority;
  }
  removeNFirstElements(&waitQueueS, waitQueueS.size);
  opponentReady = FALSE;
  rezSalke = FALSE;
}

int calculateWinner()
{

  if(strength >= opponentStrength)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}



#endif
