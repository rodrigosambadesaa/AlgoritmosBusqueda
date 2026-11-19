#ifndef DYNAMIC_VECTOR_H
#define DYNAMIC_VECTOR_H

#include <stddef.h>
#include "big_decimal.h"

/* El vector es propietario de los BigDecimal almacenados.
 * vector_set recibe la propiedad de punteros externos; si el valor ya se
 * encuentra dentro del vector, se copia para evitar aliasing/doble free.
 * vector_get devuelve una referencia prestada, que no debe liberarse.
 */
typedef BigDecimal *vector_element_t;

typedef struct
{
    vector_element_t *data;
    long size;
    long capacity;
    /* Indice de punteros propios: deteccion de alias O(1) amortizado. */
    struct vector_index_slot *index_slots;
    size_t index_capacity;
    size_t index_count;
    size_t index_deleted;
    int index_disabled;
} dynamic_vector_t;

typedef dynamic_vector_t *vector_ptr_t;

/* Devuelve NULL en *vector si la reserva falla. Acepta longitud cero. */
void vector_create(vector_ptr_t *vector, long size);
void vector_set(vector_ptr_t vector, long position, vector_element_t value);
void vector_swap(vector_ptr_t *vector, unsigned long first_index, unsigned long second_index);
vector_element_t vector_get(vector_ptr_t vector, long position);
void vector_free(vector_ptr_t *vector);
long vector_size(vector_ptr_t *vector);

/* Operaciones nuevas. Devuelven 1 si tuvieron exito y 0 si fallaron.
 * Al crecer, las nuevas posiciones contienen NULL; al reducir, los
 * BigDecimal descartados se liberan. Si falla una reserva, no hay cambios.
 */
int vector_reserve(vector_ptr_t vector, long capacity);
int vector_resize(vector_ptr_t vector, long size);
long vector_capacity(vector_ptr_t vector);
/* Transfiere la propiedad de value SOLO en caso de exito. */
int vector_append(vector_ptr_t vector, vector_element_t value);

#endif
