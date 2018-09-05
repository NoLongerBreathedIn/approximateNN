#ifndef ANN_SAVE
#define ANN_SAVE
#include <stdio.h>
#include "ftype.h"
// A save data structure for randomised approximate nearest neighbor queries.
// Treat as opaque.
typedef struct {
  int tries;
  size_t n, k, d_short, d_long, **which_par, *par_maxes, *graph;
  ftype *row_means, *bases;
} save_t;

// Frees a save data structure.
extern void free_save(save_t *save);
// Write a save data structure to a file
extern void write_save(const save_t *save, FILE *f);
// Reads a save data structure from a file
extern void read_save(save_t *save, FILE *f);

#endif
