#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_vector.h"

static int vector_owns(const vector_ptr_t vector, const vector_element_t value)
{
    long i;
    if (vector == NULL || value == NULL)
        return 0;
    for (i = 0; i < vector->size; ++i)
        if (vector->data[i] == value)
            return 1;
    return 0;
}

/* La operacion conserva el valor anterior si fallase la copia del alias. */
static int vector_replace(vector_ptr_t vector, long position, vector_element_t value)
{
    vector_element_t replacement = value;
    if (vector == NULL || position < 0 || position >= vector->size)
        return 0;
    if (vector->data[position] == value)
        return 1;
    if (vector_owns(vector, value))
    {
        replacement = bd_copy(value);
        if (replacement == NULL)
            return 0;
    }
    bd_free(vector->data[position]);
    vector->data[position] = replacement;
    return 1;
}

void vector_create(vector_ptr_t *vector, long size)
{
    vector_ptr_t created;
    if (vector == NULL)
        return;
    *vector = NULL;
    if (size < 0)
        return;
    created = (vector_ptr_t)calloc(1, sizeof(*created));
    if (created == NULL)
        return;
    if (!vector_reserve(created, size))
    {
        free(created);
        return;
    }
    created->size = size;
    *vector = created;
}

int vector_reserve(vector_ptr_t vector, long capacity)
{
    vector_element_t *memory;
    if (vector == NULL || capacity < 0)
        return 0;
    if (capacity <= vector->capacity)
        return 1;
    if ((uintmax_t)capacity > SIZE_MAX / sizeof(*vector->data))
        return 0;
    memory = (vector_element_t *)realloc(vector->data,
                                        (size_t)capacity * sizeof(*vector->data));
    if (memory == NULL)
        return 0;
    memset(memory + vector->capacity, 0,
           (size_t)(capacity - vector->capacity) * sizeof(*memory));
    vector->data = memory;
    vector->capacity = capacity;
    return 1;
}

int vector_resize(vector_ptr_t vector, long size)
{
    long i, capacity;
    if (vector == NULL || size < 0)
        return 0;
    if (size < vector->size)
    {
        for (i = size; i < vector->size; ++i)
        {
            bd_free(vector->data[i]);
            vector->data[i] = NULL;
        }
    }
    else if (size > vector->size)
    {
        if (size > vector->capacity)
        {
            capacity = vector->capacity ? vector->capacity : 1;
            while (capacity < size)
            {
                if (capacity > LONG_MAX / 2)
                {
                    capacity = size;
                    break;
                }
                capacity *= 2;
            }
            if (!vector_reserve(vector, capacity))
                return 0;
        }
        for (i = vector->size; i < size; ++i)
            vector->data[i] = NULL;
    }
    vector->size = size;
    return 1;
}

void vector_set(vector_ptr_t vector, long position, vector_element_t value)
{
    if (!vector_replace(vector, position, value) && !vector_owns(vector, value))
        bd_free(value); /* Compatibilidad: valor entrante no insertado. */
}

int vector_append(vector_ptr_t vector, vector_element_t value)
{
    long old_size;
    if (vector == NULL || vector->size == LONG_MAX)
        return 0;
    old_size = vector->size;
    if (!vector_resize(vector, old_size + 1))
        return 0;
    if (!vector_replace(vector, old_size, value))
    {
        (void)vector_resize(vector, old_size);
        return 0;
    }
    return 1;
}

void vector_swap(vector_ptr_t *vector, unsigned long first_index, unsigned long second_index)
{
    vector_element_t temp;
    if (vector == NULL || *vector == NULL ||
        first_index >= (unsigned long)(*vector)->size ||
        second_index >= (unsigned long)(*vector)->size)
        return;
    temp = (*vector)->data[first_index];
    (*vector)->data[first_index] = (*vector)->data[second_index];
    (*vector)->data[second_index] = temp;
}

void vector_free(vector_ptr_t *vector)
{
    long i;
    if (vector == NULL || *vector == NULL)
        return;
    for (i = 0; i < (*vector)->size; ++i)
        bd_free((*vector)->data[i]);
    free((*vector)->data);
    free(*vector);
    *vector = NULL;
}

vector_element_t vector_get(vector_ptr_t vector, long position)
{
    if (vector == NULL || position < 0 || position >= vector->size)
        return NULL;
    return vector->data[position];
}

long vector_size(vector_ptr_t *vector)
{
    return (vector == NULL || *vector == NULL) ? -1 : (*vector)->size;
}

long vector_capacity(vector_ptr_t vector)
{
    return vector == NULL ? -1 : vector->capacity;
}
