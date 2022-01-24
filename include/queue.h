#ifndef __QUEUE_H__
#define __QUEUE_H__

#define MAX_SIZE 1024
struct Queue
{
    int front;
    int rear;
    int size;
    char buffer[MAX_SIZE];
};

void initQueue(struct Queue *);
int isFull(struct Queue *);
int isEmpty(struct Queue *);
void pushQueue(struct Queue *, char);
char popQueue(struct Queue *);

#endif