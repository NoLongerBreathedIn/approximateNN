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

#define checked_fwrite(p, t, c, f) \
  if(fwrite(p, sizeof(t), c, f) != (size_t)(c)) return(1)
#define checked_fread(p, t, c, f) \
  if(fread(p, sizeof(t), c, f) != (size_t)(c)) return(1)

char write_save(const save_t *save, FILE *f) {
  checked_fwrite(&save->tries, int, 1, f);
  checked_fwrite(&save->n, size_t, 1, f);
  checked_fwrite(&save->k, size_t, 1, f);
  checked_fwrite(&save->d_short, size_t, 1, f);
  checked_fwrite(&save->d_long, size_t, 1, f);
  checked_fwrite(save->par_maxes, size_t, save->tries, f);
  checked_fwrite(save->graph, size_t, save->n * save->k, f);
  for(int i = 0; i < save->tries; i++)
    checked_fwrite(save->which_par[i], size_t,
		   save->par_maxes[i] << save->d_short, f);
  checked_fwrite(save->row_means, ftype, save->d_long, f);
  checked_fwrite(save->bases, ftype,
		 save->tries * save->d_long * save->d_short, f);
  return(0);
}

#define checked_malloc(p, t, c) \
  if(((p) = malloc(sizeof(t) * (c))) == NULL) return(1)

#define mallocread(p, t, c, f) \
  checked_malloc(p, t, c); else \
  checked_fread(p, sizeof(t), c, f)

#define free_safe(p) if(p != NULL) free(p)
    
static char read_save_int(save_t *save, FILE *f) {
    checked_fread(&save->tries, int, 1, f);
    checked_fread(&save->n, size_t, 1, f);
    checked_fread(&save->k, size_t, 1, f);
    checked_fread(&save->d_short, size_t, 1, f);
    checked_fread(&save->d_long, size_t, 1, f);
    mallocread(save->par_maxes, size_t, save->tries, f);
    mallocread(save->graph, size_t, save->n * save->k, f);
    checked_malloc(save->which_par, size_t *, save->tries);
    for(int i = 0; i < save->tries; i++)
      save->which_par[i] = NULL;
    for(int i = 0; i < save->tries; i++)
      mallocread(save->which_par[i], size_t,
		 save->par_maxes[i] << save->d_short, f);
    mallocread(save->row_means, ftype, save->d_long, f);
    mallocread(save->bases, ftype,
	       save->tries * save->d_long * save->d_short, f);
    return(0);
}

char read_save(save_t *save, FILE *f) {
  save->tries = 0;
  save->par_maxes = save->graph = NULL;
  save->which_par = NULL;
  save->row_means = save->bases = NULL;
  
  char r = read_save_int(save, f);
  if(r) {
    free_safe(save->bases);
    free_safe(save->row_means);
    if(save->which_par != NULL)
      for(int i = 0; i < save->tries; i++)
	free_safe(save->which_par[i]);
    free_safe(save->which_par);
    free_safe(save->graph);
    free_safe(save->par_maxes);
  }
  return(r);
}
