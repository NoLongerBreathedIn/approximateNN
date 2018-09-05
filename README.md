# approximateNN
Approximate nearest neighbors implementation, in C/OpenCL.

`alg.c` contains the algorithm, in a way that can easily be transformed
into pure C or usage of the OpenCL API.

`algc.c` contains the code to turn the algorithm into pure C;
`algc.h` contains extern declarations for `precomp_cpu` and `query_cpu`.

`algorithm.txt` is a very weird pseudocode version of the algorithm.

`ann.c` contains code to allow a user to use either CPU or GPU versions with
the same function call;
`ann.h` contains the declarations for `precomp` and `query`.

`ann_ext.c` contains code to allow a user to use a separate process to
do the same as `ann.c`; it is fully compatible.
`ann_ext.h` contains declarations.

`ann_save.h` contains a definition of `save_t` (the save data structure)
and a declaration of functions to free, read, and write a `save_t`.

`compute.cl` contains the OpenCL code.

`ftype.h` contains code to enable easy switching between float and double
(separate compilation necessary).

`ocl2c.h` is used to convert OpenCL C to regular C.

`precomp.c` is the external process for precomputation.
`query.c` is the same for querying.

`randNorm.c` is used by the test code to create random numbers with a
standard normal distribution.
`randNorm.h` has bindings.

`rand_pr.c` contains code to create random subpermutation matrices and
rotation matrices in a fixed format.
`rand_pr.h` has bindings for this.

`test_correctness.c` contains code to check how well the algorithm does
on random data.

`time_results.c` contains code to time the algorithm on random data.

`timing.h` is a hack to allow identical code to be used on OSX and Linux for
very simple timing purposes.
