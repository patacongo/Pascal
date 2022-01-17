/****************************************************************************
 * pmmgr.c
 *
 *   Copyright (C) 2022 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"
#include "pas_errcodes.h"
#include "pas_error.h"
#include "pexec.h"
#include "pmmgr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HEAP_ALIGN_SHIFT  (4)
#define HEAP_ALLOC_UNIT   (1 << HEAP_ALIGN_SHIFT)
#define HEAP_ALIGN_MASK   (HEAP_ALLOC_UNIT - 1)
#define HEAP_ALIGNUP(a)   (((a) + HEAP_ALIGN_MASK) & ~HEAP_ALIGN_MASK)
#define HEAP_ALIGNDOWN(a) ((a) & ~HEAP_ALIGN_MASK)

/****************************************************************************
 * Private Type Definitions
 ****************************************************************************/

/* This structure describes one memory chunk */

struct memChunk_s
{
  uint16_t forward : 12; /* Offset to next chunk in 16-byte units */
  uint16_t inUse   : 1;  /* true:  This chunk is in-use */
  uint16_t pad1    : 3;  /* Available for future use */

  uint16_t back    : 12; /* Offset to previous chunk in 16-byte units */
  uint16_t pad2    : 4;  /* Available for future use */
};

typedef struct memChunk_s memChunk_t;

/* This structure describes one free memory chunk */

struct freeChunk_s
{
  memChunk_t   chunk;    /* Chunk offsets */
  uint16_t     next;     /* Offset to next free chunk */
  uint16_t     address;  /* Stack address of this chunk */
};

typedef struct freeChunk_s freeChunk_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static memChunk_t  *g_inUseChunks;
static freeChunk_t *g_freeChunks;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static int pexec_DisposeChunk(struct pexec_s *st,
                              freeChunk_t *newFreeChunk)
{
  freeChunk_t *prevFreeChunk;
  freeChunk_t *currFreeChunk;

  /* Find the location to insert the free chunk in the ordered list */

  prevFreeChunk = NULL;
  currFreeChunk = g_freeChunks;

  while (currFreeChunk != NULL)
    {
      freeChunk_t *nextFreeChunk;

      /* Is this chunk larger than the one are are inserting? */

      if (currFreeChunk->chunk.forward >= newFreeChunk->chunk.forward)
        {
          /* Found it!  the previous chunk is smaller than this one (or is
           * the head of the list) and the following free chunk is larger.
           * So it belongs in between.
           */

          /* Add the new chunk to the free list */

          if (prevFreeChunk == NULL)
            {
              newFreeChunk->next  = currFreeChunk->address;
              g_freeChunks        = newFreeChunk;
            }
          else
            {
              newFreeChunk->next  = prevFreeChunk->next;
              prevFreeChunk->next = newFreeChunk->address;
            }

          return eNOERROR;
        }

      /* Get a pointer to the next free chunk */

      if (currFreeChunk->next == 0)
        {
          nextFreeChunk = NULL;
        }
      else
        {
          nextFreeChunk = (freeChunk_t *)ATSTACK(st, currFreeChunk->next);
        }

      /* Set up for the next time through the loop */

      prevFreeChunk = currFreeChunk;
      currFreeChunk = nextFreeChunk;
    }

  /* We get here if the free chunk belows at the end of the list */

  if (prevFreeChunk == NULL)
    {
      newFreeChunk->next = 0;
      g_freeChunks       = newFreeChunk;
    }
  else
    {
      prevFreeChunk->next = newFreeChunk->next;
    }
  prevFreeChunk->next = newFreeChunk->address;
  return eNOERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void pexec_InitializeHeap(struct pexec_s *st)
{
  uint16_t     heapStart = HEAP_ALIGNUP(st->hpb);
  uint16_t     heapEnd   = HEAP_ALIGNDOWN(heapStart + st->hpsize);
  uint16_t     heapSize;
  memChunk_t  *terminus;
  freeChunk_t *initialChunk;

  terminus                     = (memChunk_t *)
                                 ATSTACK(st, heapEnd - HEAP_ALLOC_UNIT);
  memset(terminus, 0, sizeof(memChunk_t));
  terminus->forward            = 0;
  terminus->inUse              = 1;

  heapSize                     = heapEnd - heapStart - HEAP_ALLOC_UNIT;
  terminus->back               = heapSize;

  initialChunk                 = (freeChunk_t *)ATSTACK(st, heapStart);
  memset(initialChunk, 0, sizeof(freeChunk_t));
  initialChunk->chunk.forward  = heapSize;
  initialChunk->next           = heapEnd - HEAP_ALLOC_UNIT;
  initialChunk->address        = heapStart;

  g_inUseChunks                = NULL;
  g_freeChunks                 = initialChunk;
}

/****************************************************************************/

int pexec_New(struct pexec_s *st, uint16_t size)
{
  freeChunk_t *prevFreeChunk;
  freeChunk_t *currFreeChunk;

  /* Search the ordered free list for the smallest free chunk that is big
   * enough for this allocation.
   */

  prevFreeChunk = NULL;
  currFreeChunk = g_freeChunks;
  size          = HEAP_ALIGNUP(size);

  while (currFreeChunk != NULL)
    {
      freeChunk_t *nextFreeChunk;
      uint16_t chunkSize;

      /* Get the size of this chunk (minus overhead when in-use) */

      chunkSize = currFreeChunk->chunk.forward - sizeof(memChunk_t);

      /* Get a pointer to the next free chunk */

      if (currFreeChunk->next == 0)
        {
          nextFreeChunk = NULL;
        }
      else
        {
          nextFreeChunk = (freeChunk_t *)ATSTACK(st, currFreeChunk->next);
        }

      /* Is it big enough to provide the requested allocation? */

      if (chunkSize >= size)
        {

          /* Yes, we have one.  Remove if from the free list */

          if (prevFreeChunk == NULL)
            {
              g_freeChunks = nextFreeChunk;
            }
          else
            {
              prevFreeChunk->next = currFreeChunk->next;
            }

          /* Divide the chunk into an in-use and an available chunk if we did
           * not need the whole thing.
           */

           
          if (chunkSize > size + sizeof(freeChunk_t))
            {
              freeChunk_t *freeChunk;
              freeChunk = (freeChunk_t *)
                          ATSTACK(st, currFreeChunk->address + size);

              freeChunk->chunk.forward     = currFreeChunk->chunk.forward -
                                             size;
              freeChunk->chunk.inUse       = 0;
              freeChunk->chunk.pad1        = 0;
              freeChunk->chunk.back        = size;
              freeChunk->chunk.pad2        = 0;
              freeChunk->next              = currFreeChunk->next - size;
              freeChunk->address           = currFreeChunk->address + size;

              currFreeChunk->chunk.forward = size;

              /* Add the smaller free chunk to the ordered free list */

              pexec_DisposeChunk(st, freeChunk);
            }

          currFreeChunk->chunk.inUse = 1;

          /* Return the address of the allocated memory */

          PUSH(st, currFreeChunk->address + sizeof(memChunk_t));
          return eNOERROR;
        }

      /* Set up for the next time through the loop */

      prevFreeChunk = currFreeChunk;
      currFreeChunk = nextFreeChunk;
    }

  /* Failed to allocate */

  PUSH(st, 0);
  return eNEWFAILED;
}

/****************************************************************************/

int pexec_Dispose(struct pexec_s *st, uint16_t address)
{
  freeChunk_t *freeChunk;

  freeChunk = (freeChunk_t *)ATSTACK(st, address - sizeof(memChunk_t));
  return pexec_DisposeChunk(st, freeChunk);
}
