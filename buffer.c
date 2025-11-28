// buffer.c -- FIFO Buffer & Message Queue management
//             or use pico/util/queue.h, if you want it.

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"

uint8_t Buffer[ MAX_BUFFER_SIZE ];
volatile uint16_t Buffer_WritePtr;
volatile uint16_t Buffer_ReadPtr;
volatile bool Buffer_WriteReady;

void Buffer_Init()
{
    Buffer_WriteReady = false;

    Buffer_WritePtr = 0;
    Buffer_ReadPtr = 0;

    Buffer_WriteReady = true;
}

void Buffer_Write1Byte( uint8_t data )
{
    while( 1 )
    {
        if( Buffer_WriteReady == true )
        {
            Buffer_WriteReady = false;
            break;
        }
    }

    Buffer[ Buffer_WritePtr ] = data;
    Buffer_WritePtr++;
    Buffer_WritePtr &= MAX_BUFFER_SIZE_MASK;

    Buffer_WriteReady = true;
}

int16_t Buffer_Read1Byte()
{
    uint8_t data;

    if( Buffer_WritePtr == Buffer_ReadPtr ) return -1;

    data = Buffer[ Buffer_ReadPtr ];
    Buffer_ReadPtr++;
    Buffer_ReadPtr &= MAX_BUFFER_SIZE_MASK;

    return ( int16_t )data;
}

