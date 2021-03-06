#define _GNU_SOURCE
#include "ann_ext.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#ifdef OSX
#include <errno.h>
int pipe2(int fd[2], int flags) {
  int tmp[2];
  tmp[0] = fd[0];
  tmp[1] = fd[1];
  if ((flags & ~(O_CLOEXEC | O_NONBLOCK)) != 0) {
    errno = EINVAL;
    return(-1);
  }
  if(pipe(fd) < 0)
    return(-1);
  flags &= O_NONBLOCK | O_CLOEXEC;
  if(flags) {
    int fcntl_flags;
    if((fcntl_flags = fcntl(fd[1], F_GETFD, 0)) < 0 ||
       fcntl(fd[1], F_SETFD, fcntl_flags | flags) == -1 ||
       (fcntl_flags = fcntl(fd[0], F_GETFD, 0)) < 0 ||
       fcntl(fd[0], F_SETFD, fcntl_flags | flags) == -1) {
      int saved_errno = errno;
      close(fd[0]);
      close(fd[1]);
      fd[0] = tmp[0];
      fd[1] = tmp[1];
      errno = saved_errno;
      return(-1);
    }
  }
  return(0);
}

#endif

static char *dir = NULL;
static char *dirend = NULL;

static void compdir(void) {
  if(dir == NULL) {
    size_t s = strlen(DIR);
    dir = malloc(s + 10);
    strcpy(dir, DIR);
    dir[s] = '/';
    dirend = dir + s + 1;
  }
}

#define write_obj(x, f) fwrite(&x, sizeof(x), 1, f)

size_t *precomp_ext(size_t n, size_t k, size_t d, const ftype *points, 
		    int tries, size_t rots_before, size_t rot_len_before,
		    size_t rots_after, size_t rot_len_after,
		    save_t *save, ftype **dists, char use_cpu) {
  compdir();
  char *args[5];
  args[0] = "precomp";
  for(int i = 1; i < 4; i++)
    args[i] = malloc(11);
  strcpy(args[1], "answXXXXXX");
  strcpy(args[2], "distXXXXXX");
  strcpy(args[3], "saveXXXXXX");
  args[4] = NULL;
  int pp[2];
  if(pipe2(pp, O_CLOEXEC) == -1) {
    for(int i = 1; i < 4; i++)
      free(args[i]);
    return(NULL);
  }
    
  int fs[3];
  for(int i = 0; i < 3; i++)
    fs[i] = mkostemp(args[i + 1], O_CLOEXEC);
  pid_t c = fork();
  if(c == -1)
    return(NULL); // something's wrong
  if(!c) {
    dup2(pp[0], 0);
    if(save == NULL) {
      free(args[3]), args[3] = NULL;
      if(dists == NULL)
	free(args[2]), args[2] = NULL;
    }
    strcpy(dirend, "precomp");
    execv(dir, args);
  }
  close(pp[0]);
  FILE *f = fdopen(pp[1], "wb");
  write_obj(n, f);
  write_obj(k, f);
  write_obj(d, f);
  write_obj(tries, f);
  write_obj(rots_before, f);
  write_obj(rot_len_before, f);
  write_obj(rots_after, f);
  write_obj(rot_len_after, f);
  write_obj(use_cpu, f);
  fwrite(points, sizeof(ftype), n * d, f);
  fclose(f);
  int cstat;
  waitpid(c, &cstat, 0);
  if(!WIFEXITED(cstat) || WEXITSTATUS(cstat)) {
    for(int i = 1; i < 4; i++)
      unlink(args[i]), free(args[i]), close(fs[i - 1]);
    return(NULL);
  }
  size_t *results = malloc(sizeof(size_t) * n * k);
  f = fdopen(fs[0], "rb");
  fread(results, sizeof(size_t), n * k, f);
  fclose(f);
  if(dists != NULL) {
    *dists = malloc(sizeof(ftype) * n * k);
    f = fdopen(fs[1], "rb");
    fread(*dists, sizeof(ftype), n * k, f);
    fclose(f);
  } else
    close(fs[1]);
  if(save != NULL) {
    f = fdopen(fs[2], "rb");
    read_save(save, f);
    fclose(f);
  } else
    close(fs[2]);
  for(int i = 1; i < 4; i++)
    unlink(args[i]), free(args[i]);
  return(results);
}

size_t *query_ext(const save_t *save, const ftype *points,
		  size_t ycnt, const ftype *y, ftype **dists, char use_cpu) {
  compdir();
  char *args[4];
  args[0] = "query";
  for(int i = 1; i < 8; i++)
    args[i] = malloc(11);
  strcpy(args[1], "answXXXXXX");
  strcpy(args[2], "distXXXXXX");
  args[3] = NULL;
  int pp[2];
  if(pipe2(pp, O_CLOEXEC) == -1) {
    for(int i = 1; i < 3; i++)
      free(args[i]);
    return(NULL);
  }
  int fs[2];
  for(int i = 0; i < 2; i++)
    fs[i] = mkostemp(args[i + 1], O_CLOEXEC);
  pid_t c = fork();
  if(c == -1)
    return(NULL);
  if(!c) {
    dup2(pp[0], 0);
    if(dists == NULL)
      free(args[2]), args[2] = NULL;
    strcpy(dirend, "query");
    execv(dir, args);
  }
  close(pp[0]);
  FILE *f = fdopen(pp[1], "wb");
  write_save(save, f);
  fwrite(points, sizeof(ftype), save->n * save->d_long, f);
  write_obj(ycnt, f);
  fwrite(y, sizeof(ftype), ycnt * save->d_long, f);
  write_obj(use_cpu, f);
  fclose(f);

  int cstat;
  waitpid(c, &cstat, 0);
  if(!WIFEXITED(cstat) || WEXITSTATUS(cstat)) {
    for(int i = 1; i < 3; i++)
      unlink(args[i]), free(args[i]), close(fs[i - 1]);
    return(NULL);
  }
  
  size_t *results = malloc(sizeof(size_t) * ycnt * save->k);
  f = fdopen(fs[0], "rb");
  fread(results, sizeof(size_t), ycnt * save->k, f);
  fclose(f);
  if(dists != NULL) {
    *dists = malloc(sizeof(ftype) * ycnt * save->k);
    f = fdopen(fs[1], "rb");
    fread(*dists, sizeof(ftype), ycnt * save->k, f);
    fclose(f);
  } else
    close(fs[1]);
  for(int i = 1; i < 3; i++)
    unlink(args[i]), free(args[i]);
  return(results);
}
