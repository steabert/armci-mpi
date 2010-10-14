#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "debug.h"

#include "armci.h"
#include "armci-internals.h"
#include "mem_region.h"


/** Allocate a shared memory segment.  Collective.
  *
  * @param[out] base_ptrs Array of length nproc that will contain pointers to
  *                       the base address of each process' patch of the
  *                       segment.
  * @param[in]       size Number of bytes to allocate on the local process.
  */
int ARMCI_Malloc(void **ptr_arr, int bytes) {
  return ARMCI_Malloc_group(ptr_arr, bytes, &ARMCI_GROUP_WORLD);
}


/** Free a shared memory allocation.  Collective.
  *
  * @param[in] ptr Pointer to the local patch of the allocation
  */
int ARMCI_Free(void *ptr) {
  return ARMCI_Free_group(ptr, &ARMCI_GROUP_WORLD);
}


/** Allocate a shared memory segment.  Collective.
  *
  * @param[out] base_ptrs Array of length nproc that will contain pointers to
  *                       the base address of each process' patch of the
  *                       segment.
  * @param[in]       size Number of bytes to allocate on the local process.
  */
int ARMCI_Malloc_group(void **base_ptrs, int size, ARMCI_Group *group) {
  int i;
  mem_region_t *mreg;
 
  mreg = mem_region_create(size);

  for (i = 0; i < mreg->nslices; i++)
    base_ptrs[i] = mreg->slices[i].base;

  if (DEBUG_CAT_ENABLED(DEBUG_CAT_ALLOC)) {
#define BUF_LEN 1000
    char ptr_string[BUF_LEN];
    int  count = 0;

    for (i = 0; i < mreg->nslices && count < BUF_LEN; i++)
      count += snprintf(ptr_string+count, BUF_LEN-count, (i == mreg->nslices-1) ? "%p" : "%p ", base_ptrs[i]);

    dprint(DEBUG_CAT_ALLOC, __func__, "base ptrs [%s]\n", ptr_string);
#undef BUF_LEN
  }

  return 0;
}


/** Free a shared memory allocation.  Collective.
  *
  * @param[in] ptr Pointer to the local patch of the allocation
  */
int ARMCI_Free_group(void *ptr, ARMCI_Group *group) {
  int me;
  mem_region_t *mreg;

  MPI_Comm_rank(MPI_COMM_WORLD, &me);

  if (ptr != NULL) {
    mreg = mem_region_lookup(ptr, me);
    assert(mreg != NULL);
  } else {
    dprint(DEBUG_CAT_ALLOC, __func__, "given NULL\n");
    mreg = NULL;
  }

  mem_region_destroy(mreg);

  return 0;
}


/** Allocate a local buffer suitable for use in one-sided communication
  *
  * @param[in] size Number of bytes to allocate
  * @return         Pointer to the local buffer
  */
void *ARMCI_Malloc_local(int size) {
  void *buf;
  MPI_Alloc_mem(size, MPI_INFO_NULL, &buf);
  return buf;
}


/** Free memory allocated with ARMCI_Malloc_local
  *
  * @param[in] buf Pointer to local buffer to free
  */
int ARMCI_Free_local(void *buf) {
  MPI_Free_mem(buf);
  return 0;
}
