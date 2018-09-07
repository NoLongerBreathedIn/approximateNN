#include <stdio.h>
#include <stdlib.h>
#include "ann.h"

// Arguments:
// results dists (filenames, second is optional)
// Reads from stdin (in order):
// save pts ycnt y use_cpu
// save is save_t; pts is ftype[]; ycnt is size_t; y is ftype; use_cpu is char.
int main(int argc, char **argv) {
  if(argc < 2)
    exit(1);
  save_t save;
  read_save(&save, stdin);
  size_t k = save.k;
  ftype *pts = malloc(sizeof(ftype) * save.n * save.d_long);
  fread(pts, sizeof(ftype), save.n * save.d_long, stdin);
  size_t ycnt;
  fread(&ycnt, sizeof(size_t), 1, stdin);
  ftype *y = malloc(sizeof(ftype) * ycnt * save.d_long);
  fread(y, sizeof(ftype), ycnt * save.d_long, stdin);
  char ucpu;
  fread(&ucpu, 1, 1, stdin);
  ftype *dists;
  size_t *res;
  res = query(&save, pts, ycnt, y, argc > 2? &dists : NULL, ucpu);
  free_save(&save);
  free(pts);
  free(y);
  FILE *f = fopen(argv[1], "wb");
  if(f == NULL)
    return(1);
  if(fwrite(res, sizeof(size_t), ycnt * k, f) != ycnt * k)
    return(1);
  fclose(f);
  if(argc > 2) {
    if((f = fopen(argv[2], "wb")) == NULL)
      return(1);
    if(fwrite(dists, sizeof(ftype), ycnt * k, f) != ycnt * k)
      return(1);
    fclose(f);
  }
}
