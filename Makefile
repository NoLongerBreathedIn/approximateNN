TEST_EFILES := time_results test_correctness

RUN_EFILES := precomp query
EFILES := $(TEST_EFILES) $(RUN_EFILES)
LIB_OFILES := algc.o rand_pr.o ann.o ann_save.o
TEST_OFILES := time_results.o test_correctness.o
RUN_OFILES := precomp.o query.o
OFILES := $(LIB_OFILES) randNorm.o $(TEST_OFILES) $(RUN_OFILES) ann_ext.o
FAKE_HFILES := time_results.h test_correctness.h compare_results.h \
	precomp.h query.h
WARNS := -Wall -Wextra -Wpedantic -Wno-unused-parameter
OS := $(shell uname -s)
DIR := $(shell pwd)

ifeq ($(OS), Darwin)
	OSOPT := -DOSX
else
	OSOPT := -DLINUX
endif

.PHONY: clean

all: $(EFILES) ann_ext.o
clean: 
	rm -rf $(EFILES) $(OFILES) *.dSYM

algc.o: ocl2c.h compute.cl rand_pr.h ann.h alg.c

ann.o: algc.h

algc.h: ann.h
	touch $@

ann.h ann_ext.h: ann_save.h
	touch $@

ann_save.h: ftype.h
	touch $@

$(RUN_OFILES): ann.h

$(TEST_OFILES): ann.h randNorm.h

time_results.o: timing.h

time_results: time_results.o $(LIB_OFILES) randNorm.o
	cc -o $@ $^ -lm

test_correctness: test_correctness.o $(LIB_OFILES) randNorm.o
	cc -o $@ $^ -lm

precomp: precomp.o $(LIB_OFILES)
	cc -o $@ $^ -lm

query: query.o $(LIB_OFILES)
	cc -o $@ $^ -lm

.INTERMEDIATE: $(FAKE_HFILES)

$(FAKE_HFILES): %.h:
	touch $@

$(OFILES): %.o: %.c %.h ftype.h
	clang -c -g -DDIR='"$(DIR)"' $(OSOPT) $(WARNS) $<
# CC will complain about sign comparisons where one side is unsigned var
# and other side is positive int literal.
# Clang won't. 
