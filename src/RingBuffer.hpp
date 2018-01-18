/*
 * RingBuffer.hpp
 */

#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <iostream>
#include <atomic>
#include <string>
#include <unistd.h>
#include <string.h> // memcpy
#include <cstring>

template <class T>
class RingBuffer
{
public:
    RingBuffer();
    RingBuffer(unsigned long size);
    ~RingBuffer();
    void setup(unsigned long size);
    
    unsigned long push(const T* data, unsigned long n);
    unsigned long pop(T* data, unsigned long n);
    unsigned long getWithoutPop(T* data, unsigned long n);
    unsigned long items_available_for_write() const;
    unsigned long items_available_for_read() const;
    bool isLockFree() const;
    void pushMayBlock(bool block);
    void popMayBlock(bool block);
    void setBlockingNap(unsigned long blockingNap);
    void setSize(unsigned long size);
    void reset();
    
private:
    unsigned long array_size;
    T* buffer = nullptr;
    long type_size; // also depends on #channels

    std::atomic<unsigned long> tail_index; // write pointer
    std::atomic<unsigned long> head_index; // read pointer

    bool blockingPush;
    bool blockingPop;
    useconds_t blockingNap = 500;
}; // RingBuffer{}

template <class T>
RingBuffer<T>::RingBuffer()
{
    tail_index = 0;
    head_index = 0;
    
    blockingPush = false;
    blockingPop = false;
} // RingBuffer()

template <class T>
RingBuffer<T>::RingBuffer(unsigned long array_size)
{
    tail_index = 0;
    head_index = 0;
    
    blockingPush = false;
    blockingPop = false;
    
    setup(array_size);
} // RingBuffer()

template <class T>
RingBuffer<T>::~RingBuffer()
{
    if (buffer)
        delete [] buffer;
} // ~RingBuffer()

template <class T>
void RingBuffer<T>::setup(unsigned long array_size)
{
    if (buffer)
        delete [] buffer;
    
    this->array_size = array_size;
    buffer = new T [array_size](); // allocate storage
    type_size = sizeof(T);
}

template <class T>
unsigned long RingBuffer<T>::items_available_for_write() const
{
    long pointerspace = head_index.load()-tail_index.load(); // signed
    
    if(pointerspace > 0) return pointerspace; // NB: > 0 so NOT including 0
    else return pointerspace + array_size;
} // items_available_for_write()

template <class T>
unsigned long RingBuffer<T>::items_available_for_read() const
{
    long pointerspace = tail_index.load() - head_index.load(); // signed
    
    if(pointerspace >= 0) return pointerspace; // NB: >= 0 so including 0
    else return pointerspace + array_size;
} // items_available_for_read()

template <class T>
void RingBuffer<T>::pushMayBlock(bool block)
{
    this->blockingPush = block;
} // pushMayBlock()

template <class T>
void RingBuffer<T>::popMayBlock(bool block)
{
    this->blockingPop = block;
} // popMayBlock()

template <class T>
void RingBuffer<T>::setBlockingNap(unsigned long blockingNap)
{
    this->blockingNap = blockingNap;
} // setBlockingNap()


/*
 * Try to write as many items as possible and return the number actually written
 */
template <class T>
unsigned long RingBuffer<T>::push(const T* data, unsigned long n)
{
    unsigned long space = array_size, n_to_write, memory_to_write, first_chunk, second_chunk, current_tail;
    
    //space = items_available_for_write();
    if(blockingPush)
    {
        while((space = items_available_for_write()) < n)
        { // blocking
            usleep(blockingNap);
        } // while, possibly use a system level sleep operation with CVAR?
    } // if
    
    n_to_write = (n <= space) ? n : space; // limit
    
    current_tail = tail_index.load();
    if(current_tail + n_to_write <= array_size)
    { // chunk fits without wrapping
        memory_to_write = n_to_write * type_size;
        memcpy(buffer+current_tail, data, memory_to_write);
        tail_index.store(current_tail + n_to_write);
    }
    else { // chunk has to wrap
        first_chunk = array_size - current_tail;
        memory_to_write = first_chunk * type_size;
        memcpy(buffer + current_tail, data, memory_to_write);
        
        second_chunk = n_to_write - first_chunk;
        memory_to_write = second_chunk * type_size;
        memcpy(buffer, data + first_chunk, memory_to_write);
        tail_index.store(second_chunk);
    }
    
    return n_to_write;
} // push()


/*
 * Try to read as many items as possible and return the number actually read
 */
template <class T>
unsigned long RingBuffer<T>::pop(T* data, unsigned long n)
{
    unsigned long space = array_size, n_to_read, memory_to_read, first_chunk, second_chunk, current_head;
    
    //space = items_available_for_read(); //if checking outside of thread, not necessary?
    if(blockingPop)
    {
        while((space = items_available_for_read()) < n)
        { // blocking
            usleep(blockingNap);
        } // while
    } // if
    
    if(space == 0)
        return 0;
    
    n_to_read = (n <= space) ? n : space; // limit
    
    current_head = head_index.load();
    if(current_head + n_to_read <= array_size)
    { // no wrapping necessary
        memory_to_read = n_to_read * type_size;
        memcpy(data, buffer + current_head, memory_to_read);
        head_index.store(current_head + n_to_read);
    }
    else { // read has to wrap
        first_chunk = array_size - current_head;
        memory_to_read = first_chunk * type_size;
        memcpy(data, buffer + current_head, memory_to_read);
        
        second_chunk = n_to_read - first_chunk;
        memory_to_read = second_chunk * type_size;
        memcpy(data + first_chunk, buffer, memory_to_read);
        head_index.store(second_chunk);

    }
    return n_to_read;
} // pop()

/*
 * Try to read as many items as possible and return the number actually read
 */
template <class T>
unsigned long RingBuffer<T>::getWithoutPop(T* data, unsigned long n)
{
    unsigned long space = array_size, n_to_read, memory_to_read, first_chunk, second_chunk, current_head;
    
    //space = items_available_for_read(); //if checking outside of thread, not necessary?
    if(blockingPop)
    {
        while((space = items_available_for_read()) < n)
        { // blocking
            usleep(blockingNap);
        } // while
    } // if
    
    if(space == 0)
        return 0;
    
    n_to_read = (n <= space) ? n : space; // limit
    
    current_head = head_index.load();
    if(current_head + n_to_read <= array_size)
    { // no wrapping necessary
        memory_to_read = n_to_read * type_size;
        memcpy(data, buffer + current_head, memory_to_read);
        //head_index.store(current_head + n_to_read);
    }
    else { // read has to wrap
        first_chunk = array_size - current_head;
        memory_to_read = first_chunk * type_size;
        memcpy(data, buffer + current_head, memory_to_read);
        
        second_chunk = n_to_read - first_chunk;
        memory_to_read = second_chunk * type_size;
        memcpy(data + first_chunk, buffer, memory_to_read);
        //head_index.store(second_chunk);
        
    }
    return n_to_read;
} // getWithoutPop()

template <class T>
bool RingBuffer<T>::isLockFree() const
{
    return (tail_index.is_lock_free() && head_index.is_lock_free());
} // isLockFree()


template <class T>
void RingBuffer<T>::setSize(unsigned long size)
{
    if (buffer)
        delete [] buffer;
    
    this->array_size = array_size;
    buffer = new T [array_size](); // allocate storage
    type_size = sizeof(T);
    tail_index.store(0);
    head_index.store(0);
}

template <class T>
void RingBuffer<T>::reset()
{
    tail_index.store(0);
    head_index.store(0);
} // isLockFree()

#endif
