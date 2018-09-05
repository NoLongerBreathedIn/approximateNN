#include <stdio.h>
#include <stdlib.h>
#include "ann.h"

// Arguments should be in the order:
// save pts ycnt y use_cpu results dists
// Last is optional.
// ycnt and use_cpu are integers, rest are filenames.
int main(int argc, char **argv) {
  if(argc < 7)
    exit(1);
  FILE *f = fopen(argv[1], "rb");
  save_t save;
  read_save(&save, f);
  fclose(f);
  size_t k = save.k;
  f = fopen(argv[2], "rb");
  ftype *pts = malloc(sizeof(ftype) * save.n * save.d_long);
  fread(pts, sizeof(ftype), save.n * save.d_long, f);
  fclose(f);
  size_t ycnt = atol(argv[3]);
  ftype *y = malloc(sizeof(ftype) * ycnt * save.d_long);
  f = fopen(argv[4], "rb");
  fread(y, sizeof(ftype), ycnt * save.d_long, f);
  fclose(f);
  char ucpu = atoi(argv[5]);
  ftype *dists;
  size_t *res;
  res = query(&save, pts, ycnt, y, argc > 7? &dists : NULL, ucpu);
  free_save(&save);
  free(pts);
  free(y);
  f = fopen(argv[6], "wb");
  fwrite(res, sizeof(size_t), ycnt * k, f);
  fclose(f);
  if(argc > 7) {
    f = fopen(argv[7], "wb");
    fwrite(dists, sizeof(ftype), ycnt * k, f);
    fclose(f);
  }
}
