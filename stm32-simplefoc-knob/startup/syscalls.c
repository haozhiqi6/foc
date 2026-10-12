#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include "stm32f10x.h"
extern char _end;
extern char _estack;
static char *heap_end;
int _write(int file, const char *ptr, int len) {
    int i; (void)file;
    for (i = 0; i < len; ++i) {
        while ((USART1->SR & USART_SR_TXE) == 0U) {}
        USART1->DR = (uint16_t)(unsigned char)ptr[i];
    }
    return len;
}
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; errno = EIO; return -1; }
void *_sbrk(ptrdiff_t incr) {
    char *prev; char *limit = &_estack - 0x400;
    if (heap_end == 0) heap_end = &_end;
    prev = heap_end;
    if (heap_end + incr > limit) { errno = ENOMEM; return (void *)-1; }
    heap_end += incr; return prev;
}
int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; (void)st; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
void _exit(int status) { (void)status; while (1) {} }
