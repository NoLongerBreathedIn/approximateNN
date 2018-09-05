#include <stdio.h>
#include <stdlib.h>
#include "ann.h"

// Arguments should be in the order:
// n k d t rb rlb ra rla use_cpu data results dists save
// Last two are optional.
// Last four are filenames, rest are integers.
int main(int argc, char **argv) {
  if(argc < 12)
    exit(1);
  size_t n, k, d, rb, rlb, ra, rla;
  char ucpu;
  int t;
  n = atol(argv[1]);
  k = atol(argv[2]);
  d = atol(argv[3]);
  t = atoi(argv[4]);
  rb = atol(argv[5]);
  rlb = atol(argv[6]);
  ra = atol(argv[7]);
  rla = atol(argv[8]);
  ucpu = atoi(argv[9]);
  FILE *data_f = fopen(argv[10], "rb");
  FILE *results_f = fopen(argv[11], "wb");
  FILE *dists_f = NULL;
  FILE *save_f = NULL;
  save_t save, *save_p = NULL;
  ftype *dists, **dists_p = NULL;
  if(argc > 12) {
    dists_f = fopen(argv[12], "wb"), dists_p = &dists;
    if(argc > 13)
      save_f = fopen(argv[13], "wb"), save_p = &save;
  }
  ftype *data = malloc(sizeof(ftype) * n * d);
  fread(data, sizeof(ftype), n * d, data_f);
  fclose(data_f);
  size_t *results = precomp(n, k, d, data, t, rb, rlb, ra, rla,
			    save_p, dists_p, ucpu);
  free(data);
  fwrite(results, sizeof(size_t), n * k, results_f);
  fclose(results_f);
  free(results);
  
  if(argc > 12) {
    fwrite(dists, sizeof(ftype), n * k, dists_f);
    fclose(dists_f);
    free(dists);
    if(argc > 13) {
      write_save(&save, save_f);
      fclose(save_f);
      free_save(&save);
    }
  }
}
