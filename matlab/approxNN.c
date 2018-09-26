/*==========================================================
 * approxNN.c
 *
 * X is d x n matrix
 * funciton returns k approximate nearest neighbors
 *
 * The calling syntax is:
 *
 *		[Inds,Dis] = approxNN(X,k);
 *
 *
 * This is a MEX-file for MATLAB.
 * Gal Mishne 2018
 *========================================================*/

#include "mex.h"
#include <string.h>
#include "../ann_ext.h"


/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[],
		 int nrhs, const mxArray *prhs[]) {
  double *inMatrix;               /* dxn input matrix */
  size_t *Idx;              /* output matrix */
  int n;              /*  Number of points				 */
  int k;              /*  Number of nearest neighbors		 */
  int d;              /*  Dimensionality of the points     */
  double *Dis;              /* output matrix */
    
  int tries=10; 
  size_t rots_before=1; 
  size_t rot_len_before=6;
  size_t rots_after=1; 
  size_t rot_len_after=1; 
  save_t *save=NULL;
  char use_cpu=1;
            
  /* check for proper number of arguments */
  if(nrhs!=2)
    mexErrMsgIdAndTxt("approxNN:precomp:nrhs", "Two inputs required.");
  if(nlhs!=2)
    mexErrMsgIdAndTxt("approxNN:precomp:nlhs", "Two outputs required.");
  
  /* make sure the first input argument is type double */
  if(!mxIsDouble(prhs[0]) || mxIsComplex(prhs[0]))
    mexErrMsgIdAndTxt("approxNN:precomp:notDouble",
		      "Input matrix must be type double.");
  /* make sure the second input argument is scalar */
  if(!(mxIsDouble(prhs[1]) && !mxIsComplex(prhs[1])
       && mxGetNumberOfElements(prhs[1]) == 1))
    mexErrMsgIdAndTxt("approxNN:precomp:notScalar",
		      "Input k must be a scalar.");

  mxDestroyArray(plhs[0]);
  mxDestroyArray(plhs[1]);
  
  /* get the value of the scalar input  */
  n = mxGetN(prhs[0]);
  k = mxGetScalar(prhs[1]);
  d = mxGetM(prhs[0]);
  
  /* create a pointer to the real data in the input matrix  */
#if MX_HAS_INTERLEAVED_COMPLEX
  inMatrix = mxGetDoubles(prhs[0]);
#else
  inMatrix = mxGetPr(prhs[0]);
#endif
  
  /* call the computational routine */
  Idx = precomp_ext(n, k, d, inMatrix, 
		tries, rots_before, rot_len_before,
		rots_after, rot_len_after, save,
		&Dis, use_cpu);
    /* create the output matrix */
  if(Idx == NULL)
    mexPrintf("Something went wrong.\n"), return;
  
  plhs[0] = mxCreateNumericMatrix(k, n,
				  sizeof(size_t) == 8?
				  mxUINT64_CLASS : mxUINT32_CLASS, mxREAL);
  plhs[1] = mxCreateDoubleMatrix(k, n, mxREAL);
  
  // Stupid Matlab starts array indices from 1, WTF?
  for(int i = 0; i < k * n; i++)
    Idx[i]++;
  
#if MX_HAS_INTERLEAVED_COMPLEX
#define mxGetPr mxGetDoubles
#endif
  memcpy(mxGetPr(plhs[1]), Dis, sizeof(double) * k * n);
  memcpy(mxGetData(plhs[0]), Idx, sizeof(size_t) * k * n);
  free(Idx);
}
