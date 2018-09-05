#include "ann_ext.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

static char *dir = NULL;
static char *dirend = NULL;

static void compdir(void) {
  if(dir == NULL) {
    size_t s = strlen(__FILE__)
    dir = malloc(s + 1);
    strcpy(dir, s);
    char *f = dir + s;
    while(*f != '/') f--;
    dirend = f + 1;
  }
}

size_t *precomp_ext(size_t n, size_t k, size_t d, const ftype *points, 
		    int tries, size_t rots_before, size_t rot_len_before,
		    size_t rots_after, size_t rot_len_after,
		    save_t *save, ftype **dists, char use_cpu) {
  compdir();
  char *args[15];
  args[0] = "precomp";
  for(int i = 1; i < 14; i++)
    args[i] = malloc(23);
  sprintf(args[1], "%lu", n);
  sprintf(args[2], "%lu", k);
  sprintf(args[3], "%lu", d);
  sprintf(args[4], "%i", t);
  sprintf(args[5], "%lu", rots_before);
  sprintf(args[6], "%lu", rot_len_before);
  sprintf(args[7], "%lu", rots_after);
  sprintf(args[8], "%lu", rot_len_after);
  sprintf(args[9], "%i", use_cpu);
  strcpy(args[10], "dataXXXXXX");
  strcpy(args[11], "resultsXXXXXX");
  strcpy(args[12], "distsXXXXXX");
  strcpy(args[13], "saveXXXXXX");
  args[14] = NULL;
  int fs[4];
  for(int i = 0; i < 4; i++)
    fs[i] = mkstemp(args[i + 10]);
  FILE *f = fdopen(fs[0], "wb");
  fwrite(points, sizeof(ftype), n * d, f);
  fclose(f);
  pid_t c = fork();
  if(c == -1)
    return(NULL); // something's wrong
  if(!c) {
    for(int i = 1; i < 4; i++)
      close(fs[i]);
    if(save == NULL) {
      free(args[13]), args[13] = NULL;
      if(dists == NULL)
	free(args[12]), args[12] = NULL;
    }
    strcpy(dirend, "precomp");
    execv(dir, args);
  }
  waitpid(c, NULL, 0);
  size_t *results = malloc(sizeof(size_t) * n * k);
  f = fdopen(fs[1], "rb");
  fread(results, sizeof(size_t), n * k, f);
  fclose(f);
  if(dists != NULL) {
    *dists = malloc(sizeof(ftype) * n * k);
    f = fdopen(fs[2], "rb");
    fread(*dists, sizeof(ftype), n * k, f);
    fclose(f);
  }
  if(save != NULL) {
    f = fdopen(fs[3], "rb");
    read_save(save, f);
    fclose(f);
  }
  for(int i = 0; i < 4; i++)
    unlink(args[i + 10]);
  for(int i = 1; i < 14; i++)
    free(args[i]);
  return(results);
}

size_t *query_ext(const save_t *save, const ftype *points,
		  size_t ycnt, const ftype *y, ftype **dists, char use_cpu) {
  compdir();
  char *args[9];
  args[0] = "query";
  for(int i = 1; i < 8; i++)
    args[i] = malloc(22);
  strcpy(args[1], "saveXXXXXX");
  strcpy(args[2], "dataXXXXXX");
  sprintf(args[3], "%lu", ycnt);
  strcpy(args[4], "yptsXXXXXX");
  sprintf(args[5], "%i", use_cpu);
  strcpy(args[6], "resultXXXXX");
  strcpy(args[7], "distXXXXXX");
  args[8] = NULL;
  FILE *f = fdopen(mkstemp(args[1]), "wb");
  write_save(save, f);
  fclose(f);
  f = fdopen(mkstemp(args[2]), "wb");
  fwrite(points, sizeof(ftype), save->n * save->d_long, f);
  fclose(f);
  f = fdopen(mkstemp(args[4]), "wb");
  fwrite(y, sizeof(ftype), ycnt * save->d_long, f);
  fclose(f);
  int resd = mkstemp(args[6]);
  int disd = mkstemp(args[7]);
  pid_t c = fork();
  if(c == -1)
    return(NULL);
  if(!c) {
    if(dists == NULL)
      free(args[7]), args[7] = NULL;
    strcpy(dirend, "query");
    execv(dir, args);
  }
  waitpid(c, NULL, 0);
  size_t *results = malloc(sizeof(size_t) * ycnt * save->k);
  f = fdopen(resd, "rb");
  fread(results, sizeof(size_t), ycnt * save->k, f);
  fclose(f);
  if(dists != NULL) {
    *dists = malloc(sizeof(ftype) * ycnt * save->k);
    f = fdopen(dist, "rb");
    fread(*dists, sizeof(ftype), ycnt * save->k, f);
    fclose(f);
  }
  for(int i = 1; i < 8; i++) {
    if(i != 3 && i != 5)
      unlink(args[i]);
    free(args[i]);
  }
  return(results);
}
