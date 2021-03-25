#include <msp430.h>
#include <string.h>
#include "fw_queue.h"
#include "fw_public.h"

void initQueue(Queue* handle, uint8_t *buffer, uint16_t size)
{
  handle->buffer = buffer;
  handle->size = size;
  handle->txpos = 0;
  handle->rxpos = 0;
  handle->space = size;
}

uint16_t queue_CanWrite(Queue *handle)
{
  return handle->space;
}

uint16_t queue_CanRead(Queue *handle)
{
  return handle->size - handle->space;
}

uint16_t queue_Write(Queue *handle, uint8_t *buffer, uint16_t size)
{
  uint16_t gie;
  _ECRIT(gie);

  if(size > queue_CanWrite(handle))
    size = queue_CanWrite(handle);
     
  uint16_t length = 0;
  if(handle->rxpos + size > handle->size)
  {
    length = handle->size - handle->rxpos;
    memcpy(handle->buffer + handle->rxpos, buffer, length);
    handle->rxpos = 0;
  }
  memcpy(handle->buffer + handle->rxpos, buffer + length, size - length);
  handle->rxpos += size - length;
  handle->space -= size;

  _LCRIT(gie);

  return size;
}

uint16_t queue_Read(Queue *handle, uint8_t *buffer, uint16_t size)
{
  uint16_t gie;
  _ECRIT(gie);

  if(size > queue_CanRead(handle))
    size = queue_CanRead(handle);
  
  uint16_t length = 0;
  if(handle->txpos + size > handle->size)
  {
    length = handle->size - handle->txpos;
    memcpy(buffer, handle->buffer + handle->txpos, length);
    handle->txpos = 0;
  }
  memcpy(buffer + length, handle->buffer + handle->txpos, size - length);
  handle->txpos += size - length;
  handle->space += size;

  _LCRIT(gie);

  return size;
}

uint16_t queue_ThrowData(Queue *handle, uint16_t size)
{
  uint16_t gie;
  _ECRIT(gie);

  if(size > queue_CanRead(handle))
  size = queue_CanRead(handle);

  uint16_t length = 0;
  if(handle->txpos + size > handle->size)
  {
    length = handle->size - handle->txpos;
    handle->txpos = 0;
  }
  handle->txpos += size - length;
  handle->space += size;

  _LCRIT(gie);

  return size;
}
