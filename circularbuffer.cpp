#include "circularbuffer.h"

namespace commaudio
{
    CircularBuffer::CircularBuffer(size_t nSlots, size_t len)
    {
        numOfSlots = nSlots;
        itemLen = len;
        head = 0;
        tail = 0;
        buffer = new void*[numOfSlots];
    }

    void CircularBuffer::put(void* item)
    {
        QMutexLocker locker(&mutex);
        while (tail == head + numOfSlots)
        {
            qDebug() << "buffer is full";
            bufferIsNotFull.wait(&mutex);
        }
        buffer[tail++ % numOfSlots] = item;
        bufferIsNotEmpty.wakeOne();
    }

    void* CircularBuffer::pop()
    {
        QMutexLocker locker(&mutex);
        while (head == tail)
        {
            qDebug() << "buffer is empty";
            bufferIsNotEmpty.wait(&mutex);
        }
        void* item = buffer[head++ % numOfSlots];
        bufferIsNotFull.wakeOne();
        return item;
    }

    void CircularBuffer::popAll()
    {
        for (int i = 0; i < numOfSlots; i++)
        {
            free(buffer[i]);
        }
    }

    bool CircularBuffer::isFull()
    {
        return (tail == head + numOfSlots);
    }

    bool CircularBuffer::isEmpty()
    {
        return (head == tail);
    }

    CircularBuffer::~CircularBuffer()
    {
        delete[] buffer;
    }
}
