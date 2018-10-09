#include <string.h>

#define conc(a, b, c) a ## b ## c
#define concb(a, b, c) conc(a, b, c)
#define MK_NAME(x) concb(x, _, TYPE_OF_COMP)
// hack to allow two versions of a function to be defined in separate files
// with separate names, but only slightly different.

static unsigned lg(size_t d) {
  unsigned r = (d > 0xFFFFFFFF) << 5;
  d >>= r;
  unsigned s = (d > 0xFFFF) << 4;
  d >>= s, r |= s;
  d >>= s = (d > 0xFF) << 3, r |= s;
  d >>= s = (d > 0xF) << 2, r |= s;
  d >>= s = (d > 3) << 1;
  return(r | s | d >> 1);
}


// Hacks to skip first (two) arguments if they are OpenCL types
#ifndef ocl2c
#define FST_GONLY_INT(x, ...) x(__VA_ARGS__)
#define TWO_GONLY_INT(x, ...) x(__VA_ARGS__)
#else
#define FST_GONLY_INT(x, y, ...) x(__VA_ARGS__)
#define TWO_GONLY_INT(x, y, z, ...) x(__VA_ARGS__)
#endif

#define FST_GONLY(x, ...) FST_GONLY_INT(MK_NAME(x), __VA_ARGS__)
#define TWO_GONLY(x, ...) TWO_GONLY_INT(MK_NAME(x), __VA_ARGS__)

void MK_NAME(mk_rot_inf)(size_t rot_len, size_t rots,
			 size_t dim, size_t *wkspc, size_t *cout,
			 ftype *aout) {
  for(size_t j = 0; j < rots; j++)
    rand_rot(rot_len, dim, cout + j * 2 * rot_len,
	     cout + (j * 2 + 1) * rot_len,
	     aout + j * rot_len, wkspc);
}

void MK_NAME(mk_orth_inf)(size_t rot_len_b, size_t rots_b,
			  size_t rot_len_a, size_t rots_a,
			  size_t dim_low, size_t dim_high, size_t dim_max,
			  size_t *wkspc, size_t *cout, ftype *aout) {
  MK_NAME(mk_rot_inf)(rot_len_b, rots_b, dim_high, wkspc, cout, aout);
  aout += rot_len_b * rots_b;
  cout += rot_len_b * rots_b * 2;
  rand_perm(dim_high, dim_max, cout);
  cout += dim_max;
  rand_perm(dim_low, dim_low, cout);
  cout += dim_low;
  MK_NAME(mk_rot_inf)(rot_len_a, rots_a, dim_low, wkspc, cout, aout);
}

void FST_GONLY(walsh, cl_command_queue q,
	       size_t d, size_t n, size_t w, BUFTYPE(ftype) a) {
  // 1 / sqrt(2)
  static const ftype rsr = .7071067811865475244008443621048490392848;
  if(d == 1)
    return;
  int l = lg(d);
  for(int i = l - 1; i >= 0; i--) {
    size_t r = ~(size_t)0 << (i + 1);
    size_t nth = (w + ~r & r) >> 1;
    // The idea here is that due to the order,
    // we can efficiently figure out which iterations we need to do
    // to find the first w coordinates at the end.
    LOOP2(q, apply_walsh_step(l, i, rsr, a), n, nth);
  }
}

void FST_GONLY(add_up_rows, cl_command_queue q,
	       size_t d, size_t n,
	       BUFTYPE(ftype) points, BUFTYPE(ftype) sums) {
  LOOP2(q, add_rows_step_0(d, n, points, sums), n/2, d);
  for(size_t m = n >> 1; m >> 1; m >>= 1)
    LOOP2(q, add_rows_step_n(d, m, sums), m/2, d);
}

void FST_GONLY(add_up_cols, cl_command_queue q, size_t d, size_t k,
	       size_t skip, size_t n, BUFTYPE(ftype) mat, BUFTYPE(ftype) out) {
  for(size_t l = d; l >> 1; l >>= 1)
    LOOP3(q, add_cols_step(d, l, k - skip, mat), n, k - skip, l / 2);
  fin_add_cols(q, d, k, skip, mat, out, n);
}

void FST_GONLY(do_sort, cl_command_queue q, size_t k, size_t n,
	     BUFTYPE(size_t) along, BUFTYPE(ftype) order) {
  int lk = lg(k);
  size_t yms[lk];
  for(int s = 0; s < lk; s++)
    yms[s] = (k >> (s + 1)) << s | (k & (1 << s) - 1 & -(k >> s & 1));
  // That crazy expression computes the maximum y (plus 1) such that,
  // if yl = y & (1 << s) - 1, yh = y ^ yl,
  // (y_high << 1 | 1 << s | yl) < k.
  // Basically, we want either k >> 1 & ~((1 << s) - 1),
  // or that inclusive or k & ((1 << s) - 1).
  // We want the latter if k >> s is odd.
  // Hence the work. Oi.
  for(int s = 0; s < lk; s++)
    for(int ss = s; ss >= 0; ss--)
      LOOP2(q, sort_two_step(k, s, ss, along, order), n, yms[ss]);
}

void FST_GONLY(apply_rot, cl_command_queue q,
	       size_t n, size_t d, size_t i,
	       size_t rot_len,
	       size_t aoff, size_t doff,
	       BUFTYPE(const ftype) rot_angs,
	       BUFTYPE(const size_t) ortho_dat,
	       BUFTYPE(ftype) points, char swap) {
  BUFTYPE(const ftype) as =
    MK_SUBBUF_RO_NA_REG(ftype, rot_angs, aoff + i * rot_len, rot_len);
  BUFTYPE(const size_t) is =
    MK_SUBBUF_RO_NA_REG(size_t, ortho_dat,
			doff + (i * 2 + swap) * rot_len, rot_len);
  BUFTYPE(const size_t) js =
    MK_SUBBUF_RO_NA_REG(size_t, ortho_dat,
			doff + (i * 2 + 1 - swap) * rot_len, rot_len);
  LOOP2(q, apply_rotation(d, is, js, as, points), n, rot_len);
  relMemU(as);
  relMemU(is);
  relMemU(js);
}


// Undoes the random orthogonal operation and projection (supplied),
// on the basis vectors of the output (I.E., the input represents the
// d_high by d_low matrix M, we want M^T)
void FST_GONLY(save_vecs, cl_command_queue q,
	       size_t d_low, size_t d_high, size_t d_max,
	       size_t rots_b, size_t rot_len_b,
	       size_t rots_a, size_t rot_len_a,
	       size_t ang_off, size_t d_off,
	       BUFTYPE(const ftype) rot_angs,
	       BUFTYPE(const size_t) ortho_dat,
	       const ftype *vcs,
	       BUFTYPE(ftype) vecs, BUFTYPE(ftype) vecs2) {
  enqueueWriteBuf(q, sizeof(ftype) * d_low * d_low, vcs, vecs);
  for(long i = rots_a - 1; i >= 0; i--)
    FST_GONLY(apply_rot, q, d_low, d_low, i, rot_len_a,
	      ang_off + rots_b * rot_len_b,
	      d_off + rots_b * rot_len_b * 2 + d_max + d_low,
	      rot_angs, ortho_dat, vecs, 1);
  fillZeroes(q, vecs2, d_low * d_max * sizeof(ftype));
  BUFTYPE(const size_t) perm_ai =
    MK_SUBBUF_RO_NA_REG(size_t, ortho_dat,
			d_off + rots_b * rot_len_b * 2 + d_max, d_low);  
  LOOP2(q, apply_permutation(d_low, d_max, perm_ai, vecs, vecs2),
	    d_low, d_low);
  relMemU(perm_ai);
  FST_GONLY(walsh, q, d_max, d_low, d_max, vecs2);
  BUFTYPE(const size_t) perm_b =
    MK_SUBBUF_RO_NA_REG(size_t, ortho_dat, d_off + rots_b * rot_len_b * 2,
			d_max);
  LOOP2(q, apply_perm_inv(d_max, d_high, perm_b, vecs2, vecs),
	d_low, d_max);
  for(long i = rots_b - 1; i >= 0; i--)
    FST_GONLY(apply_rot, q, d_low, d_high, i, rot_len_b,
	      ang_off, d_off, rot_angs, ortho_dat, vecs, 1);
}

// Pass it a pair of matrices, both n by k, one of ftypes,
// the other of size_ts.
// Sorts both by the ftypes along the columns,
// but of all entries with the same size_t,
// at most one will remain (the rest have the ftype set to infinity).
void FST_GONLY(sort_and_uniq, cl_command_queue q, size_t n,
		 size_t k, BUFTYPE(size_t) along,
		 BUFTYPE(ftype) order) {
  FST_GONLY(do_sort, q, k, n, along, order);
  LOOP2(q, rdups(k, along, order), n, k - 1);
  FST_GONLY(do_sort, q, k, n, along, order);
}

// Ugh, I dunno how to describe this. It computes certain distances.
void FST_GONLY(compdists, cl_command_queue q,
	       size_t n, size_t k, size_t d, size_t ycnt, size_t s,
	       BUFTYPE(const ftype) y, BUFTYPE(const ftype) points,
	       BUFTYPE(size_t) pointers, BUFTYPE(ftype) dists,
	       BUFTYPE(ftype) space) {
  LOOP3(q, compute_diffs_squared(d, k, n, s, pointers, y, points, space),
	ycnt, k - s, d);
  FST_GONLY(add_up_cols, q, d, k, s, ycnt, space, dists);
}

// This (a) figures out what the candidates for near neighbors are,
// and (b) computes the distances and sorts, then returns the k nearest.
void TWO_GONLY(second_half, cl_context c, cl_command_queue q,
	       size_t n, size_t k, size_t d_low, size_t d_high,
	       save_t *save, int i, int tries,
	       const size_t *sgns,
	       size_t *counts, BUFTYPE(size_t) which,
	       BUFTYPE(size_t) which_d,
	       size_t *wh,
	       BUFTYPE(const ftype) points,
	       BUFTYPE(size_t) pointers_out,
	       BUFTYPE(ftype) dists_out,
	       BUFTYPE(ftype) space,
	       BUFTYPE(ftype) dists) {
  BUFTYPE(const size_t) signs = MK_BUF_USE_RO_NA(c, size_t, n, sgns);
  size_t tmax = counts[0];
  for(size_t j = 1; j < 1 << d_low; j++)
    if(tmax < counts[j])
      tmax = counts[j];
  for(size_t j = 0; j < 1 << d_low; j++)
    for(size_t l = counts[j]; l < tmax; l++)
      wh[j * tmax + l] = n;
  for(size_t j = 0; j < n; j++)
    wh[sgns[j] * tmax + --counts[sgns[j]]] = j;
  enqueueWriteBuf(q, sizeof(size_t) * tmax << d_low, wh, which);
  if(save != NULL) {
    save->which_par[i] = malloc(sizeof(size_t) * tmax << d_low);
    memcpy(save->which_par[i], wh, sizeof(size_t) * tmax << d_low);
    save->par_maxes[i] = tmax;
  }
  LOOP3(q, compute_which(d_low, tmax, signs, which, which_d),
	n, d_low + 1, tmax);
  relMemU(signs);
  FST_GONLY(compdists, q, n, (d_low + 1) * tmax, d_high, n, 0,
	    points, points, which_d, dists, space);
  FST_GONLY(sort_and_uniq, q, n, (d_low + 1) * tmax, which_d, dists);
  enqueueCopy2D(q, size_t, (d_low + 1) * tmax, k * tries, k * i, which_d,
		pointers_out, n, k);
  enqueueCopy2D(q, ftype, (d_low + 1) * tmax, k * tries, k * i, dists,
		dists_out, n, k);
}

// Result is not guaranteed to be filled in, so wait on q.
// pointers, dists should be ycnt by len;
// graph should be n by k or equal to pointers;
// y should be ycnt by d,
// points should be n by d.
// distspace and ptrspace should be at least ycnt by k(k+1);
// space should be at least ycnt by k by d.
// Releases pointers, dists, and *space, but nothing else.
// Sorts, tosses all but top k,
// supercharges, recomputes distances, sorts,
// tosses all but top k, returns new guesses.
size_t *FST_GONLY(det_results, cl_command_queue q,
		  size_t n, size_t k, size_t d, size_t ycnt, size_t len,
		  BUFTYPE(size_t) pointers, BUFTYPE(ftype) dists,
		  BUFTYPE(const size_t) graph, BUFTYPE(const ftype) y,
		  BUFTYPE(const ftype) points, ftype **dists_o,
		  BUFTYPE(ftype) space, BUFTYPE(ftype) distspace,
		  BUFTYPE(size_t) ptrspace) {
  FST_GONLY(sort_and_uniq, q, ycnt, len, pointers, dists);
  enqueueCopy2D(q, size_t, len, k * (k + 1), 0, pointers, ptrspace, ycnt, k);
  LOOP3(q, supercharge(n, len, pointers == graph? len : k, k,
		       pointers, graph, ptrspace), ycnt, k, k);
  relMem(pointers);
  enqueueCopy2D(q, ftype, len, k * (k + 1), 0, dists, distspace, ycnt, k);
  FST_GONLY(compdists, q, n, k * (k + 1), d, ycnt, k,
	    y, points, ptrspace, distspace, space);
  relMem(dists);
  FST_GONLY(sort_and_uniq, q, ycnt, k * (k + 1), ptrspace, distspace);
  relMem(space);
  clFinish(q); // For some reason was crashing if this wasn't called here.
  if(dists_o != NULL) {
    LOOP2(q, sqrtip(k * (k + 1), distspace), ycnt, k);
    *dists_o = malloc(sizeof(ftype) * ycnt * k);
    enqueueRead2D(q, ftype, k * (k + 1), k, 0, distspace, *dists_o, ycnt, k);
  }
  relMem(distspace);
  size_t *results = malloc(sizeof(size_t) * ycnt * k);
  enqueueRead2D(q, size_t, k * (k + 1), k, 0, ptrspace, results, ycnt, k);
  relMem(ptrspace);
  return(results);
}

void FST_GONLY(prod_and_sign, cl_command_queue q,
	       size_t n, size_t d, size_t k, size_t t,
	       BUFTYPE(const ftype) y, BUFTYPE(const ftype) b,
	       BUFTYPE(ftype) psp, BUFTYPE(ftype) ssp,
	       BUFTYPE(size_t) rsp, BUFTYPE(size_t) rout) {
  LOOP3(q, prods(d, k * t, y, b, psp), n, k * t, d);
  FST_GONLY(add_up_cols, q, d, k, 0, t * n, psp, ssp);
  LOOP1(q, compute_signs(k, ssp, rsp), t * n);
  LOOP2(q, transpose(n, t, rsp, rout), t, n);
}

/* Starting point: */
/* We have an array, points, that is n by d_long. */
/* We also have save, which is a save structure. */
size_t *MK_NAME(precomp) (size_t n, size_t k, size_t d, const ftype *points,
			  int tries, size_t rots_before, size_t rot_len_before,
			  size_t rots_after, size_t rot_len_after,
			  save_t *save, ftype **dists_o) {
  setup();
  size_t d_short = ceil(log2((ftype)n / k));
  size_t d_max = d - 1;
  d_max |= d_max >> 1;
  d_max |= d_max >> 2;
  d_max |= d_max >> 4;
  d_max |= d_max >> 8;
  d_max |= d_max >> 16;
  d_max |= d_max >> 32;
  d_max++;
  if(d_short > d)
    d_short = d;
  if(rot_len_before * 2 > d)
    rot_len_before = d / 2;
  if(rot_len_after * 2 > d_max)
    rot_len_after = d_max / 2;
  MAKE_COMMAND_QUEUE(gpu_context, the_gpu, NULL, NULL, q);
  BUFTYPE(ftype) pnts =
    MK_BUF_COPY_RW_NA(gpu_context, ftype, n * d, points);
  BUFTYPE(ftype) row_sums;
  row_sums = MK_BUF_RW_RO(gpu_context, ftype, (n/2) * d);  
  FST_GONLY(add_up_rows, q, d, n, pnts, row_sums);  
  LOOP1(q, divide_by_length(n, row_sums), d);
  LOOP2(q, subtract_off(d, pnts, row_sums), n, d);
  ftype *svcs = malloc(sizeof(ftype) * d_short * d_short);
  for(size_t i = 0; i < d_short; i++)
    for(size_t j = 0; j < d_short; j++)
      svcs[i * d_short + j] = i == j;
  BUFTYPE(ftype) svecs;
  BUFTYPE(ftype) svecs2 = MK_BUF_RW_WO(gpu_context, ftype, d_short * d_max);
  if(save != NULL) {
    save->tries = tries;
    save->n = n;
    save->k = k;
    save->d_short = d_short;
    save->d_long = d;
    save->row_means = malloc(sizeof(ftype) * d);
    enqueueReadBuf(q, sizeof(ftype) * d, row_sums, save->row_means);
    save->which_par = malloc(sizeof(size_t *) * tries);
    save->par_maxes = malloc(sizeof(size_t) * tries);
    save->bases = malloc(sizeof(ftype) * tries * d_short * d);
    svecs = MK_BUF_USE_RW_WO(gpu_context, ftype, tries * d_short * d,
			     save->bases);
  } else
    svecs = MK_BUF_RW_WO(gpu_context, ftype, tries * d_short * d);
  relMem(row_sums);
  BUFTYPE(size_t) pointers_out =
    MK_BUF_RW_NA(gpu_context, size_t, n * k * tries);
  BUFTYPE(ftype) dists_out =
    MK_BUF_RW_NA(gpu_context, ftype, n * k * tries);
  size_t rlt = (rot_len_before * rots_before + rot_len_after * rots_after);
  BUFTYPE(size_t) ortho_dat;
  BUFTYPE(ftype) rot_angs;
  size_t *odt =
      malloc(sizeof(size_t) * ((rlt * 2 + d_max + d_short) * tries + d_max));
  ftype *angs = malloc(sizeof(ftype) * rlt * tries);

  {
    size_t *wkspc = odt + (rlt * 2 + d_max + d_short) * tries;
    for(int i = 0; i < tries; i++)
      MK_NAME(mk_orth_inf)(rot_len_before, rots_before,
			   rot_len_after, rots_after,
			   d_short, d, d_max,
			   wkspc, odt + (rlt * 2 + d_max + d_short) * i,
			   angs + rlt * i);
    ortho_dat = MK_BUF_USE_RO_NA(gpu_context, size_t,
				 (rlt * 2 + d_max + d_short) * tries, odt);
    rot_angs = MK_BUF_USE_RO_NA(gpu_context, ftype, rlt * tries, angs);
  }
  size_t *sgns = malloc(sizeof(size_t) * n * tries);
  BUFTYPE(size_t) signs = MK_BUF_RW_RO(gpu_context, size_t, n * tries);
  for(int i = 0; i < tries; i++) {
    BUFTYPE(ftype) svecsp = MK_SUBBUF_RW_WO_REG(ftype, svecs,
						i * d_short * d, d_short * d);
    FST_GONLY(save_vecs, q, d_short, d, d_max,
	      rots_before, rot_len_before, rots_after, rot_len_after,
	      rlt * i, (rlt * 2 + d_max + d_short) * i,
	      rot_angs, ortho_dat, svcs, svecsp, svecs2);
    relMemU(svecsp);
  }
  free(svcs);
  relMem(svecs2);
  relMemU(ortho_dat);
  relMemU(rot_angs);
  free(odt);
  free(angs);
  BUFTYPE(ftype) space = MK_BUF_RW_NA(gpu_context, ftype,
				      n * d * d_short * tries);
  BUFTYPE(ftype) sumspace = MK_BUF_RW_NA(gpu_context, ftype,
					 n * d_short * tries);
  FST_GONLY(prod_and_sign, q, n, d, d_short, tries, pnts, svecs,
	    space, sumspace, (BUFTYPE(size_t))space, signs);
  relMem(space);
  relMem(sumspace);
  relMem(pnts);
  enqueueReadBuf(q, sizeof(size_t) * n * tries, signs, sgns);
  relMem(signs);
  if(save == NULL)
    relMem(svecs);
  else
    relMemU(svecs);
  clFinish(q);
  size_t *counts = calloc(tries << d_short, sizeof(size_t));
  for(int i = 0; i < tries; i++)
    for(size_t j = 0; j < n; j++)
      counts[i << d_short | sgns[i * n + j]]++;
  size_t max_count = 0;
  for(size_t i = 0; i < (size_t)tries << d_short; i++)
    if(max_count < counts[i])
      max_count = counts[i];
  size_t space_needed = max_count * (d_short + 1);
  if(space_needed < k * k)
    space_needed = k * k;
  space = MK_BUF_RW_NA(gpu_context, ftype,
		       n * d * space_needed);
  BUFTYPE(size_t) which = MK_BUF_RO_WO(gpu_context, size_t,
				       max_count << d_short);
  if(space_needed < k * (k + 1))
    space_needed = k * (k + 1);
  BUFTYPE(size_t) which_d =
    MK_BUF_RW_RO(gpu_context, size_t, n * space_needed);
  BUFTYPE(ftype) distspace =
    MK_BUF_RW_RO(gpu_context, ftype, n * space_needed);
  BUFTYPE(const ftype) pnts2 =
    MK_BUF_USE_RO_NA(gpu_context, ftype, n * d, points);
  size_t *wh = malloc(sizeof(size_t) * max_count << d_short);
  for(int i = 0; i < tries; i++)
    TWO_GONLY(second_half, gpu_context, q, n, k, d_short, d, save, i, tries,
	      sgns + i * n, counts + ((size_t)i << d_short),
	      which, which_d, wh, pnts2, pointers_out, dists_out, space,
	      distspace);
  relMem(which);
  free(counts);
  free(sgns);
  free(wh);
  size_t *fedges = FST_GONLY(det_results, q,
			     n, k, d, n, k * tries,
			     pointers_out, dists_out,
			     pointers_out, pnts2, pnts2, dists_o, space,
			     distspace, which_d);
  relMemU(pnts2);
  clFinish(q);
  clReleaseCommandQueue(q);
  if(save != NULL) {
    save->graph = fedges;
    fedges = malloc(sizeof(size_t) * n * k);
    memcpy(fedges, save->graph, sizeof(size_t) * n * k);
  }
  return(fedges);
}

// Frees subsigns (soft).
// Computes the candidates and puts them in the right places.
void TWO_GONLY(shufcomp, cl_context c, cl_command_queue q, size_t d,
	       size_t ycnt, size_t offset, size_t len, size_t flen,
	       BUFTYPE(const size_t) subsigns,
	       const size_t *which,
	       BUFTYPE(size_t) ipts,
	       BUFTYPE(size_t) ppts) {
  BUFTYPE(const size_t) wp = MK_BUF_USE_RO_NA(c, size_t, len << d, which);
  LOOP3(q, compute_which(d, len, subsigns, wp, ppts), ycnt, d + 1, len);
  relMemU(subsigns);
  relMemU(wp);
  enqueueCopy2D(q, size_t, len * (d + 1), flen * (d + 1), offset * (d + 1),
		ppts, ipts, ycnt, len * (d + 1));
}

// We now have points (n by d_long), save->graph (n by k),
// save->row_means (d_long), save->par_maxes (tries),
// save->which_par (tries, then 1 << d_short by save->par_maxes[i]),
// save->bases (tries by d_short by d_long), y (ycnt by d_long).
size_t *MK_NAME(query) (const save_t *save, const ftype *points,
			size_t ycnt, const ftype *y, ftype **dists_o) {
  setup();
  MAKE_COMMAND_QUEUE(gpu_context, the_gpu, NULL, NULL, q);
  BUFTYPE(ftype) y2 = MK_BUF_COPY_RW_NA(gpu_context, ftype,
					 save->d_long * ycnt, y);
  BUFTYPE(const ftype) rm =
    MK_BUF_USE_RO_NA(gpu_context, ftype, save->d_long, save->row_means);
  LOOP2(q, subtract_off(save->d_long, y2, rm), ycnt, save->d_long);
  relMemU(rm);
  BUFTYPE(const ftype) bases = MK_BUF_USE_RO_NA(gpu_context, ftype,
						 save->tries * save->d_short *
						 save->d_long, save->bases);
  size_t *pmaxes = malloc(sizeof(size_t) * save->tries);
  size_t msofar = 0;
  size_t ispace_needs = 0;
  for(int i = 0; i < save->tries; i++) {
    pmaxes[i] = msofar;
    msofar += save->par_maxes[i];
    if(ispace_needs < save->par_maxes[i])
      ispace_needs = save->par_maxes[i];
  }
  ispace_needs *= save->d_short + 1;
  if(ispace_needs < save->k * (save->k + 1))
     ispace_needs = save->k * (save->k + 1);
  size_t space_needs = msofar * (save->d_short + 1);
  if(space_needs < save->k * save->k)
    space_needs = save->k * save->k;
  BUFTYPE(ftype) space = MK_BUF_RW_NA(gpu_context, ftype,
				       ycnt * space_needs * save->d_long);
  if(space_needs < save->k * (save->k + 1))
    space_needs = save->k * (save->k + 1);
  BUFTYPE(ftype) sumspace = MK_BUF_RW_RO(gpu_context, ftype,
					 ycnt * space_needs);
  BUFTYPE(size_t) intspace = MK_BUF_RW_RO(gpu_context, size_t,
					  ycnt * ispace_needs);
  BUFTYPE(size_t) signs = MK_BUF_RW_NA(gpu_context, size_t,
				       save->tries * ycnt);
  FST_GONLY(prod_and_sign, q, ycnt, save->d_long, save->d_short, save->tries,
	    y2, bases, space, sumspace, (BUFTYPE(size_t))space, signs);
  relMemU(bases);
  relMem(y2);
  BUFTYPE(size_t) ipts = MK_BUF_RW_NA(gpu_context, size_t,
				      msofar * (save->d_short + 1) * ycnt);
  for(int i = 0; i < save->tries; i++) {
    BUFTYPE(size_t) subsgns =
      MK_SUBBUF_RO_NA_REG(size_t, signs, i * ycnt, ycnt);
    TWO_GONLY(shufcomp, gpu_context, q, save->d_short, ycnt, pmaxes[i],
	      save->par_maxes[i], msofar, subsgns, save->which_par[i], ipts,
	      intspace);
  }
  free(pmaxes);
  relMem(signs);
  BUFTYPE(const ftype) y3 =
    MK_BUF_USE_RO_NA(gpu_context, ftype, save->d_long * ycnt, y);
  BUFTYPE(const ftype) pnts =
    MK_BUF_USE_RO_NA(gpu_context, ftype, save->n * save->d_long, points);
  BUFTYPE(const size_t) graph =
    MK_BUF_USE_RO_NA(gpu_context, size_t, save->n * save->k, save->graph);
  BUFTYPE(ftype) dists = MK_BUF_RW_NA(gpu_context, ftype,
				      msofar * (save->d_short + 1) * ycnt);
  FST_GONLY(compdists, q, save->n, msofar * (save->d_short + 1),
	    save->d_long, ycnt, 0, y3, pnts, ipts, dists, space);

  size_t *ans = FST_GONLY(det_results, q,
			  save->n, save->k, save->d_long, ycnt,
			  msofar * (save->d_short + 1), ipts, dists,
			  graph, y3, pnts, dists_o, space, sumspace,
			  intspace);
  relMemU(y3);
  relMemU(pnts);
  relMemU(graph);
  clFinish(q);
  clReleaseCommandQueue(q);
  return(ans);
}
