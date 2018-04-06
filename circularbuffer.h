#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QMutex>
#include <QWaitCondition>
#include <QDebug>

namespace commaudio
{
    class CircularBuffer
    {
    public:
        CircularBuffer(size_t max, size_t len);
        ~CircularBuffer();
        void put(void* item);
        void* pop();
        void popAll();
        bool isFull();
        bool isEmpty();
    private:
        QMutex mutex;
        QWaitCondition bufferIsNotFull;
        QWaitCondition bufferIsNotEmpty;
        size_t head;
        size_t tail;
        void** buffer;
        size_t numOfSlots;
        size_t itemLen;
    };
}

#endif // CIRCULARBUFFER_H
