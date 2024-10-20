// malloc() -
// calloc() - 
// realloc() -
// free() -

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

struct header_t {
    size_t size;
    unsigned is_free;
    struct header_t *next;
};

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};

typedef union header header_t;

header_t *head=NULL, *tail=NULL;

pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size) {
    header_t *current = head;
    while(current) {
        if (current->s.is_free && current->s.size >= size)
            return current;
        current = current->s.next;
    }
    return NULL;
}

void *malloc(size_t size){
    size_t total_size;
    void *block;
    header_t *header;
    if (!size) {
        return NULL;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if(header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void*)-1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if (!head)
        head = header;
    if(tail)
        tail->s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}


void free(void *block) {
    header_t *header, *tmp;
    void *prgbreak;
    if(!block)
        return;
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block - 1;
    prgbreak = sbrk(0);
    if ((char*)block + header->s.size == prgbreak) {
        if (head == tail)
            head = tail = NULL;
        else {
            tmp = head;
            while(tmp) {
                if(tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *calloc(size_t num, size_t nsize) {
    size_t size;
    void *block;
    if(!num || !nsize)
        return NULL;
    block = malloc(size);
    if(!block)
        return NULL;
    memset(block, 0, size);
    return block;
}

void *realloc(void *block, size_t size) {
    header_t *header;
    void *ret;
    if(!block || !size)
        return malloc(size);
    header = (header_t*)block - 1;
    if(header->s.size >= size)
        return block;
    ret = malloc(size);
    if (ret) {
        memcpy(ret, block, head->s.size);
        free(block);
    }
    return ret;
}

void memlistOutput() {
    header_t *curr = head;
    printf("head = %p tail = %p \n", (void*)head, (void*)tail);
    while(curr) {
        printf("addres = %p sizee = %zu free=%u next=%p\n", (void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
        curr = curr->s.next;
    }
}
