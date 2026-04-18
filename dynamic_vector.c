#include <stdio.h>
#include <stdlib.h>
#include "dynamic_vector.h"

void vector_create(vector_ptr_t *vector, long size)
{
    long i = 0;
    if (vector == NULL || size <= 0)
    {
        return;
    }
    *vector = (vector_ptr_t)malloc(sizeof(dynamic_vector_t));
    if (*vector == NULL)
    {
        return;
    }
    (*vector)->data = (vector_element_t *)malloc(size * sizeof(vector_element_t));
    if ((*vector)->data == NULL)
    {
        free(*vector);
        *vector = NULL;
        return;
    }
    (*vector)->size = size;
    for (i = 0; i < size; i++)
    {
        *((*vector)->data + i) = NULL;
    }
}

void vector_set(vector_ptr_t vector, long position, vector_element_t value)
{
    if (vector == NULL || position < 0 || position >= vector->size)
    {
        bd_free(value);
        return;
    }

    if (*(vector->data + position) != NULL)
    {
        bd_free(*(vector->data + position));
    }

    *(vector->data + position) = value;
}

void vector_free(vector_ptr_t *vector)
{
    long i;

    if (vector == NULL)
    {
        return;
    }

    if (*vector == NULL)
    {
        printf("Vector inexistente. No hay memoria que liberar...\n");
    }
    else
    {
        for (i = 0; i < (*vector)->size; i++)
        {
            bd_free((*vector)->data[i]);
            (*vector)->data[i] = NULL;
        }
        free((*vector)->data);
        free(*vector);
        (*vector) = NULL;
    }
}

vector_element_t vector_get(vector_ptr_t vector, long position)
{
    if (vector == NULL || position < 0 || position >= vector->size)
    {
        return NULL;
    }
    else
    {
        return *(vector->data + position);
    }
}

void vector_swap(vector_ptr_t *vector, unsigned long first_index, unsigned long second_index)
{
    vector_element_t temp;

    if (vector == NULL || *vector == NULL || (long)first_index >= (*vector)->size || (long)second_index >= (*vector)->size)
    {
        return;
    }

    temp = (*vector)->data[first_index];
    (*vector)->data[first_index] = (*vector)->data[second_index];
    (*vector)->data[second_index] = temp;
}

long vector_size(vector_ptr_t *vector)
{
    long size = 0;
    if (vector == NULL)
    {
        return -1;
    }
    if (*vector == NULL)
    {
        return -1;
    }
    else
        size = (*vector)->size;
    return size;
}
