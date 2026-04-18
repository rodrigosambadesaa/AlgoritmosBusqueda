#ifndef SEARCH_H
#define SEARCH_H

#include "dynamic_vector.h"

long search_sequential(vector_ptr_t vector, vector_element_t element);
long search_sequential_sentinel(vector_ptr_t vector, vector_element_t element);
long search_binary(vector_ptr_t vector, vector_element_t element);

#endif
