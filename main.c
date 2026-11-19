#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dynamic_vector.h"
#include "quick_sort.h"
#include "search.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

typedef enum
{
    ALG_SEQ = 0,
    ALG_SEQ_SENT,
    ALG_BIN
} algorithm_t;

typedef struct
{
    int id;
    const char *name;
    const char *filename;
    algorithm_t algorithm;
    long searches_label;
    long searches_reduced;
} benchmark_config_t;

/* La configuracion normal conserva todas las mediciones historicas. */
#ifdef VECTOR_CI_SMOKE
static const long BENCHMARK_SIZES[] = {8L, 32L, 128L};
#else
static const long BENCHMARK_SIZES[] = {
    10000L, 25000L, 50000L, 100000L, 200000L};
#endif
static const size_t BENCHMARK_SIZES_COUNT = sizeof(BENCHMARK_SIZES) / sizeof(BENCHMARK_SIZES[0]);

static const benchmark_config_t CONFIGS[] = {
    {1, "BSEC100000", "BSEC100000.DAT", ALG_SEQ, 100000L, 200L},
    {2, "BSEC5000000", "BSEC5000000.DAT", ALG_SEQ, 5000000L, 1000L},
    {3, "BSEC500000000", "BSEC500000000.DAT", ALG_SEQ, 500000000L, 3000L},
    {4, "BSECCEN100000", "BSECCEN100000.DAT", ALG_SEQ_SENT, 100000L, 200L},
    {5, "BSECCEN5000000", "BSECCEN5000000.DAT", ALG_SEQ_SENT, 5000000L, 1000L},
    {6, "BSECCEN500000000", "BSECCEN500000000.DAT", ALG_SEQ_SENT, 500000000L, 3000L},
    {7, "BBIN100000", "BBIN100000.DAT", ALG_BIN, 100000L, 200L},
    {8, "BBIN5000000", "BBIN5000000.DAT", ALG_BIN, 5000000L, 1000L},
    {9, "BBIN500000000", "BBIN500000000.DAT", ALG_BIN, 500000000L, 3000L}};
static const size_t CONFIG_COUNT = sizeof(CONFIGS) / sizeof(CONFIGS[0]);

static double now_seconds(void)
{
    struct timespec ts;
    (void)timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static int ensure_results_dir(void)
{
    int rc = MKDIR("results");
    return (rc == 0 || errno == EEXIST) ? 1 : 0;
}

static long random_long(long limit)
{
    long value = ((long)rand() << 16) ^ (long)rand();
    if (value < 0)
    {
        value = -value;
    }
    return (limit > 0) ? (value % limit) : value;
}

static BigDecimal *random_big_decimal(long max_value)
{
    return bd_create_from_int((long long)random_long(max_value));
}

static int initialize_vector_random(vector_ptr_t vector, long n)
{
    long i;
    long max_value = (n * 3L) + 1000L;

    for (i = 0; i < n; i++)
    {
        BigDecimal *value = random_big_decimal(max_value);
        if (value == NULL)
        {
            return 0;
        }
        vector_set(vector, i, value);
    }

    return 1;
}

static long run_one_search(vector_ptr_t vector, algorithm_t algorithm, BigDecimal *key)
{
    if (algorithm == ALG_SEQ)
    {
        return search_sequential(vector, key);
    }
    if (algorithm == ALG_SEQ_SENT)
    {
        return search_sequential_sentinel(vector, key);
    }
    return search_binary(vector, key);
}

static int run_config(const benchmark_config_t *config)
{
    size_t i;
    char output_path[128];
    FILE *fp;
    int completed = 1;

    (void)snprintf(output_path, sizeof(output_path), "results/%s", config->filename);
    fp = fopen(output_path, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "No se pudo crear %s\n", output_path);
        return 0;
    }

    fprintf(fp, "n,searches_label,searches_executed,time_seconds\n");
    printf("\n[%s] -> %s\n", config->name, output_path);

    for (i = 0; i < BENCHMARK_SIZES_COUNT; i++)
    {
        long n = BENCHMARK_SIZES[i];
        long j;
        long max_value = (n * 3L) + 1000L;
        double t0;
        double elapsed;
        vector_ptr_t vector = NULL;
        int alloc_ok = 1;

        vector_create(&vector, n);
        if (vector == NULL || !initialize_vector_random(vector, n))
        {
            alloc_ok = 0;
        }

        if (!alloc_ok)
        {
            fprintf(stderr, "Fallo de memoria para n=%ld en %s\n", n, config->name);
            vector_free(&vector);
            completed = 0;
            break;
        }

        t0 = now_seconds();

        if (config->algorithm == ALG_BIN)
        {
            /* En binaria se cronometra ordenacion + busquedas. */
            quick_sort(&vector, 0, vector_size(&vector) - 1);
        }

        for (j = 0; j < config->searches_reduced; j++)
        {
            BigDecimal *key = random_big_decimal(max_value);
            if (key == NULL)
            {
                alloc_ok = 0;
                break;
            }
            (void)run_one_search(vector, config->algorithm, key);
            bd_free(key);
        }

        elapsed = now_seconds() - t0;

        if (!alloc_ok)
        {
            fprintf(stderr, "Fallo de memoria en bucle de busqueda para %s\n", config->name);
            vector_free(&vector);
            break;
        }

        fprintf(fp, "%ld,%ld,%ld,%.9f\n", n, config->searches_label, config->searches_reduced, elapsed);
        printf("n=%-8ld label=%-10ld reduced=%-6ld time=%.6f s\n", n, config->searches_label, config->searches_reduced, elapsed);

        vector_free(&vector);
    }

    fclose(fp);
    return completed;
}

static const benchmark_config_t *find_config_by_id(int id)
{
    size_t i;
    for (i = 0; i < CONFIG_COUNT; i++)
    {
        if (CONFIGS[i].id == id)
        {
            return &CONFIGS[i];
        }
    }
    return NULL;
}

static void print_menu(void)
{
    size_t i;

    printf("\nBenchmark de busquedas (modo reducido para ejecucion rapida)\n");
    printf("Seleccione opcion:\n");
    for (i = 0; i < CONFIG_COUNT; i++)
    {
        printf("  %d) %s\n", CONFIGS[i].id, CONFIGS[i].name);
    }
    printf("  0) Salir\n");
}

static int run_all(void)
{
    size_t i;
    for (i = 0; i < CONFIG_COUNT; i++)
    {
        if (!run_config(&CONFIGS[i]))
        {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
#ifdef VECTOR_CI_SMOKE
    srand(0xC0FFEEu);
#else
    srand((unsigned int)time(NULL));
#endif

    if (!ensure_results_dir())
    {
        fprintf(stderr, "No se pudo preparar el directorio results/.\n");
        return EXIT_FAILURE;
    }

    if (argc > 1)
    {
        if (strcmp(argv[1], "--all") == 0)
        {
            if (!run_all())
            {
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        if (strcmp(argv[1], "--option") == 0 && argc > 2)
        {
            int id = atoi(argv[2]);
            const benchmark_config_t *config = find_config_by_id(id);
            if (config == NULL)
            {
                fprintf(stderr, "Opcion invalida: %d\n", id);
                return EXIT_FAILURE;
            }
            return run_config(config) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        fprintf(stderr, "Uso: %s [--all | --option N]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (;;)
    {
        int option = -1;
        const benchmark_config_t *config;

        print_menu();
        printf("Opcion: ");
        if (scanf("%d", &option) != 1)
        {
            fprintf(stderr, "Entrada invalida.\n");
            return EXIT_FAILURE;
        }

        if (option == 0)
        {
            break;
        }

        config = find_config_by_id(option);
        if (config == NULL)
        {
            printf("Opcion fuera de rango.\n");
            continue;
        }

        if (!run_config(config))
        {
            return EXIT_FAILURE;
        }

        printf("Archivo generado: results/%s\n", config->filename);
    }

    return EXIT_SUCCESS;
}
