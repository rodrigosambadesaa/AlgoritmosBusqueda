#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "dynamic_vector.h"

int main(void)
{
    vector_ptr_t v = NULL;
    BigDecimal *x;
    long initial_capacity;
    vector_create(&v, 0);
    assert(v != NULL && vector_size(&v) == 0);
    { vector_ptr_t old = v; vector_create(&v, 5); assert(v == old && vector_size(&v) == 0); }
    assert(vector_get(v, 0) == NULL);
    assert(vector_resize(v, 2));
    assert(vector_get(v, 0) == NULL && vector_get(v, 1) == NULL);
    vector_set(v, 0, bd_create_from_int(42));
    assert(vector_get(v, 0) != NULL);
    x = vector_get(v, 0);
    vector_set(v, 0, x); /* Autoasignacion: no liberar x. */
    assert(vector_get(v, 0) == x);
    vector_set(v, 1, x); /* Alias: debe clonar para tener dos propietarios. */
    assert(vector_get(v, 1) != x);
    assert(bd_compare(vector_get(v, 0), vector_get(v, 1)) == 0);
    vector_swap(&v, ULONG_MAX, 0);
    assert(vector_get(v, 0) == x);
    vector_swap(&v, 0, 1);
    assert(vector_get(v, 0) != x);
    assert(vector_get(v, 1) == x);
    initial_capacity = vector_capacity(v);
    assert(vector_reserve(v, 12));
    assert(vector_capacity(v) >= 12 && vector_size(&v) == 2);
    assert(vector_resize(v, 256)); /* Construye el indice de punteros. */
    vector_set(v, 100, bd_create_from_int(88));
    vector_set(v, 101, vector_get(v, 100));
    assert(vector_get(v, 100) != vector_get(v, 101));
    assert(vector_resize(v, 8));
    assert(vector_get(v, 7) == NULL);
    assert(vector_resize(v, 1)); /* Debe liberar el segundo objeto. */
    assert(!vector_resize(v, -1));
    assert(vector_size(&v) == 1);
    assert(!vector_reserve(v, LONG_MAX)); /* No desbordar size_t. */
    assert(vector_size(&v) == 1);
    x = bd_create_from_int(17);
    assert(x != NULL && vector_append(v, x));
    assert(vector_get(v, 1) == x);
    assert(vector_append(v, x)); /* Alias durante append. */
    assert(vector_get(v, 2) != x);
    assert(vector_resize(v, 0));
    assert(vector_capacity(v) >= initial_capacity);
    assert(vector_append(v, bd_create_from_int(7)));
    assert(vector_size(&v) == 1);
    vector_free(&v);
    vector_free(&v);
    assert(v == NULL);
    puts("dynamic_vector: OK");
    return 0;
}
