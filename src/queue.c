#include "queue.h"
#include "utils.h"

struct Queue *initQueue() {
    struct Queue* q = (struct Queue*)simple_malloc((unsigned long)sizeof(struct Queue));
    q->front = 0;
    q->rear = 0;
    q->size = MAX_SIZE;
    return q;
}

int isFull(struct Queue *queue) {
    return queue->front == (queue->rear + 1) % MAX_SIZE;
}

int isEmpty(struct Queue *queue) {
    return queue->front == queue->rear;
}

void pushQueue(struct Queue *queue, char item) {
    if(isFull(queue)) return;
    else {
        queue->buffer[queue->rear] = item;
        queue->rear = (queue->rear + 1) % MAX_SIZE;
    }
}

char popQueue(struct Queue *queue) {
    char item;
    if(isEmpty(queue)) return '\0';
    else {
        item = queue->buffer[queue->front];
        if(queue->front == queue->rear) {
            queue->front = 0;
            queue->rear = 0;
        }
        else {
            queue->front = (queue->front + 1) % MAX_SIZE;
        }
        return item;
    }
}


