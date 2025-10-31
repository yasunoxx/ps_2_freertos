// buffer.h

// MAX_BUFFER_SIZE: 64, 128, 256 etc.
#define MAX_BUFFER_SIZE         64
#define MAX_BUFFER_SIZE_MASK    ( MAX_BUFFER_SIZE - 1 )

extern uint8_t Buffer[ MAX_BUFFER_SIZE ];
extern volatile uint16_t Buffer_WritePtr;
extern volatile uint16_t Buffer_ReadPtr;
extern volatile bool Buffer_WriteReady;

void Buffer_Init( void );
void Buffer_Write1Byte( uint8_t );
int16_t Buffer_Read1Byte( void );
