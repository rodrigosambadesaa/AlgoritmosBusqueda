
#ifndef DYNAMIC_VECTOR_H
#define DYNAMIC_VECTOR_H

#include "big_decimal.h"

typedef BigDecimal *vector_element_t;

typedef struct
{
    vector_element_t *data;
    long size;
} dynamic_vector_t;

typedef dynamic_vector_t *vector_ptr_t;

void vector_create(vector_ptr_t *vector, long size);
void vector_set(vector_ptr_t vector, long position, vector_element_t value);
void vector_swap(vector_ptr_t *vector, unsigned long first_index, unsigned long second_index);
vector_element_t vector_get(vector_ptr_t vector, long position);
void vector_free(vector_ptr_t *vector);
long vector_size(vector_ptr_t *vector);

#endif
