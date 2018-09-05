#include "ann_save.h"
#include <stdlib.h>

void free_save(save_t *save) {
  for(int i = 0; i < save->tries; i++) {
    free(save->which_par[i]);
  }
  free(save->which_par);
  free(save->par_maxes);
  free(save->graph);
  free(save->row_means);
  free(save->bases);
}

void write_save(const save_t *save, FILE *f) {
  fwrite(&save->tries, sizeof(int), 1, f);
  fwrite(&save->n, sizeof(size_t), 1, f);
  fwrite(&save->k, sizeof(size_t), 1, f);
  fwrite(&save->d_short, sizeof(size_t), 1, f);
  fwrite(&save->d_long, sizeof(size_t), 1, f);
  fwrite(save->par_maxes, sizeof(size_t), save->tries, f);
  fwrite(save->graph, sizeof(size_t), save->n * save->k, f);
  for(int i = 0; i < save->tries; i++)
    fwrite(save->which_par[i], sizeof(size_t),
	   save->par_maxes[i] << save->d_short, f);
  fwrite(save->row_means, sizeof(ftype), save->d_long, f);
  fwrite(save->bases, sizeof(ftype),
	 save->tries * save->d_long * save->d_short, f);
}

#define mallocread(p, t, c, f) \
  fread(p = malloc(sizeof(t) * (c)), sizeof(t), c, f)

void read_save(save_t *save, FILE *f) {
  fread(&save->tries, sizeof(int), 1, f);
  fread(&save->n, sizeof(size_t), 1, f);
  fread(&save->k, sizeof(size_t), 1, f);
  fread(&save->d_short, sizeof(size_t), 1, f);
  fread(&save->d_long, sizeof(size_t), 1, f);
  mallocread(save->par_maxes, size_t, save->tries, f);
  mallocread(save->graph, size_t, save->n * save->k, f);
  save->which_par = malloc(sizeof(size_t *) * save->tries);
  for(int i = 0; i < save->tries; i++)
    mallocread(save->which_par[i], size_t,
	       save->par_maxes[i] << save->d_short, f);
  mallocread(save->row_means, ftype, save->d_long, f);
  mallocread(save->bases, ftype,
	     save->tries * save->d_long * save->d_short, f);
}
