#include "mex.h"
#include <string.h>
#include "ann_save_matlab.h"
#include "../ann_ext.h"

#if MX_HAS_INTERLEAVED_COMPLEX
#define mxGetPr mxGetDoubles
#endif

void mexFunction(int nres, mexArray **res,
		 int narg, mexArray **arg) {
  if(nres != 2) {
    mexErrMsgIdAndTxt("approxNN:query:nlhs",
		      "Two outputs required.");
    return;
  }
  if(narg != 3) {
    mexErrMsgIdAndTxt("approxNN:query:nrhs",
		      "Three inputs required.");
    return;
  }
  save_t save;
  if(retrieve_from_matlab(&save, arg[0])) {
    mexErrMsgIdAndTxt("approxNN:query:save",
		      "The first argument must be "
		      "a valid save data structure.");
    return;
  }
  if(!mxIsDouble(arg[1]) || mxIsComplex(arg[1]) ||
     mxGetNumberOfDimensions(arg[1]) != 2 ||
     mxGetM(arg[1]) != save->d_long ||
     mxGetN(arg[1]) != save->n) {
    mexErrMsgIdAndTxt("approxNN:query:points",
		      "The first argument must be a real double matrix "
		      "of the proper dimensions for the save data structure.");
    return;
  }
  if(!mxIsDouble(arg[2]) || mxIsComplex(arg[2]) ||
     mxGetNumberOfDimensions(arg[2]) != 2 ||
     mxGetM(arg[2]) != save->d_long) {
    mexErrMsgIdAndTxt("approxNN:query:y",
		      "The second argument must be a real double matrix "
		      "of the proper height for the save data structure.");
    return;
  }

  double *dists;
  size_t *ptrs = query_ext(&save, mxGetPr(arg[1]),
			   mxGetN(arg[2]), mxGetPr(arg[2]), &array, 0);
  if(ptrs == NULL) {
    mexErrMsgIdAndTxt("approxNN:query:error",
		      "Something's gone wrong, it didn't work.");
    return;
  }

  res[1] = mxCreateUninitNumericMatrix(save->k, mxGetN(arg[2]),
				       sizeof(size_t) == 8?
				       mxUINT64_CLASS : mxUINT32_CLASS,
				       mxREAL);
  size_t *resptr = mxGetData(res[1]);
  // Stupid Matlab starts array indices from 1, WTF?
  for(int i = 0; i < save->k * mxGetN(arg[2]); i++)
    resptr[i] = ptrs[i] + 1;
  free(ptrs);
  
  res[0] = mxCreateUninitNumericMatrix(save->k, mxGetN(arg[2]),
				       mxDOUBLE_CLASS, mxREAL);
  memcpy(mxGetPr(res[0]), dists, sizeof(double) * save->k * mxGetN(arg[2]));
  free(dists);
}
