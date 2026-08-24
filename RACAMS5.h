#ifndef RACAMS_H
#define RACAMS_H

#include <stddef.h>     

typedef int (*racams_cmp_fn)(const void *, const void *);

typedef struct {
    size_t l1_bytes;               /* detected L1 cache size */
    size_t initial_block_elems;    /* starting block size */
    size_t block_elems;            /* current adaptive block size */

    void *buffer_a;                /* ping buffer */
    void *buffer_b;                /* pong buffer */

    void *left_block;              /* left block buffer */
    void *right_block;             /* right block buffer */

    double last_pass_ns;           /* previous merge-pass time */
} racams_context;

void racams_init(racams_context *ctx,
                 double cache_fraction,
                 size_t max_elems,
                 size_t elem_size);

void racams_free(racams_context *ctx);

void racams_sort(racams_context *ctx,
                 void *base,
                 size_t n,
                 size_t elem_size,
                 racams_cmp_fn cmp);

int racams_cmp_double(const void *a, const void *b);

#endif