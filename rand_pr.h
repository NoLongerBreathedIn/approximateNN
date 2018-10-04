#ifndef RAND_PR
#define RAND_PR
#include <stdlib.h>
#include "ftype.h"

extern void rand_rot(size_t rotlen, size_t d,
		     size_t *is, size_t *js, ftype *as, size_t *wkspc);
extern void rand_perm(size_t d_pre, size_t d_post, size_t *perm);

#endif
