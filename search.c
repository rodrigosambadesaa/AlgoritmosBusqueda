#include "search.h"

long search_sequential(vector_ptr_t vector, vector_element_t element)
{
    long i = 0;
    long vector_length = vector_size(&vector);

    if (vector_length <= 0)
    {
        return -1;
    }

    while (i < vector_length && bd_compare(vector_get(vector, i), element) != 0)
    {
        i++;
    }

    return (i < vector_length) ? i : -1;
}

long search_sequential_sentinel(vector_ptr_t vector, vector_element_t element)
{
    long i = 0;
    long n = vector_size(&vector);
    vector_element_t last_value;

    if (n <= 0)
    {
        return -1;
    }

    last_value = vector_get(vector, n - 1);
    vector->data[n - 1] = element;

    while (bd_compare(vector_get(vector, i), element) != 0)
    {
        i++;
    }

    vector->data[n - 1] = last_value;

    if (i < n - 1 || bd_compare(last_value, element) == 0)
    {
        return i;
    }

    return -1;
}

long search_binary(vector_ptr_t vector, vector_element_t element)
{
    long start = 0;
    long end = vector_size(&vector) - 1;
    long middle;

    if (end < 0)
    {
        return -1;
    }

    middle = (long)((start + end) / 2);
    while ((start <= end) && bd_compare(vector_get(vector, middle), element) != 0)
    {
        if (bd_compare(element, vector_get(vector, middle)) < 0)
        {
            end = middle - 1;
        }
        else
        {
            start = middle + 1;
        }

        middle = (long)((start + end) / 2);
    }

    return (start <= end) ? middle : -1;
}
