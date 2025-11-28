cmake_minimum_required(VERSION 3.13)
set(BUILD_TARGET ps_2_freertos)

set(DIR_WORK /home/yasuno/Developments)

set(PICO_BOARD pico)
## or
#set(PICO_BOARD pico_w)

set(PICO_SDK_PATH ${DIR_WORK}/RP2040/pico-sdk)
set(FREERTOS_KERNEL_PATH ${DIR_WORK}/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel)

if(PICO_CYW43_SUPPORTED)
	set(CYW43_WL_GPIO_LED_PIN 1)
endif()

if(NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Skipping FreeRTOS examples as FREERTOS_KERNEL_PATH not defined")
    return()
endif()

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)
include(target_pico_w.cmake)

project(${BUILD_TARGET}_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(${BUILD_TARGET}
	${BUILD_TARGET}.c
	spi_slave.c spi_slave.h
	buffer.c buffer.h
	hostside.c hostside.h
)

# pico_generate_pio_header(${BUILD_TARGET} ${CMAKE_CURRENT_LIST_DIR}/ps_2.pio)

pico_enable_stdio_usb( ${BUILD_TARGET} 0 )
pico_enable_stdio_uart( ${BUILD_TARGET} 1 )

if(PICO_CYW43_SUPPORTED)
	# For led support on pico_w
	target_link_libraries(${BUILD_TARGET} PRIVATE
		pico_async_context_freertos
		FreeRTOS-Kernel-Heap4
		pico_stdlib
		hardware_spi
		pico_cyw43_arch_none
	)
else()
	target_link_libraries(${BUILD_TARGET}
		pico_async_context_freertos
		FreeRTOS-Kernel-Heap4
		pico_stdlib
		hardware_spi
	)
endif()

target_include_directories(${BUILD_TARGET} PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
	${FREERTOS_KERNEL_PATH}/include
#		${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
	${PICO_SDK_PATH}/pico-sdk/src/rp2_common/pico_async_context/include
	${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
#	${PICO_SDK_PATH}/src/boards/include/boards/
	${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
)


target_compile_definitions(${BUILD_TARGET} PRIVATE
	configNUMBER_OF_CORES=2
)

pico_add_extra_outputs(
	${BUILD_TARGET}
)
