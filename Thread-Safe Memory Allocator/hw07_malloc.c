#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "xmalloc.h"
#include "hmalloc.h"

pthread_mutex_t m;

void*
xmalloc(size_t bytes)
{
    pthread_mutex_lock(&m);
    void* alloc = hmalloc(bytes);
    pthread_mutex_unlock(&m);
    return alloc;
}

void
xfree(void* ptr)
{
    pthread_mutex_lock(&m);
    hfree(ptr);
    pthread_mutex_unlock(&m);
}

void*
xrealloc(void* prev, size_t bytes)
{
    pthread_mutex_lock(&m);
    void* realloc = hrealloc(prev, bytes);
    pthread_mutex_unlock(&m);
    return realloc;
}

