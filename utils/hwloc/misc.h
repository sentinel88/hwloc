/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2015 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/misc.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

extern void usage(const char *name, FILE *where);

static __hwloc_inline void
hwloc_utils_input_format_usage(FILE *where, int addspaces)
{
  fprintf (where, "  --input <XML file>\n");
  fprintf (where, "  -i <XML file>   %*sRead topology from XML file <path>\n",
	   addspaces, " ");
#ifdef HWLOC_LINUX_SYS
  fprintf (where, "  --input <directory>\n");
  fprintf (where, "  -i <directory>  %*sRead topology from chroot containing the /proc and /sys\n",
	   addspaces, " ");
  fprintf (where, "                  %*sof another system\n",
	   addspaces, " ");
#endif
  fprintf (where, "  --input \"n:2 2\"\n");
  fprintf (where, "  -i \"n:2 2\"      %*sSimulate a fake hierarchy, here with 2 NUMA nodes of 2\n",
	   addspaces, " ");
  fprintf (where, "                  %*sprocessors\n",
	   addspaces, " ");
  fprintf (where, "  --input-format <format>\n");
  fprintf (where, "  --if <format>   %*sEnforce input format among "
	   "xml, "
#ifdef HWLOC_LINUX_SYS
	   "fsroot, "
#endif
	   "synthetic\n",
	   addspaces, " ");
}

enum hwloc_utils_input_format {
  HWLOC_UTILS_INPUT_DEFAULT,
  HWLOC_UTILS_INPUT_XML,
  HWLOC_UTILS_INPUT_FSROOT,
  HWLOC_UTILS_INPUT_SYNTHETIC
};

static __hwloc_inline enum hwloc_utils_input_format
hwloc_utils_parse_input_format(const char *name, const char *callname)
{
  if (!hwloc_strncasecmp(name, "default", 3))
    return HWLOC_UTILS_INPUT_DEFAULT;
  else if (!hwloc_strncasecmp(name, "xml", 1))
    return HWLOC_UTILS_INPUT_XML;
  else if (!hwloc_strncasecmp(name, "fsroot", 1))
    return HWLOC_UTILS_INPUT_FSROOT;
  else if (!hwloc_strncasecmp(name, "synthetic", 1))
    return HWLOC_UTILS_INPUT_SYNTHETIC;

  fprintf(stderr, "input format `%s' not supported\n", name);
  usage(callname, stderr);
  exit(EXIT_FAILURE);
}

static __hwloc_inline int
hwloc_utils_lookup_input_option(char *argv[], int argc, int *consumed_opts,
				char **inputp, enum hwloc_utils_input_format *input_formatp,
				const char *callname)
{
  if (!strcmp (argv[0], "--input")
	       || !strcmp (argv[0], "-i")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    if (strlen(argv[1]))
      *inputp = argv[1];
    else
      *inputp = NULL;
    *consumed_opts = 1;
    return 1;
  }
  else if (!strcmp (argv[0], "--input-format")
	   || !strcmp (argv[0], "--if")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    *input_formatp = hwloc_utils_parse_input_format (argv[1], callname);
    *consumed_opts = 1;
    return 1;
  }

  /* backward compat with 1.0 */
  else if (!strcmp (argv[0], "--synthetic")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    *inputp = argv[1];
    *input_formatp = HWLOC_UTILS_INPUT_SYNTHETIC;
    *consumed_opts = 1;
    return 1;
  } else if (!strcmp (argv[0], "--xml")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    *inputp = argv[1];
    *input_formatp = HWLOC_UTILS_INPUT_XML;
    *consumed_opts = 1;
    return 1;
  } else if (!strcmp (argv[0], "--fsys-root")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    *inputp = argv[1];
    *input_formatp = HWLOC_UTILS_INPUT_FSROOT;
    *consumed_opts = 1;
    return 1;
  }

  return 0;
}

static __hwloc_inline int
hwloc_utils_enable_input_format(struct hwloc_topology *topology,
				const char *input,
				enum hwloc_utils_input_format input_format,
				int verbose, const char *callname)
{
  if (input_format == HWLOC_UTILS_INPUT_DEFAULT) {
    struct stat inputst;
    int err;
    err = stat(input, &inputst);
    if (err < 0) {
      if (verbose > 0)
	printf("assuming `%s' is a synthetic topology description\n", input);
      input_format = HWLOC_UTILS_INPUT_SYNTHETIC;
    } else if (S_ISDIR(inputst.st_mode)) {
      if (verbose > 0)
	printf("assuming `%s' is a file-system root\n", input);
      input_format = HWLOC_UTILS_INPUT_FSROOT;
    } else if (S_ISREG(inputst.st_mode)) {
      if (verbose > 0)
	printf("assuming `%s' is a XML file\n", input);
      input_format = HWLOC_UTILS_INPUT_XML;
    } else {
      fprintf (stderr, "Unrecognized input file: %s\n", input);
      usage (callname, stderr);
      return EXIT_FAILURE;
    }
  }

  switch (input_format) {
  case HWLOC_UTILS_INPUT_XML:
    if (!strcmp(input, "-"))
      input = "/dev/stdin";
    if (hwloc_topology_set_xml(topology, input)) {
      perror("Setting source XML file");
      return EXIT_FAILURE;
    }
    break;

  case HWLOC_UTILS_INPUT_FSROOT:
#ifdef HWLOC_LINUX_SYS
    if (hwloc_topology_set_fsroot(topology, input)) {
      perror("Setting source filesystem root");
      return EXIT_FAILURE;
    }
#else /* HWLOC_LINUX_SYS */
    fprintf(stderr, "This installation of hwloc does not support changing the file-system root, sorry.\n");
    exit(EXIT_FAILURE);
#endif /* HWLOC_LINUX_SYS */
    break;

  case HWLOC_UTILS_INPUT_SYNTHETIC:
    if (hwloc_topology_set_synthetic(topology, input)) {
      perror("Setting synthetic topology description");
      return EXIT_FAILURE;
    }
    break;

  case HWLOC_UTILS_INPUT_DEFAULT:
    assert(0);
  }

  return 0;
}

static __hwloc_inline void
hwloc_utils_print_distance_matrix(hwloc_topology_t topology, hwloc_obj_t root, unsigned nbobjs, unsigned reldepth, float *matrix, int logical)
{
  hwloc_obj_t objj, obji;
  unsigned i, j;

  /* column header */
  printf("  index");
  for(j=0, objj=NULL; j<nbobjs; j++) {
    objj = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, root->depth+reldepth, objj);
    printf(" % 5d",
	   (int) (logical ? objj->logical_index : objj->os_index));
  }
  printf("\n");

  /* each line */
  for(i=0, obji=NULL; i<nbobjs; i++) {
    obji = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, root->depth+reldepth, obji);
    /* row header */
    printf("  % 5d",
	     (int) (logical ? obji->logical_index : obji->os_index));

    /* row values */
    for(j=0, objj=NULL; j<nbobjs; j++) {
      objj = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, root->depth+reldepth, objj);
      for(j=0; j<nbobjs; j++)
	printf(" %2.3f", matrix[i*nbobjs+j]);
      printf("\n");
    }
  }
}

static __hwloc_inline hwloc_pid_t
hwloc_pid_from_number(int pid_number, int set_info __hwloc_attribute_unused)
{
  hwloc_pid_t pid;
#ifdef HWLOC_WIN_SYS
  pid = OpenProcess(set_info ? PROCESS_SET_INFORMATION : PROCESS_QUERY_INFORMATION, FALSE, pid_number);
  if (!pid) {
    DWORD error = GetLastError();
    char *message;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&message, 0, NULL);
    fprintf(stderr, "OpenProcess %d failed %ld: %s\n", pid_number, error, message);
    exit(EXIT_FAILURE);
  }
#else
  pid = pid_number;
#endif
  return pid;
}

static __hwloc_inline void
hwloc_lstopo_show_summary(FILE *output, hwloc_topology_t topology)
{
  unsigned topodepth = hwloc_topology_get_depth(topology);
  unsigned depth, nbobjs;
  for (depth = 0; depth < topodepth; depth++) {
    hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, 0);
    char type[64];
    nbobjs = hwloc_get_nbobjs_by_depth (topology, depth);
    fprintf(output, "%*s", (int) depth, "");
    hwloc_obj_type_snprintf(type, sizeof(type), obj, 1);
    fprintf (output,"depth %u:\t%u %s (type #%u)\n",
	     depth, nbobjs, type, obj->type);
  }
  nbobjs = hwloc_get_nbobjs_by_depth (topology, HWLOC_TYPE_DEPTH_BRIDGE);
  if (nbobjs)
    fprintf (output, "Special depth %d:\t%u %s (type #%u)\n",
	     HWLOC_TYPE_DEPTH_BRIDGE, nbobjs, "Bridge", HWLOC_OBJ_BRIDGE);
  nbobjs = hwloc_get_nbobjs_by_depth (topology, HWLOC_TYPE_DEPTH_PCI_DEVICE);
  if (nbobjs)
    fprintf (output, "Special depth %d:\t%u %s (type #%u)\n",
	     HWLOC_TYPE_DEPTH_PCI_DEVICE, nbobjs, "PCI Device", HWLOC_OBJ_PCI_DEVICE);
  nbobjs = hwloc_get_nbobjs_by_depth (topology, HWLOC_TYPE_DEPTH_OS_DEVICE);
  if (nbobjs)
    fprintf (output, "Special depth %d:\t%u %s (type #%u)\n",
	     HWLOC_TYPE_DEPTH_OS_DEVICE, nbobjs, "OS Device", HWLOC_OBJ_OS_DEVICE);
  nbobjs = hwloc_get_nbobjs_by_depth (topology, HWLOC_TYPE_DEPTH_MISC);
  if (nbobjs)
    fprintf (output, "Special depth %d:\t%u %s (type #%u)\n",
	     HWLOC_TYPE_DEPTH_MISC, nbobjs, "Misc", HWLOC_OBJ_MISC);
}
