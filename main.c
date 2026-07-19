#include "crosslibc/stdio.h"
#include "crosslibc/stdlib.h"
#include "crosslibc/string.h"

/* 
 * This file contains a kind of extended prove of concept of a custome implementation of the 
 * Linux kernel function load_elf_binary done in assembly
 */

/* Constants from flags.asm.inc and asm-generic_errno-base.lib.asm equivalents */
#ifndef BUFSIZ
#define BUFSIZ 8192
#endif

#define ELFMAG0 0x7f /* \177 */

// Definiciones para control de E/S estándar (reemplazando unistd.h / fcntl.h)
#define STDOUT_FILENO 1
#define O_RDONLY      0x0000

// Declaraciones explícitas de las syscalls emuladas para evitar warnings
int open(const char *pathname, int flags);
long read(int fd, void *buf, unsigned long count);
long write(int fd, const void *buf, unsigned long count);

// DEFINICIONES MANUALES REEMPLAZANDO A "sys/mman.h"
#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

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

// Declaración explícita de mmap para evitar "implicit declaration" warnings
void *mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);

/* section .bss */
char *ELF_FILENAME;   /* quad word variable to store the char pointer to the filename */
int ELF_FILE_FD;      /* quad word variable to store the file descriptor of the efi file */
char *IO_BUFFER;      /* quad word variable to store the char pointer to the buffer */
long IO_BUFFER_PTR;   /* quad word variable to store read/write index to IO-Buffer into */
int SYS_ERRNO;        /* C-style errno number variable */

/* section .data */
const char *FATAL_ERROR_STR = "Fatal error occured!\n";
const char *ARGUMENTS_ERROR_STR = "Usage: ./load_elf_binary.asm <path/to/binary>\n";

/* 
 * procedure for printing text
 * we expect the string to be printed TO BE null-terminated, and put into rdi
 */
void print_text(const char *rdi) {
    /* ENTER macro equivalent: push rbp; mov rbp, rsp; and rsp, -16 */
    /* In C, the compiler handles the prologue and alignment. */

    const char *rsi;
    char al;
    
    /* first find out how long the string is */
    rsi = rdi;        /* move string pointer into source index */
    al = -1;          /* move into al anything except the null terminator */
    
    /* .for: count the length of the string */
    while (1) {
        /* test al, al; js .end_for */
        if (al == 0) break; 
        
        /* lodsb: load byte at byte [rsi] into al and increment si */
        al = *rsi;
        rsi++;
        
        /* jmp .for */
    }
    /* .end_for: label to end the for loop */

    /* now calculate the length of the string */
    /* sub rsi, rdi */
    size_t length = (size_t)(rsi - rdi) - 1; /* -1 because lodsb incremented after loading \0 */

    /* mov rdx, rsi; mov rsi, rdi; mov rax, __NR_write; mov rdi, STDOUT; syscall */
    write(STDOUT_FILENO, rdi, length);

    /* .return: leave; ret */
}

/* 
 * procedure that mirrors behavior to to C-function perror
 * (Left empty in original assembly)
 */
void print_error() {
}

/* 
 * Custome implementation of load_elf_binary
 * Note that we use _start, therefore we dont have any glibc functionality
 * As opposed to the original function signature: static int load_elf_binary(struct linux_binprm *bprm)
 * we just take a simple char* filename as argument: int load_elf_binary(char* filename)
 */
int main(int argc, char **argv) {
    /* ENTER macro equivalent */

    /* **** Save the filename argument into the filename variable **** */
    /* check if we received enough arguments */
    /* cmp qword [rbp+8*2], 0x02; jne .print_usage */
    if (argc != 2) {
        goto print_usage;
    }

    /* else continue with execution */
    /* mov rsi, [rbp+8*3]; mov [ELF_FILENAME], rsi */
    ELF_FILENAME = argv[1];

    /* **** Create a buffer for read operations to store them read bytes into **** */
    /* Do this with mmap: void *mmap(void addr[.length], size_t length, int prot, int flags, int fd, off_t offset); */
    /* mov rax, __NR_mmap; ... syscall */
    IO_BUFFER = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    /* test rax, rax; js .error */
    if (IO_BUFFER == MAP_FAILED) {
        goto error;
    }

    /* First open the efi file - creating a file handle, initializing structs and pointers */
    /* .open_efi: */
    /* mov rax, __NR_open; mov rdi, ELF_FILENAME; mov rsi, O_RDONLY; syscall */
    ELF_FILE_FD = open(ELF_FILENAME, O_RDONLY);
    
    /* test rax, rax; js .error */
    if (ELF_FILE_FD < 0) {
        goto error;
    }

    /* Second, make some consistency checks - magic number, ... */
    /* .check_efi_consistency: */
    /* mov rax, __NR_read; mov rdi, [ELF_FILE_FD]; mov rsi, IO_BUFFER; mov rdx, 0x04; syscall */
    ssize_t bytes_read = read(ELF_FILE_FD, IO_BUFFER, 4);
    
    /* cmp byte [IO_BUFFER+0], ELFMAG0; jne .error */
    if (bytes_read < 4 || (unsigned char)IO_BUFFER[0] != ELFMAG0) {
        goto error;
    }
    /* ... */

    /* jmp .return */
    goto return_label;

error:
    /* .error: do some error printing and debugging information ... */
    /* mov rdi, FATAL_ERROR_STR; call print_text */
    print_text(FATAL_ERROR_STR);
    /* jmp .return */
    goto return_label;

print_usage:
    /* .print_usage: print the usage and exit */
    /* mov rdi, ARGUMENTS_ERROR_STR; call print_text */
    print_text(ARGUMENTS_ERROR_STR);
    /* jmp .return */
    goto return_label;

return_label:
    /* .return: leave; ret */
    return 0;
}
