#include "quick_sort.h"

void quick_sort(vector_ptr_t *vector, long begin_index, long end_index)
{
    long i;
    long j;
    vector_element_t pivot;

    if (vector == NULL || *vector == NULL || begin_index >= end_index)
    {
        return;
    }

    i = begin_index;
    j = end_index;
    pivot = vector_get(*vector, begin_index + (end_index - begin_index) / 2);

    while (i <= j)
    {
        while (bd_compare(vector_get(*vector, i), pivot) < 0)
        {
            i++;
        }
        while (bd_compare(vector_get(*vector, j), pivot) > 0)
        {
            j--;
        }
        if (i <= j)
        {
            vector_swap(vector, (unsigned long)i, (unsigned long)j);
            i++;
            j--;
        }
    }

    if (begin_index < j)
    {
        quick_sort(vector, begin_index, j);
    }
    if (i < end_index)
    {
        quick_sort(vector, i, end_index);
    }
}
