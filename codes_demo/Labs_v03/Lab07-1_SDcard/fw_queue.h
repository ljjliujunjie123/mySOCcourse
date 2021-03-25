#ifndef __FW_QUEUE_H_
#define __FW_QUEUE_H_

#include <stdint.h>

typedef struct
{
  uint8_t* buffer;
  uint16_t size;
  uint16_t txpos;
  uint16_t rxpos;
  uint16_t space;
} Queue;

void initQueue(Queue* handle, uint8_t *buffer, uint16_t size);
uint16_t queue_CanWrite(Queue *handle);
uint16_t queue_CanRead(Queue *handle);
uint16_t queue_Write(Queue *handle, uint8_t *buffer, uint16_t size);
uint16_t queue_Read(Queue *handle, uint8_t *buffer, uint16_t size);
uint16_t queue_ThrowData(Queue *handle, uint16_t size);

#endif
