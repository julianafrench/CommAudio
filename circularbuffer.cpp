#include "circularbuffer.h"

CircularBuffer::CircularBuffer(int maxSize, int itemLen)
{
    this->maxSize = maxSize;
    this->itemLen = itemLen;
    this->head = 0;
    this->tail = 0;
    buffer = malloc(maxSize * itemLen);
}

void CircularBuffer::put(void* item)
{
    QMutexLocker locker(&mutex);
    while (tail == head + maxSize)
    {
        bufferIsNotFull.wait(&mutex);
    }
    buffer[tail++ % maxSize] = item;
    bufferIsNotEmpty.wakeOne();
}

void* CircularBuffer::get()
{
    QMutexLocker locker(&mutex);
    while (head == tail)
    {
        bufferIsNotEmpty.wait(&mutex);
    }
    void* item = buffer[head++ % maxSize];
    bufferIsNotFull.wakeOne();
    return item;
}
