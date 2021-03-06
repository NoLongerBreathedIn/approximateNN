#ifndef ANN_SAVE_MATLAB
#define ANN_SAVE_MATLAB
#include "../ann_save.h"
#include "mex.h"
extern mxArray *save_to_matlab(const save_t *save);
// Returns true if something went wrong.
extern char retrieve_from_matlab(save_t *save, mxArray *stuff);
#endif
