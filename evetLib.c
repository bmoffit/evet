#include <byteswap.h>
#include "evetLib.h"

#define EVETCHECKINIT(x)					\
  if(x.etSysId == 0) {						\
    printf("%s: ERROR: evet not initiallized\n", __func__);	\
    return -1;}

int32_t
evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh)
{
  evh.etSysId = etSysId;
  evh.etChunkSize = chunk;

  evh.etAttId = 0;
  evh.currentChunkID = -1;
  evh.etChunkNumRead = -1;

  evh.currentChunkStat.evioHandle = 0;
  evh.currentChunkStat.length = 0;
  evh.currentChunkStat.endian = 0;
  evh.currentChunkStat.swap = 0;

  /* allocate some memory */
  evh.etChunk = (et_event **) calloc((size_t)chunk, sizeof(et_event *));
  if (evh.etChunk == NULL) {
    printf("%s: out of memory\n", __func__);
    evh.etSysId = 0;
    return -1;
  }

  return 0;
}

int32_t
evetClose(evetHandle_t &evh)
{

  // Close up any currently opened evBufferOpen's.
  if(evh.currentChunkStat.evioHandle)
    {
      int32_t stat = evClose(evh.currentChunkStat.evioHandle);
      if(stat != S_SUCCESS)
	{
	  printf("%s: ERROR: evClose returned %s\n",
		 __func__, et_perror(stat));
	  return -1;
	}
    }

  // put any events we may still have
  if(evh.etChunkNumRead != -1)
    {
      /* putting array of events */
      int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
      if (status != ET_OK)
	{
	  printf("%s: ERROR: et_events_put returned %s\n",
		 __func__, et_perror(status));
	  return -1;
	}
    }

  // free up the etChunk memory
  if(evh.etChunk)
    free(evh.etChunk);

  return 0;
}


int32_t
evetGetEtChunks(evetHandle_t &evh)
{
  if(evh.verbose == 1)
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh);

  int32_t status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk,
				 ET_SLEEP, NULL, evh.etChunkSize, &evh.etChunkNumRead);
  if(status != ET_OK)
    {
      printf("%s: ERROR: et_events_get returned (%d) %s\n",
	     __func__, status, et_perror(status));

      return -1;
    }

  evh.currentChunkID = -1;

  return 0;
}

int32_t
evetGetChunk(evetHandle_t &evh)
{
  if(evh.verbose == 1)
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh);

  evh.currentChunkID++;

  if((evh.currentChunkID >= evh.etChunkNumRead) || (evh.etChunkNumRead == -1))
    {
      if(evh.etChunkNumRead != -1)
	{
	  /* putting array of events */
	  int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
	  if (status != ET_OK)
	    {
	      printf("%s: ERROR: et_events_put returned %s\n",
		     __func__, et_perror(status));
	      return -1;
	    }
	}

      // out of chunks.  get some more
      int32_t stat = evetGetEtChunks(evh);
      if(stat != 0)
	{
	  printf("%s: ERROR: evetGetEtChunks(evh) returned %d\n",
		 __func__, stat);
	  return -1;
	}
        evh.currentChunkID++;

    }

  // Close previous handle
  if(evh.currentChunkStat.evioHandle)
    {
      int32_t stat = evClose(evh.currentChunkStat.evioHandle);
      if(stat != ET_OK)
	{
	  printf("%s: ERROR: evClose returned %s\n",
		 __func__, et_perror(stat));
	  return -1;
	}
    }

  et_event *currentChunk = evh.etChunk[evh.currentChunkID];
  et_event_getdata(currentChunk, (void **) &evh.currentChunkStat.data);
  et_event_getlength(currentChunk, &evh.currentChunkStat.length);
  et_event_getendian(currentChunk, &evh.currentChunkStat.endian);
  et_event_needtoswap(currentChunk, &evh.currentChunkStat.swap);

  if(evh.verbose == 1)
    {
      uint32_t *data = evh.currentChunkStat.data;
      uint32_t idata = 0, len = evh.currentChunkStat.length;

      printf("data byte order = %s\n",
	     (evh.currentChunkStat.endian == ET_ENDIAN_BIG) ? "BIG" : "LITTLE");
      printf(" %2d/%2d: data (len = %d) %s  int = %d\n",
	     evh.currentChunkID, evh.etChunkNumRead,
	     (int) len,
	     evh.currentChunkStat.swap ? "needs swapping" : "does not need swapping",
	     evh.currentChunkStat.swap ? bswap_32(data[0]) : data[0]);

      for(idata = 0; idata < ((32 < (len>>2)) ? 32 : (len>>2)); idata++)
	{
	  printf("0x%08x ", evh.currentChunkStat.swap ? bswap_32(data[idata]) : data[idata]);
	  if(((idata+1) % 8) == 0)
	    printf("\n");
	}
      printf("\n");
    }

  int32_t evstat = evOpenBuffer((char *) evh.currentChunkStat.data, evh.currentChunkStat.length,
			    (char *)"r",  &evh.currentChunkStat.evioHandle);

  if(evstat != 0)
    {
      printf("%s: ERROR: evOpenBuffer returned %s\n",
	     __func__, et_perror(evstat));
    }

  return evstat;
}

int32_t
evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length)
{
  if(evh.verbose == 1)
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh);

  int32_t status = evReadNoCopy(evh.currentChunkStat.evioHandle,
				outputBuffer, length);
  if(status != S_SUCCESS)
    {
      // Get a new chunk from et_get_event
      status = evetGetChunk(evh);
      if(status == 0)
	{
	  status = evReadNoCopy(evh.currentChunkStat.evioHandle,
				outputBuffer, length);
	}
      else
	{
	  printf("%s: ERROR: evetGetChunk failed %d\n",
		 __func__, status);
	  return -1;
	}
    }

  return 0;
}
