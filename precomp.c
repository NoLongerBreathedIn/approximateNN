#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ann.h"

#define readstdin(x) fread(&(x), sizeof(x), 1, stdin)

// Arguments:
// results dists save (all filenames)
// Reads from stdin:
// n k d t rb rlb ra rla use_cpu data
// Types: size_t, except t is int, use_cpu is char, data is array of ftype
int main(int argc, char **argv) {
  if(argc < 2)
    exit(1);
  size_t n, k, d, rb, rlb, ra, rla;
  char ucpu;
  int t;
  
  readstdin(n);
  readstdin(k);
  readstdin(d);
  readstdin(t);
  readstdin(rb);
  readstdin(rlb);
  readstdin(ra);
  readstdin(rla);
  readstdin(ucpu);

  save_t save;
  ftype *dists;
  ftype *data = malloc(sizeof(ftype) * n * d);
  fread(data, sizeof(ftype), n * d, stdin);
  srandom(time(NULL));
  size_t *results = precomp(n, k, d, data, t, rb, rlb, ra, rla,
			    argc > 3? &save : NULL,
			    argc > 2? &dists : NULL, ucpu);
  free(data);
  FILE *f = fopen(argv[1], "wb");
  if(f == NULL)
    return(1);
  if(fwrite(results, sizeof(size_t), n * k, f) != n * k)
    return(1);
  fclose(f);
  free(results);
  
  if(argc > 2) {
    f = fopen(argv[2], "wb");
    if(f == NULL)
      return(1);
    if(fwrite(dists, sizeof(ftype), n * k, f) != n * k)
      return(1);
    fclose(f);
    free(dists);
    if(argc > 3) {
      f = fopen(argv[3], "wb");
      if(f == NULL)
	return(1);
      char c = write_save(&save, f);
      fclose(f);
      free_save(&save);
      return(c);
    }
  }
}
