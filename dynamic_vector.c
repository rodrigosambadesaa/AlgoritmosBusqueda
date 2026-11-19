#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_vector.h"

/* Indice hash con direccion del puntero como clave. Si no hay memoria
 * adicional, se degrada a busqueda lineal sin perder ni datos ni seguridad.
 */
struct vector_index_slot
{
    vector_element_t key;
    unsigned char state; /* 0 vacio; 1 ocupado; 2 borrado */
};

static size_t vector_hash(vector_element_t p)
{
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 7;
    x ^= x >> 15;
    return (size_t)x;
}

static void index_disable(vector_ptr_t v)
{
    free(v->index_slots);
    v->index_slots = NULL;
    v->index_capacity = 0;
    v->index_count = 0;
    v->index_deleted = 0;
    v->index_disabled = 1;
}

static void index_raw_insert(vector_ptr_t v, vector_element_t key)
{
    size_t pos = vector_hash(key) & (v->index_capacity - 1);
    size_t first_deleted = SIZE_MAX;
    for (;;)
    {
        struct vector_index_slot *slot = &v->index_slots[pos];
        if (slot->state == 0)
        {
            if (first_deleted != SIZE_MAX)
                slot = &v->index_slots[first_deleted];
            if (slot->state == 2)
                --v->index_deleted;
            slot->key = key;
            slot->state = 1;
            ++v->index_count;
            return;
        }
        if (slot->state == 2 && first_deleted == SIZE_MAX)
            first_deleted = pos;
        if (slot->state == 1 && slot->key == key)
            return;
        pos = (pos + 1) & (v->index_capacity - 1);
    }
}

static int index_rehash(vector_ptr_t v, size_t capacity)
{
    struct vector_index_slot *old_slots = v->index_slots;
    size_t old_capacity = v->index_capacity, i;
    struct vector_index_slot *next;
    if (capacity > SIZE_MAX / sizeof(*next))
        return 0;
    next = (struct vector_index_slot *)calloc(capacity, sizeof(*next));
    if (next == NULL)
        return 0;
    v->index_slots = next;
    v->index_capacity = capacity;
    v->index_count = 0;
    v->index_deleted = 0;
    for (i = 0; i < old_capacity; ++i)
        if (old_slots[i].state == 1)
            index_raw_insert(v, old_slots[i].key);
    free(old_slots);
    return 1;
}

static void index_record(vector_ptr_t v, vector_element_t key)
{
    long i;
    if (v->index_disabled || key == NULL)
        return;
    if (v->index_slots == NULL)
    {
        if (v->size <= 64)
            return;
        if (!index_rehash(v, 128))
        {
            index_disable(v);
            return;
        }
        /* Construir una sola vez, incluidos los elementos ya existentes. */
        for (i = 0; i < v->size; ++i)
            if (v->data[i] != NULL)
                index_record(v, v->data[i]);
        return;
    }
    if (v->index_count + v->index_deleted >=
        v->index_capacity - v->index_capacity / 4)
    {
        if (v->index_capacity > SIZE_MAX / 2 ||
            !index_rehash(v, v->index_capacity * 2))
        {
            index_disable(v);
            return;
        }
    }
    index_raw_insert(v, key);
}

static int vector_owns(const vector_ptr_t vector, const vector_element_t value)
{
    long i;
    size_t pos;
    if (vector == NULL || value == NULL)
        return 0;
    if (vector->index_slots != NULL)
    {
        pos = vector_hash(value) & (vector->index_capacity - 1);
        for (;;)
        {
            const struct vector_index_slot *slot = &vector->index_slots[pos];
            if (slot->state == 0)
                return 0;
            if (slot->state == 1 && slot->key == value)
                return 1;
            pos = (pos + 1) & (vector->index_capacity - 1);
        }
    }
    for (i = 0; i < vector->size; ++i)
        if (vector->data[i] == value)
            return 1;
    return 0;
}

static void index_forget(vector_ptr_t v, vector_element_t key)
{
    size_t pos;
    if (v->index_slots == NULL || key == NULL)
        return;
    pos = vector_hash(key) & (v->index_capacity - 1);
    for (;;)
    {
        struct vector_index_slot *slot = &v->index_slots[pos];
        if (slot->state == 0)
            return;
        if (slot->state == 1 && slot->key == key)
        {
            slot->state = 2;
            slot->key = NULL;
            --v->index_count;
            ++v->index_deleted;
            return;
        }
        pos = (pos + 1) & (v->index_capacity - 1);
    }
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
    index_forget(vector, vector->data[position]);
    bd_free(vector->data[position]);
    vector->data[position] = replacement;
    index_record(vector, replacement);
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
            index_forget(vector, vector->data[i]);
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
    free((*vector)->index_slots);
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
