/****************************************************************************
 * libexec_heap.c
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
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"
#include "pas_errcodes.h"
#include "pas_error.h"

#include "libexec.h"
#include "libexec_heap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Alignment helpers */

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
  uint16_t forward : 12; /* Offset from this chunk to the next chunk */
  uint16_t inUse   : 1;  /* true:  This chunk is in-use */
  uint16_t pad1    : 3;  /* Available for future use */

  uint16_t back    : 12; /* Offset from this chunk to the previous chunk */
  uint16_t pad2    : 4;  /* Available for future use */

  uint16_t address;     /* Heap base stack address of this chunk */
  uint16_t pad3;         /* Available for future use */
};

typedef struct memChunk_s memChunk_t;

/* This structure describes one free memory chunk */

struct freeChunk_s
{
  memChunk_t chunk;      /* Chunk offsets */
  uint16_t   prev;       /* Heap base offset to previous free chunk */
  uint16_t   next;       /* Heap base offset to next free chunk */
  uint16_t   pad[2];     /* Unused */
};

typedef struct freeChunk_s freeChunk_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static void libexec_AddChunkToFreeList(struct libexec_s *st,
                                       freeChunk_t *newChunk)
{
  freeChunk_t *prevChunk;
  freeChunk_t *freeChunk;
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);

  /* Find the location to insert the free chunk in the ordered list */

  prevChunk = NULL;
  freeChunk = st->freeChunks;

  while (freeChunk != NULL)
    {
      freeChunk_t *nextChunk;

      /* Get a pointer to the next free chunk */

      if (freeChunk->next == 0)
        {
          nextChunk = NULL;
        }
      else
        {
          uint16_t nextAddr = heapStart + freeChunk->next;
          nextChunk         = (freeChunk_t *)ATSTACK(st, nextAddr);
        }

      /* Is this chunk larger than the one are are inserting? */

      if (freeChunk->chunk.forward >= newChunk->chunk.forward)
        {
          /* Found it!  the previous chunk is smaller than this one (or is
           * the head of the list) and the following free chunk is larger (if.
           * there is one).  So it belongs in between.
           */

          /* Add the new chunk to the free list */

          if (prevChunk == NULL)
            {
              newChunk->prev  = 0;
              newChunk->next  = freeChunk->chunk.address;
              freeChunk->prev = newChunk->chunk.address;
              st->freeChunks  = newChunk;
            }
          else if (nextChunk != NULL)
            {
              newChunk->prev  = prevChunk->chunk.address;
              newChunk->next  = nextChunk->chunk.address;
              nextChunk->prev = newChunk->chunk.address;
              prevChunk->next = newChunk->chunk.address;
            }
          else
            {
              newChunk->prev  = prevChunk->chunk.address;
              newChunk->next  = 0;
              prevChunk->next = newChunk->chunk.address;
            }

          return;
        }

      /* Set up for the next time through the loop */

      prevChunk = freeChunk;
      freeChunk = nextChunk;
    }

  /* We get here if the free chunk belows at the end of the list */

  if (prevChunk == NULL)
    {
      newChunk->prev  = 0;
      newChunk->next  = 0;
      st->freeChunks  = newChunk;
    }
  else
    {
      newChunk->prev  = prevChunk->chunk.address;
      newChunk->next  = 0;
      prevChunk->next = newChunk->chunk.address;
    }
}

/****************************************************************************/

static void libexec_RemoveChunkFromFreeList(struct libexec_s *st,
                                            freeChunk_t *freeChunk)
{
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);

  /* Check if this is the first chunk in the list */

  if (freeChunk->prev == 0)
    {
      if (freeChunk->next == 0)
        {
          /* This is the only chunk in the free list */

          st->freeChunks       = NULL;
        }
      else
        {
          uint16_t nextAddr    = heapStart + freeChunk->next;
          st->freeChunks       = (freeChunk_t *)ATSTACK(st, nextAddr);
          st->freeChunks->prev = 0;
        }
    }
  else
    {
      uint16_t     prevAddr  = heapStart + freeChunk->prev;
      uint16_t     nextAddr  = heapStart + freeChunk->next;
      freeChunk_t *prevChunk = (freeChunk_t *)ATSTACK(st, prevAddr);
      freeChunk_t *nextChunk = (freeChunk_t *)ATSTACK(st, nextAddr);

      prevChunk->next        = freeChunk->next;
      nextChunk->prev        = prevChunk->chunk.address;
    }
}

/****************************************************************************/

static void libexec_DisposeChunk(struct libexec_s *st, freeChunk_t *newChunk)
{
  freeChunk_t *prevChunk = NULL;
  freeChunk_t *nextChunk = NULL;
  uint16_t addr;
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);
  bool merged = false;

  /* This chunk is no longer in use */

  newChunk->chunk.inUse = 0;

  /* Get the memory chunks after the newly freed one */

  if (newChunk->chunk.forward != 0)
    {
      addr      = heapStart + newChunk->chunk.address +
                  newChunk->chunk.forward;
      nextChunk = (freeChunk_t *)ATSTACK(st, addr);
    }

  /* Check if we can merge the new free chunk with the preceding chunk. */

  if (newChunk->chunk.back != 0)
    {
      /* Get the memory chunks before the newly freed one */

      addr      = heapStart + newChunk->chunk.address -
                  newChunk->chunk.back;
      prevChunk = (freeChunk_t *)ATSTACK(st, addr);

      /* Is the previous chunk inUse? */

      if (!prevChunk->chunk.inUse)
        {
          /* No, then merge the newChunk into it.  First, remove it
           * from the free list.
           */

          libexec_RemoveChunkFromFreeList(st, prevChunk);

          prevChunk->chunk.forward  += newChunk->chunk.forward;
          if (nextChunk != NULL)
            {
              nextChunk->chunk.back += newChunk->chunk.back;
            }

          /* Then put the larger chunk back into the free list. */

          libexec_AddChunkToFreeList(st, prevChunk);
          newChunk                   = prevChunk;
          prevChunk                  = NULL;
          merged                     = true;
        }
    }

  /* Check if we can merge the new free chunk with the following chunk. */

  if (nextChunk != NULL)
    {
      /* Get the memory chunk after the next one */

      /* Is the next chunk inUse? */

      if (!nextChunk->chunk.inUse)
        {
          /* No, then merge it into the newChunk.  First, remove it
           * from the free list.
           */

          libexec_RemoveChunkFromFreeList(st, nextChunk);

          /* Then merge it into the newChunk */

          newChunk->chunk.forward += nextChunk->chunk.forward;

          /* Is there a memory chunk after the next chunk */

          if (nextChunk->chunk.forward != 0)
            {
              freeChunk_t *afterThat = NULL;

              /* Yes, get the memory chunk after the next chunk */

              addr      = heapStart + nextChunk->chunk.address +
                          nextChunk->chunk.forward;
              afterThat = (freeChunk_t *)ATSTACK(st, addr);

              /* And adjust its back pointer */

              afterThat->chunk.back += newChunk->chunk.back;
            }

          /* Then put the larger chunk back into the free list. */

          libexec_AddChunkToFreeList(st, newChunk);
          merged                   = true;
        }
    }

  /* If the new chunk was not merged, then insert the chunk into the free list */

  if (!merged)
    {
      libexec_AddChunkToFreeList(st, newChunk);
    }
}

/****************************************************************************/

#ifdef CONFIG_PASCAL_HEAPDEBUG
static void libexec_DumpHeap(struct libexec_s *st, const char *msg,
                             uint16_t address)
{
  memChunk_t *chunk;
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);
  uint16_t heapEnd   = HEAP_ALIGNDOWN(heapStart + st->hpSize);
  uint16_t chunkAddr = heapStart;
  unsigned int chunkNo = 1;

  printf("%s: address=%04x\n", msg, address);
  while (chunkAddr < heapEnd)
    {
      /* Print basic chunk information */

      chunk = (memChunk_t *)ATSTACK(st, chunkAddr);

      printf("%4u: address=%04x forward=%04x back=%04x inuse=%u\n",
             chunkNo, chunk->address, chunk->forward, chunk->back, chunk->inUse);

      /* Free chunks have free list links as well */

      if (chunk->inUse == 0)
        {
          freeChunk_t *freeChunk = (freeChunk_t *)chunk;

          printf("      prev=%04x next=%04x\n",
                 freeChunk->prev, freeChunk->next);
        }

      /* The final chunk should have forward == 0 */

      if (chunk->forward == 0)
        {
          break;
        }

      /* Otherwise, set up for the next chunk */

      chunkAddr += chunk->forward;
      chunkNo++;
    }
}
#else
#  define libexec_DumpHeap(st, msg, address)
#endif

/****************************************************************************/

static uint16_t libexec_Alloc(struct libexec_s *st, uint16_t size)
{
  freeChunk_t *freeChunk;
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);

  /* Search the ordered free list for the smallest free chunk that is big
   * enough for this allocation.
   */

  freeChunk = st->freeChunks;
  size      = HEAP_ALIGNUP(size);

  while (freeChunk != NULL)
    {
      freeChunk_t *nextChunk;
      uint16_t chunkSize;

      /* Get the size of this chunk (minus overhead when in-use) */

      chunkSize = freeChunk->chunk.forward - sizeof(memChunk_t);

      /* Get a pointer to the next free chunk */

      if (freeChunk->next == 0)
        {
          nextChunk = NULL;
        }
      else
        {
          uint16_t nextAddr = heapStart + freeChunk->next;
          nextChunk = (freeChunk_t *)ATSTACK(st, nextAddr);
        }

      /* Is it big enough to provide the requested allocation? */

      if (chunkSize >= size)
        {
          libexec_RemoveChunkFromFreeList(st, freeChunk);

          /* Divide the chunk into an in-use and an available chunk if we did
           * not need the whole thing.
           */

          freeChunk->chunk.inUse       = 1;

          if (chunkSize > size + sizeof(freeChunk_t))
            {
              /* Break off "sub-chunk" for the big free chunk at offset 'size' */

              freeChunk_t *subChunk    = (freeChunk_t *)
                ATSTACK(st, heapStart + freeChunk->chunk.address + size);

              subChunk->chunk.forward  = freeChunk->chunk.forward - size;
              subChunk->chunk.inUse    = 0;
              subChunk->chunk.pad1     = 0;
              subChunk->chunk.back     = size;
              subChunk->chunk.pad2     = 0;
              subChunk->chunk.address  = freeChunk->chunk.address + size;
              subChunk->chunk.pad3     = 0;

              if (freeChunk->next != 0)
                {
                  subChunk->next       = freeChunk->next - size;
                }
              else
                {
                  subChunk->next       = 0;
                }

              /* And shrink the original to 'size' */

              freeChunk->chunk.forward = size;

              /* Add the smaller sub-chunk to the ordered free list */

              libexec_DisposeChunk(st, subChunk);
            }

          /* Return the address of the allocated memory */

          libexec_DumpHeap(st, "After allocation",
            heapStart + freeChunk->chunk.address + sizeof(memChunk_t));

          return heapStart + freeChunk->chunk.address + sizeof(memChunk_t);
        }

      /* Set up for the next time through the loop */

      freeChunk = nextChunk;
    }

  /* Failed to allocate */

  libexec_DumpHeap(st, "Allocation failure", 0);
  return 0;
}

/****************************************************************************/

static int libexec_Free(struct libexec_s *st, uint16_t address)
{
  freeChunk_t *freeChunk;
  uint16_t heapStart = HEAP_ALIGNUP(st->hpb);

  /* Verify that the address being freed lies in the heap region */

  if (address < heapStart + sizeof(memChunk_t) ||
      address >= (heapStart + st->hpSize - HEAP_ALLOC_UNIT))
    {
      return eHUH;
    }

  /* Get the machine address of the free chunk and convert the memChunk_t
   * header on the in-use chunk to a freeChunk_t header.
   */

  address           -= sizeof(memChunk_t);
  freeChunk          = (freeChunk_t *)ATSTACK(st, address);
  freeChunk->next    = 0;
  freeChunk->prev    = 0;

  libexec_DisposeChunk(st, freeChunk);
  libexec_DumpHeap(st, "After free", address);
  return eNOERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void libexec_InitializeHeap(struct libexec_s *st)
{
  /* We can't use the memory manager if no heap was specified */

  if (st->hpSize > 2 * HEAP_ALLOC_UNIT)
    {
      uint16_t     heapStart = HEAP_ALIGNUP(st->hpb);
      uint16_t     heapEnd   = HEAP_ALIGNDOWN(heapStart + st->hpSize);
      uint16_t     heapSize;
      memChunk_t  *terminus;
      freeChunk_t *initialChunk;

      terminus                     = (memChunk_t *)
                                     ATSTACK(st, heapEnd - HEAP_ALLOC_UNIT);
      memset(terminus, 0, sizeof(memChunk_t));
      terminus->forward            = 0;
      terminus->address            = heapEnd - heapStart - HEAP_ALLOC_UNIT;
      terminus->inUse              = 1;

      heapSize                     = heapEnd - heapStart - HEAP_ALLOC_UNIT;
      terminus->back               = heapSize;

      initialChunk                 = (freeChunk_t *)ATSTACK(st, heapStart);
      memset(initialChunk, 0, sizeof(freeChunk_t));
      initialChunk->chunk.forward  = heapSize;
      initialChunk->chunk.address  = heapStart - heapStart;
      initialChunk->next           = 0;

      st->freeChunks               = initialChunk;

      libexec_DumpHeap(st, "Initially", heapStart);
    }
}

/****************************************************************************/

int libexec_New(struct libexec_s *st, uint16_t size)
{
  uint16_t addr = 0;
  int errorCode = eNEWFAILED;

  if (size > 0)
    {
      addr = libexec_Alloc(st, size);
      if (addr > 0)
        {
          errorCode = eNOERROR;
        }
    }

  PUSH(st, addr);
  return errorCode;
}

/****************************************************************************/

int libexec_Dispose(struct libexec_s *st, uint16_t address)
{
  return libexec_Free(st, address);
}

/****************************************************************************/

uint16_t libexec_AllocTmpString(struct libexec_s *st, uint16_t reqSize,
                                uint16_t *allocSize)
{
  uint16_t addr = 0;

  if (reqSize > 0 && reqSize <= STRING_BUFFER_MAX)
    {
      addr = libexec_Alloc(st, reqSize);
      if (addr > 0)
        {
          *allocSize = reqSize | HEAP_STRING;
        }
    }

  return addr;
}

/****************************************************************************/

int libexec_FreeTmpString(struct libexec_s *st, uint16_t allocAddr,
                          uint16_t allocSize)
{
  int errorCode = eNOERROR;

  /* Does this string buffer allocation lie in the heap? */

  if ((allocSize & HEAP_STRING) != 0)
    {
      /* Yes, free it */

      errorCode = libexec_Free(st, allocAddr);
    }

  return errorCode;
}
