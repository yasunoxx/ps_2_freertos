/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "buffer.h"
#include "hostside.h"
#include "spi_slave.h"

// Which core to run on if configNUMBER_OF_CORES==1
#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif

// Whether to flash the led
#ifndef USE_LED
//#define USE_LED 1
#endif

// Whether to busy wait in the led thread
#ifndef LED_BUSY_WAIT
#define LED_BUSY_WAIT 1
#endif

// Delay between led blinking
#define LED_DELAY_MS 500

// Priorities of our threads - higher numbers are higher priority
#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1UL )
#define BUFFER_TASK_PRIORITY    ( tskIDLE_PRIORITY + 1UL )
#define HOSTSIDE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )


// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define BLINK_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define BUFFER_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define HOSTSIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define BUFFER_WAIT_IN_MS 10
#define HOSTSIDE_WAIT_IN_MS 10

#ifdef NOW_DEBUG_ASYNC_CONTEXT
///////////////////////////////////////////////////////////////////////////////
#include "pico/async_context_freertos.h"
// rp2_common/pico_async_context/include/pico/async_context_freertos.h
static async_context_freertos_t async_context_instance;
#define WORKER_TASK_PRIORITY    ( tskIDLE_PRIORITY + 4UL )
#define WORKER_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
///////////////////////////////////////////////////////////////////////////////
#endif // NOW_DEBUG_ASYNC_CONTEXT


#if USE_LED
static void pico_set_led( bool );
static void pico_init_led( void );
void blink_task( void * );
#endif // USE_LED

void buffer_task( void * );
void hostside_task( void * );

void main_task( __unused void *params )
{
#ifdef NOW_DEBUG_ASYNC_CONTEXT
    async_context_t *context = ps_2_async_context();
    // start the worker running
//    async_context_add_at_time_worker_in_ms( context, &worker_timeout, 0 );
    async_context_set_work_pending( context, &worker_timeout );
    async_context_freertos_add_when_pending_worker( context, &worker_timeout );
#endif // NOW_DEBUG_ASYNC_CONTEXT

    // start Buffer management
    xTaskCreate( buffer_task, "BufferThread",
                 BUFFER_TASK_STACK_SIZE, NULL, BUFFER_TASK_PRIORITY, NULL );
#if USE_LED
    // start the led blinking
    xTaskCreate( blink_task, "BlinkThread",
                 BLINK_TASK_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL );
#else
    // start HostSide management
    xTaskCreate( hostside_task, "HostSideThread",
                 HOSTSIDE_TASK_STACK_SIZE, NULL, HOSTSIDE_TASK_PRIORITY, NULL );
#endif

    int count = 0;
    while( true )
    {
#if configNUMBER_OF_CORES > 1
        static int last_core_id = -1;
        if( portGET_CORE_ID() != last_core_id )
        {
            last_core_id = portGET_CORE_ID();
//            printf( "main task is on core %d\n", last_core_id );
            printf( "main core %d\n", last_core_id );
        }
#endif
        printf( "Hello from main = %u\n", count++ );
        vTaskDelay( 3000 );
    }

//    async_context_deinit(context);
}

void vLaunch( void )
{
    TaskHandle_t task;
    xTaskCreate( main_task, "MainThread",
                 MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task );

#if configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main( void )
{
    stdio_init_all();
    Buffer_Init();
    SPI_Init();

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if (configNUMBER_OF_CORES > 1)
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if (configNUMBER_OF_CORES > 1)
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif (RUN_FREE_RTOS_ON_CORE == 1 && configNUMBER_OF_CORES==1)
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}

xQueueHandle pKeyQueue;

void buffer_task( __unused void *params )
{
    while( true )
    {
#if configNUMBER_OF_CORES > 1
        static int last_core_id = -1;
        if( portGET_CORE_ID() != last_core_id )
        {
            last_core_id = portGET_CORE_ID();
            printf( "buffer task is on core %d\n", last_core_id );
        }
#endif
        while( 1 )
        {
            int16_t keycode = Buffer_Read1Byte();
            if( keycode >= 0 )
            {
                uint8_t keycode8 = ( uint8_t )keycode;
                printf( "keycode %02x\n", keycode8 );
//                xQueueSend( pKeyQueue, &keycode8, portMAX_DELAY );	// synchronous message
            }
            else
            {
                break;
            }
        }

        vTaskDelay( BUFFER_WAIT_IN_MS );
    }
}

void hostside_task( __unused void *params )
{
    uint8_t keycode;
    while( true )
    {
#if configNUMBER_OF_CORES > 1
        static int last_core_id = -1;
        if( portGET_CORE_ID() != last_core_id )
        {
            last_core_id = portGET_CORE_ID();
            printf( "hostside task is on core %d\n", last_core_id );
        }
#endif
#ifdef DEBUG
        while( 1 )
        {
            if( xQueueReceive( pKeyQueue, &keycode, 0 ) == pdPASS )
            {
                printf( "keycode %02x\n", ( uint8_t )keycode );
            }
            else
            {
                break;
            }
        }
#endif // DEBUG
        vTaskDelay( HOSTSIDE_WAIT_IN_MS );
    }
}

#if USE_LED
// Turn led on or off
static void pico_set_led( bool led_on )
{
#if defined PICO_DEFAULT_LED_PIN
    gpio_put( PICO_DEFAULT_LED_PIN, led_on );
#elif defined( CYW43_WL_GPIO_LED_PIN )
    cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, led_on );
#endif
}

// Initialise led
static void pico_init_led( void )
{
#if defined PICO_DEFAULT_LED_PIN
    gpio_init( PICO_DEFAULT_LED_PIN );
    gpio_set_dir( PICO_DEFAULT_LED_PIN, GPIO_OUT );
#elif defined( CYW43_WL_GPIO_LED_PIN )
    hard_assert( cyw43_arch_init() == PICO_OK );
    pico_set_led( false ); // make sure cyw43 is started
#endif
}

void blink_task( __unused void *params )
{
    bool on = false;
    printf( "blink_task starts\n" );
    pico_init_led();
    while( true )
    {
#if configNUMBER_OF_CORES > 1
        static int last_core_id = -1;
        if( portGET_CORE_ID() != last_core_id )
        {
            last_core_id = portGET_CORE_ID();
//            printf( "blink task is on core %d\n", last_core_id );
        }
#endif
        pico_set_led( on );
        on = !on;

#if LED_BUSY_WAIT
        // You shouldn't usually do this. We're just keeping the thread busy,
        // experiment with BLINK_TASK_PRIORITY and LED_BUSY_WAIT to see what happens
        // if BLINK_TASK_PRIORITY is higher than TEST_TASK_PRIORITY main_task won't get any free time to run
        // unless configNUMBER_OF_CORES > 1
        busy_wait_ms( LED_DELAY_MS );
#else
        sleep_ms( LED_DELAY_MS );
#endif
    }
}
#endif // USE_LED


#ifdef NOW_DEBUG_ASYNC_CONTEXT
///////////////////////////////////////////////////////////////////////////////
// Create an async context
static async_context_t *ps_2_async_context( void )
{
    async_context_freertos_config_t config = async_context_freertos_default_config();
    config.task_priority = WORKER_TASK_PRIORITY; // defaults to ASYNC_CONTEXT_DEFAULT_FREERTOS_TASK_PRIORITY
    config.task_stack_size = WORKER_TASK_STACK_SIZE; // defaults to ASYNC_CONTEXT_DEFAULT_FREERTOS_TASK_STACK_SIZE
    if ( !async_context_freertos_init( &async_context_instance, &config ) )
    {
        return NULL;
    }
    async_context_instance.core.flags = ASYNC_CONTEXT_FLAG_CALLBACK_FROM_IRQ;
    return &async_context_instance.core;
}

extern bool async_context_freertos_add_when_pending_worker(async_context_t *self_base, async_when_pending_worker_t *worker);

// async workers run in their own thread when using async_context_freertos_t with priority WORKER_TASK_PRIORITY
// static void async_do_work( async_context_t *context, async_at_time_worker_t *worker )
static void async_do_work( async_context_t *context, async_when_pending_worker_t *worker )
{
//    async_context_add_at_time_worker_in_ms( context, worker, 10000 );
    async_context_freertos_add_when_pending_worker( context, worker );
    static uint32_t count = 0;
    printf( "Hello from worker count=%u\n", count++ );
#if configNUMBER_OF_CORES > 1
        static int last_core_id = -1;
        if( portGET_CORE_ID() != last_core_id )
        {
            last_core_id = portGET_CORE_ID();
            printf( "worker is on core %d\n", last_core_id );
        }
#endif
}

// .do_work is pointer to the function, member in async_at_time_worker_t
// async_at_time_worker_t worker_timeout = { .do_work = async_do_work };
async_when_pending_worker_t worker_timeout = { .do_work = async_do_work };
///////////////////////////////////////////////////////////////////////////////
#endif // NOW_DEBUG_ASYNC_CONTEXT
