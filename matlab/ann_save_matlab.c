#include "ann_save_matlab.h"
#include <string.h>
#if MX_HAS_INTERLEAVED_COMPLEX
#define mxGetPr mxGetDoubles
#endif

static const char *names[] = {"which_par", "row_means", "bases", "graph"};

#define size_t_class (sizeof(size_t) == 8? mxUINT64_CLASS : mxUINT32_CLASS)


mxArray *save_to_matlab(const save_t *save) {
  mxArray *sstruct = mxCreateStructMatrix(1, 1, 4, names);
  mxArray *which_par = mxCreateCellMatrix(save->tries, 1);
  for(int i = 0; i < save->tries; i++) {
    mxArray *par_spot =
      mxCreateUninitNumericMatrix(1 << save->d_short, save->par_maxes[i],
				  size_t_class, mxREAL);
    memcpy(mxGetData(par_spot), save->which_par[i],
	   sizeof(size_t) * save->par_maxes[i] << save->d_short);
    mxSetCell(which_par, i, par_spot);
  }
  mxSetFieldByNumber(sstruct, 0, 0, which_par);
  mxArray *row_means = mxCreateUninitNumericMatrix(save->d_long, 1,
						   mxDOUBLE_CLASS, mxREAL);
  memcpy(mxGetPr(row_means), save->row_means, sizeof(double) * save->d_long);
  mxSetFieldByNumber(sstruct, 0, 1, row_means);
  size_t bases_size[] = {save->d_long, save->d_short, save->tries};
  mxArray *bases = mxCreateUninitNumericArray(3, bases_size,
					      mxDOUBLE_CLASS, mxREAL);
  memcpy(mxGetPr(bases), save->bases,
	 sizeof(double) * save->d_long * save->d_short * save->tries);
  mxSetFieldByNumber(sstruct, 0, 2, bases);
  size_t graph_size[] = {save->d_long, save->k, save->n};
  mxArray *graph = mxCreateUninitNumericArray(3, graph_size,
					      size_t_class, mxREAL);
  memcpy(mxGetData(graph), save->graph,
	 sizeof(size_t) * save->d_long * save->k * save->n);
  mxSetFieldByNumber(sstruct, 0, 3, graph);
}

#define checked_malloc(p, t, c) \
  if(((p) = malloc(sizeof(t) * (c))) == NULL) return(2)

static char int_retrieve(save_t *save, mxArray *stuff) {
  if(!(mxIsStruct(stuff) && mxIsScalar(stuff)))
    return(1);
  mxArray *row_means = mxGetField(stuff, 0, "row_means");
  if(row_means == NULL || !mxIsDouble(row_means) || mxIsComplex(row_means) ||
     mxGetNumberOfDimensions(row_means) != 2 ||
     mxGetN(row_means) != 1)
    return(1);
  checked_malloc(save->row_means, double,
		 save->d_long = mxGetM(row_means));
  memcpy(save->row_means, mxGetPr(row_means),
	 sizeof(double) * save->d_long);
  mxArray *bases = mxGetField(stuff, 0, "bases");
  if(bases == NULL || !mxIsDouble(bases) || mxIsComplex(bases) ||
     mxGetNumberOfDimensions(bases) != 3 ||
     mxGetM(bases) != save->d_long)
    return(1);
  save->d_short = mxGetDimensions(bases)[1];
  save->tries = mxGetDimensions(bases)[2];
  checked_malloc(save->bases, double,
		 save->d_long * save->d_short * save->tries);
  memcpy(save->bases, mxGetPr(bases),
	 sizeof(double) * save->d_long * save->d_short * save->tries);
  mxArray *which_par = mxGetField(stuff, 0, "which_par");
  if(which_par == NULL || !mxIsCell(which_par) ||
     mxGetNumberOfDimensions(which_par) != 2 ||
     mxGetM(which_par) != save->tries ||
     mxGetN(which_par) != 1)
    return(1);
  checked_malloc(save->which_par, size_t *, save->tries);
  for(int i = 0; i < save->tries; i++)
    save->which_par[i] = NULL;
  checked_malloc(save->par_maxes, size_t, save->tries);
  for(int i = 0; i < save->tries; i++) {
    mxArray *par_spot = mxGetCell(which_par, i);
    if(par_spot == NULL || mxGetClassID(par_spot) != size_t_class ||
       mxIsComplex(par_spot) ||
       mxGetNumberOfDimensions(par_spot) != 2 ||
       mxGetM(par_spot) != (1 << save->d_short))
      return(1);
    save->par_maxes[i] = mxGetN(par_spot);
    checked_malloc(save->which_par[i], size_t,
		   save->par_maxes[i] << save->d_short);
    memcpy(save->which_par[i], mxGetData(par_spot),
	   sizeof(size_t) * save->par_maxes[i] << save->d_short);
  }
  mxArray *graph = mxGetField(stuff, 0, "graph");
  if(graph == NULL || mxGetClassID(graph) != size_t_class ||
     mxIsComplex(graph) ||
     mxGetNumberOfDimensions(graph) != 3 ||
     mxGetM(graph) != save->d_long)
    return(1);
  save->k = mxGetDimensions(graph)[1];
  save->n = mxGetDimensions(graph)[2];
  checked_malloc(save->graph, size_t, save->d_long * save->k * save->n);
  memcpy(save->graph, mxGetData(graph),
	 sizeof(double) * save->d_long * save->k * save->n);
  return(0);
}

#define free_safe(p) if(p != NULL) free(p)
char retrieve_from_matlab(save_t *save, mxArray *stuff) {
  save->tries = 0;
  save->par_maxes = save->graph = NULL;
  save->which_par = NULL;
  save->row_means = save->bases = NULL;
  char c = int_retrieve(save, stuff);
  if(c) {
    free_safe(save->bases);
    free_safe(save->row_means);
    if(save->which_par != NULL)
      for(int i = 0; i < save->tries; i++)
	free_safe(save->which_par[i]);
    free_safe(save->which_par);
    free_safe(save->graph);
    free_safe(save->par_maxes);
  }
  return(c);
}
