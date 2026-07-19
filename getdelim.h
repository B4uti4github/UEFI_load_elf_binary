#ifndef _GETDELIM_H
#define _GETDELIM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

// Definición manual para entornos independientes/UEFI sin <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

// Declaramos las funciones de tu heap_impl
void *heap_alloc(size_t *size);
void heap_dealloc(void *memptr, size_t size);

static __attribute__((unused))
ssize_t my_getdelim(char **out, size_t *out_size, int delim, int in)
{
	size_t out_idx = 0;

	// 1. Inicializar el buffer si viene vacío (*out == NULL)
	if (*out == NULL) {
		*out_size = 128; // Asignamos un tamaño inicial razonable (ej. 128 bytes)
		size_t alloc_size = *out_size;
		*out = (char *)heap_alloc(&alloc_size);
		if (*out == MAP_FAILED || *out == NULL) {
			return -1;
		}
	}

	// Loop para leer carácter por carácter
	for (;;) {
		char c;
		// Usamos la función read(in, &c, 1) que tu clibc soporta nativamente
		if (read(in, &c, 1) <= 0) {
			if (out_idx == 0) return -1; // Fin de archivo o error sin leer nada
			break; 
		}

		// 2. Controlar el desbordamiento de memoria (Simulación de realloc)
		if (out_idx + 1 >= *out_size) {
			size_t old_size = *out_size;
			size_t new_size = old_size * 2; // Duplicamos el tamaño para mitigar llamadas a mmap
			
			size_t alloc_size = new_size;
			char *new_ptr = (char *)heap_alloc(&alloc_size);
			if (new_ptr == MAP_FAILED || new_ptr == NULL) {
				return -1;
			}

			// Copiamos los bytes que ya habíamos leído al nuevo bloque de memoria
			for (size_t i = 0; i < out_idx; i++) {
				new_ptr[i] = (*out)[i];
			}

			// Liberamos el bloque viejo usando el munmap de tu dealloc
			heap_dealloc(*out, old_size);
			
			*out = new_ptr;
			*out_size = new_size;
		}

		(*out)[out_idx++] = c;

		// Romper el bucle si alcanzamos el delimitador deseado
		if (c == (char)delim) {
			break;
		}
	}

	// Añadir el terminador nulo para cerrar el string de C de forma segura
	(*out)[out_idx] = '\0';

	// Retornar la cantidad de caracteres leídos incluyendo el delimitador
	return out_idx;
}

static inline __attribute__((unused))
ssize_t my_getline(char **out, size_t *out_size, int in)
{
	return my_getdelim(out, out_size, '\n', in);
}

#endif /* _GETDELIM_H */
