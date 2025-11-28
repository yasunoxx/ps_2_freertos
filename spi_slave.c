// spi_slave.c -- Motorola SPI slave mode for RP2040

// Original Copyright:
// Copyright (c) 2021 Michael Stoops. All rights reserved.
// Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
// following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//    following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
//    products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Example of an SPI bus slave using the PL022 SPI interface

// modify for ps_2_freertos (C)2025 yasunoxx▼Julia

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
// and rp2040/hardware_structs/include/hardware/structs/spi.h
#include "hardware/regs/spi.h"
#include "hardware/irq.h"

#include "buffer.h"
// or pico/util/queue.h ??
#include "spi_slave.h"

static void on_spi_rx( void );

void reenable_spi( spi_inst_t *spi )
{
    // Disable the SPI
    hw_clear_bits(&spi_get_hw(spi)->cr1, SPI_SSPCR1_SSE_BITS);

    // enable the SPI
    hw_set_bits(&spi_get_hw(spi)->cr1, SPI_SSPCR1_SSE_BITS);
}

static inline void spi_set_irqs_enabled( int spi_irq )
{
    if( spi_irq == SPI0_IRQ )
    {
        spi0_hw->imsc = ( true << SPI_SSPIMSC_RXIM_LSB ) |
                        ( true << SPI_SSPIMSC_RTIM_LSB );
    }
}

// backwards compatibility with SDK version < 2.0.0
static inline void spi_set_irq_enables( int spi_irq )
{
    spi_set_irqs_enabled( spi_irq );
}

void set_irq_spi( spi_inst_t *spi )
{
    // Set up a RX interrupt
    int SPI_IRQ = SPI0_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler( SPI_IRQ, on_spi_rx );
    irq_set_enabled( SPI_IRQ, true );

    // Now enable the UART to send interrupts - RX only
    spi_set_irq_enables( SPI_IRQ );
}

int SPI_Init()
{
#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning spi/spi_slave example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    printf("SPI initialize\n");

    // Enable SPI 0 at 1 MHz and connect to GPIOs
    spi_init( spi_default, 800 * 1000 );
    spi_set_format( spi_default, 11, 1, 0, SPI_MSB_FIRST );
    spi_set_slave( spi_default, true );

//  PICO_DEFAULT_SPI_*_PIN: configured in /boards/include/boards/*.h
    gpio_set_function( PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI );
    gpio_set_function( PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI );
    gpio_set_function( PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI );
//    gpio_set_function( PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI );
    // Make the SPI pins available to picotool
    bi_decl( bi_3pins_with_func(
            PICO_DEFAULT_SPI_RX_PIN,
            PICO_DEFAULT_SPI_TX_PIN,
            PICO_DEFAULT_SPI_SCK_PIN,
//            PICO_DEFAULT_SPI_CSN_PIN,
            GPIO_FUNC_SPI
    ) );

    set_irq_spi( spi_default );
//    while( 1 );
#endif

    return 0;
}

// #define DEBUG
static void on_spi_rx()
{
    uint16_t in_buf[ 2 ];

#define PS_2_STARTBIT   0b010000000000
#define PS_2_PARITYBIT  0b000000000010
#define PS_2_STOPBIT    0b000000000001

    spi_read16_blocking( spi_default, 0, in_buf, 1 );
#ifdef DEBUG
    printf( "%04x -> ", in_buf[ 0 ] );
#endif
    volatile uint8_t keytmp = ( in_buf[ 0 ] >> 2 ) & 0x0FF;
    volatile uint8_t keycode = 0;
    // rotate MSB first -> LSB first
    if( ( keytmp & 0b10000000 ) != 0 ) keycode |=        0b1;
    if( ( keytmp &  0b1000000 ) != 0 ) keycode |=       0b10;
    if( ( keytmp &   0b100000 ) != 0 ) keycode |=      0b100;
    if( ( keytmp &    0b10000 ) != 0 ) keycode |=     0b1000;
    if( ( keytmp &     0b1000 ) != 0 ) keycode |=    0b10000;
    if( ( keytmp &      0b100 ) != 0 ) keycode |=   0b100000;
    if( ( keytmp &       0b10 ) != 0 ) keycode |=  0b1000000;
    if( ( keytmp &        0b1 ) != 0 ) keycode |= 0b10000000;

#ifdef DEBUG
    if( ( in_buf[ 0 ] & PS_2_STARTBIT ) == 0 )
    {
        printf( "o" );
    }
    else
    {
        printf( "x" );
    }
    printf( "%08b", keytmp );
    if( ( in_buf[ 0 ] & PS_2_PARITYBIT ) == 0 )
    {
        printf( "P" );
    }
    else
    {
        printf( "X" );
    }
    if( ( in_buf[ 0 ] & PS_2_STOPBIT ) == 0 )
    {
        printf( "x " );
    }
    else
    {
        printf( "o " );
    }
#endif
//    printf( "keycode %02x\n", keycode );
    Buffer_Write1Byte( keycode );
    reenable_spi( spi_default );
}
