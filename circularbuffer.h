#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <QMutex>
#include <QWaitCondition>

class CircularBuffer
{
public:
    CircularBuffer(size_t maxSize, size_t itemLen);
    ~CircularBuffer();
    void put(void* item);
    void* get();
private:
    QMutex mutex;
    QWaitCondition bufferIsNotFull;
    QWaitCondition bufferIsNotEmpty;
    size_t head;
    size_t tail;
    void* buffer;
    size_t maxSize;
    size_t itemLen;
};

#endif // CIRCULARBUFFER_H
