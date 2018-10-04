#include "rand_pr.h"
#include <stdlib.h>
#include <limits.h>
#include <math.h>

static const double max_long_p1 = (double)RAND_MAX + 1;

#define rand_dbl() ((double)(unsigned long)random() / max_long_p1)

void rand_rot(size_t rotlen, size_t d,
	      size_t *is, size_t *js, ftype *as, size_t *wkspc) {
  rand_perm(rotlen * 2, d, wkspc);
  for(size_t i = 0; i < rotlen; i++)
    is[i] = wkspc[2 * i], js[i] = wkspc[2 * i + 1], as[i] = rand_dbl() * M_PI;
}

void rand_perm(size_t d_pre, size_t d_post, size_t *perm) {
  for(size_t i = 0; i < d_post; i++)
    perm[i] = i;
  for(size_t i = 0; i < d_pre; i++) {
    size_t j = (unsigned long)random() % (d_post - i) + i;
    if(j != i) {
      size_t t = perm[i];
      perm[i] = perm[j];
      perm[j] = t;
    }
  }
}
