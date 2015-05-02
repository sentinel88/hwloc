/*
 * Copyright © 2015 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <private/cpuid-x86.h>

static void dump_one_cpuid(FILE *output, unsigned *regs, unsigned inregmask)
{
  fprintf(output, "%x %x %x %x %x", inregmask, regs[0], regs[1], regs[2], regs[3]);
  hwloc_x86_cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
  fprintf(output, "=> %x %x %x %x\n", regs[0], regs[1], regs[2], regs[3]);
}

static void dump_one_proc(hwloc_topology_t topo, hwloc_obj_t pu, const char *path)
{
  unsigned regs[4];
  unsigned highest_cpuid, highest_ext_cpuid;
  unsigned i;
  FILE *output;
  int err;

  err = hwloc_set_cpubind(topo, pu->cpuset, HWLOC_CPUBIND_PROCESS);
  if (err < 0) {
    err = hwloc_set_cpubind(topo, pu->cpuset, HWLOC_CPUBIND_THREAD);
    if (err < 0) {
      fprintf(stderr, "Cannot bind to PU P#%u\n", pu->os_index);
      return;
    }
  }

  output = fopen(path, "w");
  if (!output) {
    fprintf(stderr, "Cannot open file '%s' for writing: %s\n", path, strerror(errno));
    return;
  }

  printf("Gathering CPUID of PU P#%u in path %s ...\n", pu->os_index, path);

  fprintf(output, "# mask e[abcd]x => e[abcd]x\n");

  regs[0] = 0;
  hwloc_x86_cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
  highest_cpuid = regs[0];
  regs[0] = 0x80000000;
  hwloc_x86_cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
  highest_ext_cpuid = regs[0];

  regs[0] = 0;
  dump_one_cpuid(output, regs, 0x1);

  if (highest_cpuid >= 1) {
    regs[0] = 1;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_cpuid >= 4) {
    for(i=0; ; i++) {
      regs[0] = 4;
      regs[2] = i;
      dump_one_cpuid(output, regs, 0x5);
      if (!(regs[0] & 0x1f))
	break;
    }
  }

  if (highest_cpuid >= 7) {
    regs[0] = 7;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_cpuid >= 0xb) {
    for(i=0; ; i++) {
      regs[0] = 0xb;
      regs[2] = i;
      dump_one_cpuid(output, regs, 0x5);
      if (!regs[0] && !regs[1])
	break;
    }
  }

  regs[0] = 0x80000000;
  dump_one_cpuid(output, regs, 0x1);

  if (highest_ext_cpuid >= 0x80000001) {
    regs[0] = 0x80000001;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000002) {
    regs[0] = 0x80000002;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000003) {
    regs[0] = 0x80000003;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000004) {
    regs[0] = 0x80000004;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000005) {
    regs[0] = 0x80000005;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000006) {
    regs[0] = 0x80000006;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x80000008) {
    regs[0] = 0x80000008;
    dump_one_cpuid(output, regs, 0x1);
  }

  if (highest_ext_cpuid >= 0x8000001d) {
    for(i=0; ; i++) {
      regs[0] = 0x8000001d;
      regs[2] = i;
      dump_one_cpuid(output, regs, 0x5);
      if (!(regs[0] & 0x1f))
	break;
    }
  }

  if (highest_ext_cpuid >= 0x8000001e) {
    regs[0] = 0x8000001e;
    dump_one_cpuid(output, regs, 0x1);
  }

  fclose(output);
}

static void usage(const char *callname)
{
  fprintf(stderr, "Usage : %s [ options ] ... [ outdir ]\n", callname);
  fprintf(stderr, "  outdir is an optional output directory instead of hwloc-x86-cpuid/\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c <n>     Only gather for logical processor with logical index <n>\n");
  fprintf(stderr, "  -h --help  Show this usage\n");
}

int main(int argc, const char * const argv[])
{
  hwloc_topology_t topo;
  hwloc_obj_t pu;
  const char *basedir;
  const char *callname;
  char *path;
  size_t pathlen;
  unsigned idx = (unsigned) -1;
  int err;
  int ret = EXIT_SUCCESS;

  callname = argv[0];
  argc--; argv++;

  while (argc > 0 && *argv[0] == '-') {
    if (argc >= 2 && !strcmp(argv[0], "-c")) {
      idx = atoi(argv[1]);
      argc -= 2;
      argv += 2;
    } else {
      usage(callname);
      if (strcmp(argv[0], "-h") && strcmp(argv[0], "--help"))
	ret = EXIT_FAILURE;
      goto out;
    }
  }

  basedir = "./hwloc-x86-cpuid";
  if (argc >= 1)
    basedir = argv[0];

  hwloc_topology_init(&topo);
  hwloc_topology_load(topo);

  err = mkdir(basedir, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
  if (err < 0) {
    if (access(basedir, X_OK|W_OK) < 0) {
      fprintf(stderr, "Could not create/open destination directory %s\n", basedir);
      ret = EXIT_FAILURE;
      goto out_with_topo;
    }
  }
  printf("Gathering in directory %s ...\n", basedir);

  pathlen = strlen(basedir) + 20; /* for '/pu%u' */
  path = malloc(pathlen);

  if (idx == (unsigned) -1) {
    pu = NULL;
    while ((pu = hwloc_get_next_obj_by_type(topo, HWLOC_OBJ_PU, pu)) != NULL) {
      idx = pu->os_index;
      snprintf(path, pathlen, "%s/pu%u", basedir, idx);
      dump_one_proc(topo, pu, path);
    }
  } else {
    pu = hwloc_get_pu_obj_by_os_index(topo, idx);
    if (!pu) {
      fprintf(stderr, "Cannot find PU P#%u\n", idx);
      ret = EXIT_FAILURE;
      goto out_with_path;
    } else {
      snprintf(path, pathlen, "%s/pu%u", basedir, idx);
      dump_one_proc(topo, pu, path);
    }
  }

 out_with_path:
  free(path);
 out_with_topo:
  hwloc_topology_destroy(topo);
 out:
  return ret;
}
