#include "RACAMS5.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#endif


#if defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

#define RACAMS_INSERTION_THRESHOLD 32
#define RACAMS_ADAPT_RATIO 1.20      /* if pass time rises >20%, shrink block */
#define RACAMS_BLOCK_SHRINK_NUM 3    /* shrink by 3/4 */
#define RACAMS_BLOCK_SHRINK_DEN 4


#define RACAMS_USE_BLOCK_MERGE 0
#define RACAMS_USE_TIMING_ADAPT 0
#define RACAMS_DEBUG_ADAPT 0

/* --------------------------------------------------------- */
/* Timing helper                                             */
/* --------------------------------------------------------- */

static double racams_now_ns(void)
{
#if defined(_WIN32)
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1e9 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
#endif
}

/* --------------------------------------------------------- */
/* Utility                                                   */
/* --------------------------------------------------------- */

static void swap_bytes(char *a, char *b, size_t elem_size)
{
    while (elem_size--) {
        char tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}

/* --------------------------------------------------------- */
/* Insertion Sort                                            */
/* --------------------------------------------------------- */

static void insertion_sort(void *base,
                           size_t n,
                           size_t elem_size,
                           racams_cmp_fn cmp)
{
    char *arr = (char *)base;

    for (size_t i = 1; i < n; i++) {
        size_t j = i;

        while (j > 0 &&
               cmp(arr + j * elem_size,
                   arr + (j - 1) * elem_size) < 0) {
            swap_bytes(arr + j * elem_size,
                       arr + (j - 1) * elem_size,
                       elem_size);
            j--;
        }
    }
}

/* --------------------------------------------------------- */
/* Standard stable merge                                     */
/* --------------------------------------------------------- */

static void merge_runs(char *src,
                       char *dst,
                       size_t left,
                       size_t mid,
                       size_t right,
                       size_t elem_size,
                       racams_cmp_fn cmp)
{
    size_t i = left;
    size_t j = mid;
    size_t k = left;

    while (i < mid && j < right) {
        char *a = src + i * elem_size;
        char *b = src + j * elem_size;

        if (cmp(a, b) <= 0) {
            memcpy(dst + k * elem_size, a, elem_size);
            i++;
        } else {
            memcpy(dst + k * elem_size, b, elem_size);
            j++;
        }
        k++;
    }

    while (i < mid) {
        memcpy(dst + k * elem_size,
               src + i * elem_size,
               elem_size);
        i++;
        k++;
    }

    while (j < right) {
        memcpy(dst + k * elem_size,
               src + j * elem_size,
               elem_size);
        j++;
        k++;
    }
}

/* --------------------------------------------------------- */
/* Block merge                                               */
/* --------------------------------------------------------- */

static void block_merge_runs(racams_context *ctx,
                             char *src,
                             char *dst,
                             size_t left,
                             size_t mid,
                             size_t right,
                             size_t elem_size,
                             racams_cmp_fn cmp)
{
    char *lbuf = (char *)ctx->left_block;
    char *rbuf = (char *)ctx->right_block;

    size_t left_next = left;
    size_t right_next = mid;
    size_t out = left;

    size_t lcount = 0, rcount = 0;
    size_t li = 0, ri = 0;

    for (;;) {
        if (li == lcount && left_next < mid) {
            lcount = ctx->block_elems;
            if (lcount > (mid - left_next))
                lcount = (mid - left_next);

            memcpy(lbuf,
                   src + left_next * elem_size,
                   lcount * elem_size);

            left_next += lcount;
            li = 0;
        }

        if (ri == rcount && right_next < right) {
            rcount = ctx->block_elems;
            if (rcount > (right - right_next))
                rcount = (right - right_next);

            memcpy(rbuf,
                   src + right_next * elem_size,
                   rcount * elem_size);

            right_next += rcount;
            ri = 0;
        }

        int left_empty = (li == lcount);
        int right_empty = (ri == rcount);

        if (left_empty && right_empty)
            break;

        if (left_empty) {
            while (ri < rcount) {
                memcpy(dst + out * elem_size,
                       rbuf + ri * elem_size,
                       elem_size);
                ri++;
                out++;
            }

            while (right_next < right) {
                size_t take = ctx->block_elems;
                if (take > (right - right_next))
                    take = (right - right_next);

                memcpy(dst + out * elem_size,
                       src + right_next * elem_size,
                       take * elem_size);
                right_next += take;
                out += take;
            }
            break;
        }

        if (right_empty) {
            while (li < lcount) {
                memcpy(dst + out * elem_size,
                       lbuf + li * elem_size,
                       elem_size);
                li++;
                out++;
            }

            while (left_next < mid) {
                size_t take = ctx->block_elems;
                if (take > (mid - left_next))
                    take = (mid - left_next);

                memcpy(dst + out * elem_size,
                       src + left_next * elem_size,
                       take * elem_size);
                left_next += take;
                out += take;
            }
            break;
        }

        if (cmp(lbuf + li * elem_size,
                rbuf + ri * elem_size) <= 0) {
            memcpy(dst + out * elem_size,
                   lbuf + li * elem_size,
                   elem_size);
            li++;
        } else {
            memcpy(dst + out * elem_size,
                   rbuf + ri * elem_size,
                   elem_size);
            ri++;
        }
        out++;
    }
}
    

/* --------------------------------------------------------- */
/* Runtime L1 detection                                      */
/* --------------------------------------------------------- */

static size_t detect_l1_cache_bytes(void)
{
#if defined(__linux__)
    for (int i = 0; i < 8; i++) {
        char path[256];

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu0/cache/index%d/level", i);
        FILE *fp_level = fopen(path, "r");
        if (!fp_level) continue;

        int level;
        if (fscanf(fp_level, "%d", &level) != 1) {
            fclose(fp_level);
            continue;
        }
        fclose(fp_level);

        if (level != 1) continue;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu0/cache/index%d/type", i);
        FILE *fp_type = fopen(path, "r");
        if (!fp_type) continue;

        char type[32];
        if (fgets(type, sizeof(type), fp_type) == NULL) {
            fclose(fp_type);
            continue;
        }
        fclose(fp_type);

        if (strncmp(type, "Data", 4) != 0 &&
            strncmp(type, "Unified", 7) != 0)
            continue;

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu0/cache/index%d/size", i);
        FILE *fp_size = fopen(path, "r");
        if (!fp_size) continue;

        size_t value;
        char unit;

        if (fscanf(fp_size, "%zu%c", &value, &unit) == 2) {
            fclose(fp_size);


            if (unit == 'K') return value * 1024;
            if (unit == 'M') return value * 1024 * 1024;
            return value;
        }

        fclose(fp_size);
    }
#endif

#if defined(_WIN32)
    DWORD size = 0;
    GetLogicalProcessorInformationEx(RelationCache, NULL, &size);

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer =
        malloc(size);

    if (buffer &&
        GetLogicalProcessorInformationEx(RelationCache, buffer, &size)) {
        char *ptr = (char *)buffer;

        while (ptr < (char *)buffer + size) {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info =
                (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)ptr;

            if (info->Relationship == RelationCache) {
                CACHE_RELATIONSHIP cache = info->Cache;

                if (cache.Level == 1 && cache.Type == CacheData) {
                    size_t result = cache.CacheSize;
                    free(buffer);
                    return result;
                }
            }

            ptr += info->Size;
        }
    }

    free(buffer);
#endif

    printf("[RACAMS] Falling back to default 32KB L1\n");
    return 32 * 1024;
}

/* --------------------------------------------------------- */
/* Init                                                      */
/* --------------------------------------------------------- */

void racams_init(racams_context *ctx,
                 double cache_fraction,
                 size_t max_elems,
                 size_t elem_size)
{
    ctx->l1_bytes = detect_l1_cache_bytes();

    size_t usable_bytes = (size_t)(ctx->l1_bytes * cache_fraction);
    ctx->initial_block_elems = usable_bytes / (3 * elem_size);

    if (ctx->initial_block_elems < RACAMS_INSERTION_THRESHOLD)
        ctx->initial_block_elems = RACAMS_INSERTION_THRESHOLD;

    ctx->block_elems = ctx->initial_block_elems;
    ctx->last_pass_ns = 0.0;

    ctx->buffer_a = malloc(max_elems * elem_size);
    ctx->buffer_b = malloc(max_elems * elem_size);
    ctx->left_block  = malloc(ctx->initial_block_elems * elem_size);
    ctx->right_block = malloc(ctx->initial_block_elems * elem_size);

    if (!ctx->buffer_a || !ctx->buffer_b ||
        !ctx->left_block || !ctx->right_block) {
        fprintf(stderr, "[RACAMS] Failed to allocate buffers\n");
        exit(EXIT_FAILURE);
    }

}

/* --------------------------------------------------------- */
/* Sort                                                      */
/* --------------------------------------------------------- */

void racams_sort(racams_context *ctx,
                 void *base,
                 size_t n,
                 size_t elem_size,
                 racams_cmp_fn cmp)
{
    if (n <= 1)
        return;

    ctx->block_elems = ctx->initial_block_elems;
    ctx->last_pass_ns = 0.0;

    char *src = (char *)base;
    char *dst = (char *)ctx->buffer_a;

    /* Phase 1: insertion sort tiny runs */
    for (size_t start = 0; start < n; start += RACAMS_INSERTION_THRESHOLD) {
        size_t len = RACAMS_INSERTION_THRESHOLD;
        if (start + len > n)
            len = n - start;

        insertion_sort(src + start * elem_size,
                       len,
                       elem_size,
                       cmp);
    }

    /* Phase 2: iterative adaptive merge */
    for (size_t width = RACAMS_INSERTION_THRESHOLD;
         width < n;
         width *= 2) {

        double pass_start = racams_now_ns();

        for (size_t left = 0; left < n; left += 2 * width) {
            size_t mid = left + width;
            size_t right = left + 2 * width;

            if (mid > n) mid = n;
            if (right > n) right = n;

            if (mid >= right) {
                memcpy(dst + left * elem_size,
                       src + left * elem_size,
                       (right - left) * elem_size);
                continue;
            }

            size_t run_elems = right - left;

            if (run_elems <= ctx->block_elems) {
                merge_runs(src, dst,
                        left, mid, right,
                        elem_size, cmp);
            } else {
            #if RACAMS_USE_BLOCK_MERGE
                block_merge_runs(ctx, src, dst,
                                left, mid, right,
                                elem_size, cmp);
            #else
                merge_runs(src, dst,
                        left, mid, right,
                        elem_size, cmp);
            #endif
            }

        }

        double pass_end = racams_now_ns();
        double pass_ns = pass_end - pass_start;

        #if RACAMS_USE_TIMING_ADAPT
        if (ctx->last_pass_ns > 0.0 &&
            pass_ns > ctx->last_pass_ns * RACAMS_ADAPT_RATIO &&
            ctx->block_elems > RACAMS_INSERTION_THRESHOLD * 2) {

            size_t new_block =
                (ctx->block_elems * RACAMS_BLOCK_SHRINK_NUM) /
                RACAMS_BLOCK_SHRINK_DEN;

            if (new_block < RACAMS_INSERTION_THRESHOLD)
                new_block = RACAMS_INSERTION_THRESHOLD;

            if (new_block < ctx->block_elems) {
        #if RACAMS_DEBUG_ADAPT
                printf("[RACAMS] block_elems shrank from %zu to %zu\n",
                    ctx->block_elems, new_block);
        #endif
                ctx->block_elems = new_block;
            }
        }
        #endif

        ctx->last_pass_ns = pass_ns;

        /* swap ping-pong buffers */
        char *tmp = src;
        src = dst;
        dst = tmp;
    }

    if (src != (char *)base) {
        memcpy(base, src, n * elem_size);
    }
}

/* --------------------------------------------------------- */
/* Built-in double comparator                               */
/* --------------------------------------------------------- */

int racams_cmp_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

/* --------------------------------------------------------- */
/* Free                                                      */
/* --------------------------------------------------------- */

void racams_free(racams_context *ctx)
{
    free(ctx->buffer_a);
    free(ctx->buffer_b);
    free(ctx->left_block);
    free(ctx->right_block);

    ctx->buffer_a = NULL;
    ctx->buffer_b = NULL;
    ctx->left_block = NULL;
    ctx->right_block = NULL;
}