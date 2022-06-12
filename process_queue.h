#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/* boolean */
#define TRUE 1
#define FALSE 0

typedef struct {
    int process;
    int priority;
} queue_element_t;

typedef struct{
    queue_element_t * data;
    int size;
    int reserved;
} process_queue_t;


void insertElement(process_queue_t * process_queue, queue_element_t el)
{
  if(process_queue->size == process_queue->reserved)
    reserveQueue(process_queue, process_queue->reserved*2);

  int i = 0;
  while(i < process_queue->size)
  {
    if(moreImportant(el, process_queue->data[i]))
      break;
    i++;
  }

  int j = process_queue->size;
  while(j > i)
  {
    process_queue->data[j] = process_queue->data[j-1];
    j--;
  }
  process_queue->data[i] = el;
  process_queue->size++;

}
void removeProcess(process_queue_t * process_queue, int process)
{
  int i = 0;
  while(i < process_queue->size)
  {
    if(process_queue->data[i].process == process)
      break;
    i++;
  }
  while(i < process_queue->size - 1)
  {
    process_queue->data[i] = process_queue->data[i+1];
    i++;
  }
  process_queue->data[process_queue->size - 1].process = -1;
  process_queue->data[process_queue->size - 1].priority = -1;
  process_queue->size--;
}
void removeNFirstElements(process_queue_t * process_queue, int N)
{
  if(N > process_queue->size)
    N = process_queue->size;
  int i = 0;
  while(i < process_queue->size - N)
  {
    process_queue->data[i] = process_queue->data[i+N];
    i++;
  }
  while(i<process_queue->size)
  {
    process_queue->data[i].priority = -1;
    process_queue->data[i].process = -1;
    i++;
  }
  process_queue->size = process_queue->size - N;
}

int findProcess(process_queue_t * process_queue, int process)
{
  int res = -1;
  for(int i = 0; i < process_queue->size; i++)
  {
    if(process_queue->data[i].process == process)
    {
      res = i;
      break;
    }
  }
  return res;
}

void reserveQueue(process_queue_t * process_queue, int reserved)
{
  queue_element_t * ptr = (queue_element_t *)(malloc(sizeof(queue_element_t)*reserved));
  for(int i = 0; i < process_queue->size; i++)
    ptr[i] = process_queue->data[i];

  free(process_queue->data);
  process_queue->data = ptr;
  process_queue->reserved = reserved;
}

void initQueue(process_queue_t * process_queue, int initialReserved)
{
  process_queue->reserved = 0;
  process_queue->size = 0;
  reserveQueue(process_queue, initialReserved);
}

void freeQueue(process_queue_t * process_queue)
{
  process_queue->reserved = 0;
  process_queue->size = 0;
  free(process_queue->data);
}

int moreImportant(queue_element_t elementA, queue_element_t elementB)
{
  if(elementA.process == -1)
    return FALSE;
  if(elementB.process == -1)
    return TRUE;

  if(elementA.priority < elementB.priority)
    return TRUE;
  if(elementA.priority == elementB.priority)
    if(elementA.process < elementB.process)
      return TRUE;
  return FALSE;
}
