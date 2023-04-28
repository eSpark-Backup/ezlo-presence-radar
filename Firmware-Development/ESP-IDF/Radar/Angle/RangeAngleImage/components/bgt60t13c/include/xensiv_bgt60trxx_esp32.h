/***********************************************************************************************//**
 * \file xensiv_bgt60trxx_esp32.h
 *
 * \brief
 * XENSIV(TM) BGT60TRxx 60GHz FMCW radar sensors library platform dependencies
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2022 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#ifndef XENSIV_BGT60TRXX_PLATFORM_H_
#define XENSIV_BGT60TRXX_PLATFORM_H_

/**
 * \addtogroup group_board_libs_platform XENSIV BGT60TRxx radar sensor platform functions
 * \{
 * XENSIV&trade; BGT60TRxx radar sensor platform functions.
 *
 * To adapt the driver to your platform, you need to provide an implementation for the functions
 * declared in this file. See the example implementation in xensiv_bgt60trxx_mtb.c using the
 * PSoC&trade; 6 HAL.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
// #include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define MAX_FIFO_BREAK_COUNT 4
#define FIFO_RESET_COUNT 0

typedef enum 
{
    FIFO_NO_ERROR,
    FIFO_READ_FAILED,
    FIFO_READ_SUCCESS
} bgt60trxx_fifo_status_t;

#ifndef XNSIV_BGT60_SPI_HOST
    #define     XNSIV_BGT60_SPI_HOST       SPI2_HOST
#endif
    #if 0
#ifdef XNSIV_BGT60_SPI_HOST
    #define     XNSIV_BGT60_RST_GPIO      36
    #define     XNSIV_BGT60_MISO_GPIO     35
    #define     XNSIV_BGT60_MOSI_GPIO     37
    #define     XNSIV_BGT60_CLK_GPIO      13
    #define     XNSIV_BGT60_CS_GPIO       14
#endif

#define     XNSIV_BGT60_CS_PIN_SEL       (1ULL << XNSIV_BGT60_CS_GPIO)
#define     XNSIV_BGT60_RST_PIN_SEL      (1ULL << XNSIV_BGT60_RST_GPIO)

#define     XNSIV_BGT60_LED_GREEN         17  
#define     XNSIV_BGT60_LED_RED           18
#define     XNSIV_BGT60_LEDS_PIN_SEL      ((1ULL << XNSIV_BGT60_LED_GREEN) | (1ULL << XNSIV_BGT60_LED_RED))

#define     XNSIV_BGT60_BUTTON_GPIO       1
#define     XNSIV_BGT60_BUTTON_PIN_SEL    (1ULL << XNSIV_BGT60_BUTTON_GPIO)

#define     XNSIV_BGT60_IRQ_GPIO          38
#define     XNSIV_BGT60_IRQ_PIN_SEL       (1ULL<<XNSIV_BGT60_IRQ_GPIO)
#elif 0
#ifdef XNSIV_BGT60_SPI_HOST
    #define     XNSIV_BGT60_RST_GPIO      14
    #define     XNSIV_BGT60_MISO_GPIO     36
    #define     XNSIV_BGT60_MOSI_GPIO     35
    #define     XNSIV_BGT60_CLK_GPIO      37
    #define     XNSIV_BGT60_CS_GPIO       38
#endif

#define     XNSIV_BGT60_CS_PIN_SEL       (1ULL << XNSIV_BGT60_CS_GPIO)
#define     XNSIV_BGT60_RST_PIN_SEL      (1ULL << XNSIV_BGT60_RST_GPIO)

#define     XNSIV_BGT60_LED_GREEN         17  
#define     XNSIV_BGT60_LED_RED           18
#define     XNSIV_BGT60_LEDS_PIN_SEL      ((1ULL << XNSIV_BGT60_LED_GREEN) | (1ULL << XNSIV_BGT60_LED_RED))

#define     XNSIV_BGT60_BUTTON_GPIO       1
#define     XNSIV_BGT60_BUTTON_PIN_SEL    (1ULL << XNSIV_BGT60_BUTTON_GPIO)

#define     XNSIV_BGT60_IRQ_GPIO          13
#define     XNSIV_BGT60_IRQ_PIN_SEL       (1ULL<<XNSIV_BGT60_IRQ_GPIO)

#else
#ifdef XNSIV_BGT60_SPI_HOST
    #define     XNSIV_BGT60_RST_GPIO      14
    #define     XNSIV_BGT60_MISO_GPIO     6
    #define     XNSIV_BGT60_MOSI_GPIO     7
    #define     XNSIV_BGT60_CLK_GPIO      8
    #define     XNSIV_BGT60_CS_GPIO       10
#endif

#define     XNSIV_BGT60_CS_PIN_SEL       (1ULL << XNSIV_BGT60_CS_GPIO)
#define     XNSIV_BGT60_RST_PIN_SEL      (1ULL << XNSIV_BGT60_RST_GPIO)

#define     XNSIV_BGT60_LED_GREEN         17  
#define     XNSIV_BGT60_LED_RED           18
#define     XNSIV_BGT60_LEDS_PIN_SEL      ((1ULL << XNSIV_BGT60_LED_GREEN) | (1ULL << XNSIV_BGT60_LED_RED))

#define     XNSIV_BGT60_BUTTON_GPIO       1
#define     XNSIV_BGT60_BUTTON_PIN_SEL    (1ULL << XNSIV_BGT60_BUTTON_GPIO)

#define     XNSIV_BGT60_IRQ_GPIO          15
#define     XNSIV_BGT60_IRQ_PIN_SEL       (1ULL<<XNSIV_BGT60_IRQ_GPIO)
#endif

#define     XNSIV_BGT60_SPI_CLK           (uint32_t)(50 * 1000 * 1000)

#define XNSIV_CS_HANDLING

/**
 * @brief Platform-specific function that sets the output value of the RST pin.
 *
 * @param[in] iface Platform SPI interface object
 * @param[in] val The value to be set (high = true, low = false)
 */
void xensiv_bgt60trxx_esp32_rst_set(const void* iface, uint8_t val);

/**
 * @brief Platform-specific function that that sets the output value of the SPI CS pin.
 *
 * @param[in] iface Platform SPI interface object
 * @param[in] val The value to be set (high = true, low = false)
 */
void xensiv_bgt60trxx_esp32_spi_cs_set(const void* iface, uint8_t val);

/**
 * @brief Platform-specific function that performs a SPI write/read transfer to
 * the register file of the sensor.
 * Synchronously write a block of data out and receive a block of data in.
 * If the data that will be received is not important, pass NULL as rx_data.
 * If the data that will be transmitted is not important, pass NULL as tx_data.
 * Note that passing NULL as rxBuffer and txBuffer are considered invalid cases.
 *
 * @param[in] iface Platform SPI interface object.
 * @param[in] tx_data The pointer of the buffer with data to transmit.
 * @param[in] rx_data The pointer to the buffer to store received data.
 * @param[in] len The number of data elements to transmit and receive.
 * @return XENSIV_BGT60TRXX_STATUS_OK if the transfer is completed without errors,
 * otherwise returns XENSIV_BGT60TRXX_STATUS_COM_ERROR.
 */
int32_t xensiv_bgt60trxx_esp32_spi_transfer(void* iface,
                                               uint8_t* tx_data,
                                               uint8_t* rx_data,
                                               uint32_t len);

/**
 * @brief Platform-specific function that performs a SPI burst read to
 * receive a block of data from sensor FIFO.
 * ADC samples are stored in the sensor FIFO using 12bits.
 * It is expected to use SPI read transfers with a word length of 12bits.
 * It is expected to drive TX high while data is read in from RX.
 *
 * @param[in] iface Platform SPI interface object.
 * @param[in] rx_data The pointer to the buffer to store the received data.
 * @param[in] len The number of FIFO data elements of 12bits to receive.
 * @return XENSIV_BGT60TRXX_STATUS_OK if the read is completed without errors,
 * otherwise returns XENSIV_BGT60TRXX_STATUS_COM_ERROR.
 */
int32_t xensiv_bgt60trxx_esp32_spi_fifo_read(void* iface,
                                                uint16_t* rx_data,
                                                uint32_t len);

/**
 * @brief Platform-specific function that waits for a specified time period in milliseconds.
 *
 * @param[in] ms Number of milliseconds to wait for.
 */
void xensiv_bgt60trxx_esp32_delay(uint32_t ms);

/**
 * @brief Platform-specific function to reverse the byte order (32 bits).
 * A sample implementation would look like
 * \code
 *  return (((x & 0x000000ffUL) << 24) |
 *          ((x & 0x0000ff00UL) <<  8) |
 *          ((x & 0x00ff0000UL) >>  8) |
 *          ((x & 0xff000000UL) >> 24));
 * \endcode
 *
 * @param[in] x Value to reverse.
 * @return Reversed value.
 */
uint32_t xensiv_bgt60trxx_esp32_word_reverse(uint32_t x);

/**
 * @brief Platform-specific function that implements a runtime assertion; used to verify the
 * assumptions made by the program and take appropriate actions if the assumption is false.
 *
 * @param[in] expr Expression to be verified.
 */
void xensiv_bgt60trxx_esp32_assert(bool expr);

uint16_t* get_data_ptr(void);
#ifdef __cplusplus
}
#endif

/** \} group_board_libs_platform */

#endif // ifndef XENSIV_BGT60TRXX_PLATFORM_H_