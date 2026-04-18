#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "dynamic_vector.h"
#include "search.h"
#include "quick_sort.h"

static int failures = 0;

#define EXPECT_TRUE(cond, msg)                   \
    do                                           \
    {                                            \
        if (!(cond))                             \
        {                                        \
            fprintf(stderr, "[FAIL] %s\n", msg); \
            failures++;                          \
        }                                        \
    } while (0)

#define EXPECT_EQ_LONG(actual, expected, msg)                                      \
    do                                                                             \
    {                                                                              \
        long _a = (long)(actual);                                                  \
        long _e = (long)(expected);                                                \
        if (_a != _e)                                                              \
        {                                                                          \
            fprintf(stderr, "[FAIL] %s (actual=%ld expected=%ld)\n", msg, _a, _e); \
            failures++;                                                            \
        }                                                                          \
    } while (0)

static void fill_from_array(vector_ptr_t vector, const vector_element_t *array, long n)
{
    long i;
    for (i = 0; i < n; i++)
    {
        vector_set(vector, i, bd_copy(array[i]));
    }
}

static int is_sorted(vector_ptr_t vector)
{
    long i;
    long n = vector_size(&vector);
    for (i = 1; i < n; i++)
    {
        if (bd_compare(vector_get(vector, i - 1), vector_get(vector, i)) > 0)
        {
            return 0;
        }
    }
    return 1;
}

static void test_bigdecimal_basics(void)
{
    BigDecimal *a = bd_create_from_str("123.4500");
    BigDecimal *b = bd_create_from_str("-23.45");
    BigDecimal *two;
    BigDecimal *sum;
    BigDecimal *mul;
    BigDecimal *div;
    char *s;

    EXPECT_TRUE(a != NULL, "bd_create_from_str parsea positivo");
    EXPECT_TRUE(b != NULL, "bd_create_from_str parsea negativo");

    s = bd_to_string(a);
    EXPECT_TRUE(s != NULL && strcmp(s, "123.45") == 0, "bd_to_string normaliza ceros");
    free(s);

    sum = bd_add(a, b);
    s = bd_to_string(sum);
    EXPECT_TRUE(s != NULL && strcmp(s, "100") == 0, "bd_add correcto");
    free(s);
    bd_free(sum);

    mul = bd_multiply(a, b);
    s = bd_to_string(mul);
    EXPECT_TRUE(s != NULL && strcmp(s, "-2894.9025") == 0, "bd_multiply correcto");
    free(s);
    bd_free(mul);

    two = bd_create_from_int(2);
    div = bd_divide(a, two, 3);
    s = bd_to_string(div);
    EXPECT_TRUE(s != NULL && strcmp(s, "61.725") == 0, "bd_divide correcto");
    free(s);
    bd_free(div);
    bd_free(two);

    EXPECT_TRUE(bd_compare(a, b) > 0, "bd_compare correcto");

    bd_free(a);
    bd_free(b);
}

static void test_vector_basics(void)
{
    vector_ptr_t v = NULL;
    BigDecimal *expected;
    BigDecimal *x;
    BigDecimal *y;

    vector_create(&v, 5);
    EXPECT_TRUE(v != NULL, "crear reserva memoria");
    EXPECT_EQ_LONG(vector_size(&v), 5, "tamano correcto tras crear");

    x = bd_create_from_int(11);
    y = bd_create_from_int(22);
    vector_set(v, 0, x);
    vector_set(v, 4, y);
    expected = bd_create_from_int(11);
    EXPECT_TRUE(bd_compare(vector_get(v, 0), expected) == 0, "recuperar posicion 0");
    bd_free(expected);
    expected = bd_create_from_int(22);
    EXPECT_TRUE(bd_compare(vector_get(v, 4), expected) == 0, "recuperar ultima posicion");
    bd_free(expected);

    EXPECT_TRUE(vector_get(v, -1) == NULL, "recuperar indice negativo devuelve NULL");
    EXPECT_TRUE(vector_get(v, 5) == NULL, "recuperar fuera de rango devuelve NULL");

    vector_free(&v);
    EXPECT_TRUE(v == NULL, "liberar pone puntero a NULL");
}

static void test_quick_sort(void)
{
    vector_ptr_t v = NULL;
    BigDecimal *data[] = {
        bd_create_from_int(7), bd_create_from_int(2), bd_create_from_int(9), bd_create_from_int(2), bd_create_from_int(5),
        bd_create_from_int(1), bd_create_from_int(8), bd_create_from_int(3), bd_create_from_int(3), bd_create_from_int(0)};
    long n = (long)(sizeof(data) / sizeof(data[0]));
    long i;

    vector_create(&v, n);
    fill_from_array(v, data, n);
    quick_sort(&v, 0, n - 1);

    EXPECT_TRUE(is_sorted(v), "quick_sort ordena correctamente");

    vector_free(&v);
    for (i = 0; i < n; i++)
    {
        bd_free(data[i]);
    }
}

static void test_searches(void)
{
    vector_ptr_t v = NULL;
    BigDecimal *data[] = {
        bd_create_from_int(1), bd_create_from_int(3), bd_create_from_int(5),
        bd_create_from_int(7), bd_create_from_int(9), bd_create_from_int(11)};
    long n = (long)(sizeof(data) / sizeof(data[0]));
    BigDecimal *k;
    long i;

    vector_create(&v, n);
    fill_from_array(v, data, n);

    k = bd_create_from_int(7);
    EXPECT_EQ_LONG(search_sequential(v, k), 3, "search_sequential encuentra elemento");
    bd_free(k);
    k = bd_create_from_int(4);
    EXPECT_EQ_LONG(search_sequential(v, k), -1, "search_sequential devuelve -1 si no existe");
    bd_free(k);

    k = bd_create_from_int(11);
    EXPECT_EQ_LONG(search_sequential_sentinel(v, k), 5, "search_sequential_sentinel encuentra ultimo elemento");
    bd_free(k);
    k = bd_create_from_int(6);
    EXPECT_EQ_LONG(search_sequential_sentinel(v, k), -1, "search_sequential_sentinel devuelve -1 si no existe");
    bd_free(k);

    k = bd_create_from_int(1);
    EXPECT_EQ_LONG(search_binary(v, k), 0, "search_binary encuentra primer elemento");
    bd_free(k);
    k = bd_create_from_int(11);
    EXPECT_EQ_LONG(search_binary(v, k), 5, "search_binary encuentra ultimo elemento");
    bd_free(k);
    k = bd_create_from_int(10);
    EXPECT_EQ_LONG(search_binary(v, k), -1, "search_binary devuelve -1 si no existe");
    bd_free(k);

    vector_free(&v);
    for (i = 0; i < n; i++)
    {
        bd_free(data[i]);
    }
}

static void test_smoke_performance(void)
{
    vector_ptr_t v = NULL;
    const long n = 4000;
    const long reps = 15000;
    long i;
    clock_t t0;
    double elapsed;
    BigDecimal *k;

    vector_create(&v, n);
    for (i = 0; i < n; i++)
    {
        vector_set(v, i, bd_create_from_int(i * 2));
    }

    t0 = clock();
    for (i = 0; i < reps; i++)
    {
        k = bd_create_from_int((i * 7) % (n * 2));
        (void)search_binary(v, k);
        bd_free(k);
    }
    elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;

    /* Umbral holgado para evitar falsos negativos en máquinas lentas. */
    EXPECT_TRUE(elapsed < 0.5, "search_binary smoke performance (<0.5s)");

    vector_free(&v);
}

int main(void)
{
    clock_t start = clock();
    double total;

    test_bigdecimal_basics();
    test_vector_basics();
    test_quick_sort();
    test_searches();
    test_smoke_performance();

    total = (double)(clock() - start) / CLOCKS_PER_SEC;

    if (failures == 0)
    {
        printf("[OK] Todos los tests pasaron. Tiempo total: %.4f s\n", total);
        return 0;
    }

    printf("[ERROR] Fallos: %d. Tiempo total: %.4f s\n", failures, total);
    return 1;
}
