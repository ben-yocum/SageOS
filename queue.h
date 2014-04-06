#include "os345.h"



void pq_test();
int pq_assert(char* message, PQueue* q, int success);
int peek(PQueue* q);
int dequeue(PQueue *q, int tid);
int enqueue(PQueue *q, int tid, int priority);
void pq_destruct(PQueue* q);
PQueue* pq_construct(int size);
void pq_print(PQueue* q);
void fair_dist_time(PQueue* q, int total_time);
