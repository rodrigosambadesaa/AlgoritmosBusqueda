#ifndef BIG_DECIMAL_H
#define BIG_DECIMAL_H

#include <stdbool.h>
#include <stdio.h>

/**
 * Estructura BigDecimal
 * Representa un numero decimal de precision arbitraria.
 * Ejemplo: 123.45 se puede almacenar como el entero 12345 con escala 2.
 */
typedef struct
{
    char *digits;
    int sign;
    int precision;
    int scale;
} BigDecimal;

/* --- Constructores y Destructores --- */

BigDecimal *bd_create_from_str(const char *str);
BigDecimal *bd_create_from_int(long long value);
BigDecimal *bd_copy(const BigDecimal *src);
void bd_free(BigDecimal *bd);

/* --- Operaciones Aritmeticas --- */

BigDecimal *bd_add(const BigDecimal *a, const BigDecimal *b);
BigDecimal *bd_subtract(const BigDecimal *a, const BigDecimal *b);
BigDecimal *bd_multiply(const BigDecimal *a, const BigDecimal *b);
BigDecimal *bd_divide(const BigDecimal *a, const BigDecimal *b, int limit_precision);

/* --- Comparaciones --- */

int bd_compare(const BigDecimal *a, const BigDecimal *b);

/* --- Utilidades --- */

char *bd_to_string(const BigDecimal *bd);
void bd_print(const BigDecimal *bd);
bool bd_is_zero(const BigDecimal *bd);

#endif