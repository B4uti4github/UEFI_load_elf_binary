#include "crosslibc/user-impl.h"
#include "crosslibc/string.h"

// REMAPEO DE SYSCALLS AL CARGADOR NATIVO (Prefijos fd_ y mem_)
#define write  fd_write
#define read   fd_read
#define mmap   mem_mmap
#define munmap mem_munmap

// Declaraciones de firmas exactas imitando tu cabecera original
long fd_write(int fd, const void *buf, size_t count);
long fd_read(int fd, void *buf, size_t count);
void *mem_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);
int mem_munmap(void *addr, size_t length);

// Definición manual de constantes mmap de Linux para entornos autónomos (UEFI)
#ifndef PROT_READ
#define PROT_READ      0x1
#endif

#ifndef PROT_WRITE
#define PROT_WRITE     0x2
#endif

#ifndef MAP_PRIVATE
#define MAP_PRIVATE    0x02
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS  0x20
#endif


void _putchar(char c) {
    write(1, &c, 1);
}

char _getchar() {
    char c;
    read(0, &c, 1);
    return c;
}

void _puts(const char *str) {
    write(1, str, strlen(str));
    _putchar('\n');
}


void heap_init() {
    // Nothing to do
}

void *heap_alloc(size_t *size) {
    return mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
}

void heap_dealloc(void *memptr, size_t size) {
    munmap(memptr, size);
}

size_t get_mem_used() {
    return 0;
}

size_t get_mem_total() {
    return 0;
}

size_t get_mem_free() {
    return 0;
}

