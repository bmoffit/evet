#pragma once

#include <et.h>

// Attributes of et_event from et_event_getdata
typedef struct etChunkStat
{
  uint32_t *data;
  size_t length;
  int32_t endian;
  int32_t swap;

  int32_t  evioHandle;
} etChunkStat_t;

typedef struct evetHandle
{
  et_sys_id etSysId;
  et_att_id etAttId;
  et_event **etChunk;      // pointer to array of et_events (pe)
  int32_t  etChunkSize;    // user requested (et_events in a chunk)
  int32_t  etChunkNumRead; // actual read from et_events_get

  int32_t  currentChunkID;  // j
  etChunkStat_t currentChunkStat; // data, len, endian, swap

  int32_t verbose;

} evetHandle_t ;

int32_t  evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh);
int32_t  evetClose(evetHandle_t &evh);
int32_t  evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length);
