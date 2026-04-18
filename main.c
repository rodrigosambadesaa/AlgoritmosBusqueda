#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "search.h"
#include "quick_sort.h"
#include "dynamic_vector.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

static const long BENCHMARK_SIZES[] = {
    1000L, 10000L, 100000L, 500000L,
    1000000L, 5000000L, 50000000L};
static const size_t BENCHMARK_SIZES_COUNT = sizeof(BENCHMARK_SIZES) / sizeof(BENCHMARK_SIZES[0]);
static const long MAX_SEARCHES = 1000000L;
static const double EXECUTION_TIMEOUT_SECONDS = 30.0;

typedef enum
{
    BENCH_OK = 0,
    BENCH_TIMEOUT,
    BENCH_ALLOC_FAIL
} benchmark_status_t;

typedef struct
{
    benchmark_status_t status;
    long searches_executed;
    double elapsed_seconds;
} benchmark_result_t;

static double now_seconds(void)
{
    struct timespec ts;
    (void)timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static long searches_for_size(long size)
{
    long candidate = size / 50L;
    if (candidate < 10000L)
    {
        candidate = 10000L;
    }
    if (candidate > MAX_SEARCHES)
    {
        candidate = MAX_SEARCHES;
    }
    return candidate;
}

static int ensure_results_dir(void)
{
    int rc = MKDIR("results");
    return (rc == 0 || errno == EEXIST) ? 1 : 0;
}

static BigDecimal *random_big_decimal(void)
{
    long value = (long)(rand() % 200000);
    return bd_create_from_int((long long)value);
}

static int initialize_vector_random(vector_ptr_t vector)
{
    long i;
    long n = vector_size(&vector);

    if (n <= 0)
    {
        return 0;
    }

    for (i = 0; i < n; i++)
    {
        BigDecimal *value = random_big_decimal();
        if (value == NULL)
        {
            return 0;
        }
        vector_set(vector, i, value);
    }

    return 1;
}

static benchmark_result_t run_sequential(long size)
{
    long i;
    long total_searches = searches_for_size(size);
    double t_start;
    benchmark_result_t result;
    vector_ptr_t vector = NULL;

    result.status = BENCH_OK;
    result.searches_executed = 0;
    result.elapsed_seconds = 0.0;

    vector_create(&vector, size);
    if (vector == NULL || !initialize_vector_random(vector))
    {
        vector_free(&vector);
        result.status = BENCH_ALLOC_FAIL;
        return result;
    }

    t_start = now_seconds();
    for (i = 0; i < total_searches; i++)
    {
        BigDecimal *key = random_big_decimal();
        if (key == NULL)
        {
            vector_free(&vector);
            result.status = BENCH_ALLOC_FAIL;
            return result;
        }

        (void)search_sequential(vector, key);
        bd_free(key);

        result.searches_executed = i + 1;
        if ((i % 4096L) == 0L && (now_seconds() - t_start) > EXECUTION_TIMEOUT_SECONDS)
        {
            result.status = BENCH_TIMEOUT;
            break;
        }
    }

    result.elapsed_seconds = now_seconds() - t_start;
    vector_free(&vector);
    return result;
}

static benchmark_result_t run_sequential_sentinel(long size)
{
    long i;
    long total_searches = searches_for_size(size);
    double t_start;
    benchmark_result_t result;
    vector_ptr_t vector = NULL;

    result.status = BENCH_OK;
    result.searches_executed = 0;
    result.elapsed_seconds = 0.0;

    vector_create(&vector, size);
    if (vector == NULL || !initialize_vector_random(vector))
    {
        vector_free(&vector);
        result.status = BENCH_ALLOC_FAIL;
        return result;
    }

    t_start = now_seconds();
    for (i = 0; i < total_searches; i++)
    {
        BigDecimal *key = random_big_decimal();
        if (key == NULL)
        {
            vector_free(&vector);
            result.status = BENCH_ALLOC_FAIL;
            return result;
        }

        (void)search_sequential_sentinel(vector, key);
        bd_free(key);

        result.searches_executed = i + 1;
        if ((i % 4096L) == 0L && (now_seconds() - t_start) > EXECUTION_TIMEOUT_SECONDS)
        {
            result.status = BENCH_TIMEOUT;
            break;
        }
    }

    result.elapsed_seconds = now_seconds() - t_start;
    vector_free(&vector);
    return result;
}

static benchmark_result_t run_binary(long size)
{
    long i;
    long total_searches = searches_for_size(size);
    double t_start;
    benchmark_result_t result;
    vector_ptr_t vector = NULL;

    result.status = BENCH_OK;
    result.searches_executed = 0;
    result.elapsed_seconds = 0.0;

    vector_create(&vector, size);
    if (vector == NULL || !initialize_vector_random(vector))
    {
        vector_free(&vector);
        result.status = BENCH_ALLOC_FAIL;
        return result;
    }

    quick_sort(&vector, 0, vector_size(&vector) - 1);

    t_start = now_seconds();
    for (i = 0; i < total_searches; i++)
    {
        BigDecimal *key = random_big_decimal();
        if (key == NULL)
        {
            vector_free(&vector);
            result.status = BENCH_ALLOC_FAIL;
            return result;
        }

        (void)search_binary(vector, key);
        bd_free(key);

        result.searches_executed = i + 1;
        if ((i % 4096L) == 0L && (now_seconds() - t_start) > EXECUTION_TIMEOUT_SECONDS)
        {
            result.status = BENCH_TIMEOUT;
            break;
        }
    }

    result.elapsed_seconds = now_seconds() - t_start;
    vector_free(&vector);
    return result;
}

static const char *status_to_text(benchmark_status_t status)
{
    if (status == BENCH_TIMEOUT)
    {
        return "TIMEOUT";
    }
    if (status == BENCH_ALLOC_FAIL)
    {
        return "ALLOC_FAIL";
    }
    return "OK";
}

static int run_benchmark(const char *name, const char *filename, benchmark_result_t (*runner)(long))
{
    size_t i;
    FILE *fp;

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        return 0;
    }

    fprintf(fp, "n,searches_requested,searches_executed,time_seconds,status\n");
    printf("\n[%s]\n", name);
    for (i = 0; i < BENCHMARK_SIZES_COUNT; i++)
    {
        long size = BENCHMARK_SIZES[i];
        long requested = searches_for_size(size);
        benchmark_result_t result = runner(size);

        fprintf(fp, "%ld,%ld,%ld,%.9f,%s\n", size, requested, result.searches_executed, result.elapsed_seconds, status_to_text(result.status));
        printf("n=%-9ld searches=%-8ld time=%.6f s status=%s\n", size, result.searches_executed, result.elapsed_seconds, status_to_text(result.status));

        if (result.status == BENCH_ALLOC_FAIL)
        {
            printf("Se detiene esta serie por falta de memoria.\n");
            break;
        }
    }

    fclose(fp);
    return 1;
}

int main(void)
{
    srand((unsigned int)time(NULL));

    if (!ensure_results_dir())
    {
        fprintf(stderr, "No se pudo preparar el directorio results/.\n");
        return EXIT_FAILURE;
    }

    if (!run_benchmark("Busqueda Secuencial", "results/seq.csv", run_sequential))
    {
        fprintf(stderr, "Fallo en benchmark secuencial.\n");
        return EXIT_FAILURE;
    }

    if (!run_benchmark("Busqueda Secuencial con Centinela", "results/seq_cent.csv", run_sequential_sentinel))
    {
        fprintf(stderr, "Fallo en benchmark secuencial con centinela.\n");
        return EXIT_FAILURE;
    }

    if (!run_benchmark("Busqueda Binaria", "results/bin.csv", run_binary))
    {
        fprintf(stderr, "Fallo en benchmark binaria.\n");
        return EXIT_FAILURE;
    }

    printf("\nCSV generados en results/: seq.csv, seq_cent.csv, bin.csv\n");
    printf("Temporizacion con timespec_get() y timeout automatico de %.0f s por ejecucion.\n", EXECUTION_TIMEOUT_SECONDS);
    return EXIT_SUCCESS;
}
