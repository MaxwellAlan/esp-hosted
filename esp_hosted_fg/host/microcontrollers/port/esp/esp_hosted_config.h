/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_HOSTED_CONFIG_H__
#define __ESP_HOSTED_CONFIG_H__

#include "sdkconfig.h"
#include "esp_task.h"
#include "hosted_os_adapter.h"

/* This file is to tune the main ESP-Hosted configurations.
 * In case you are not sure of some value, Let it be default.
 **/


/*  ========================== SPI Master Config start ======================  */
/*
Pins in use. The SPI Master can use the GPIO mux,
so feel free to change these if needed.
*/

enum {
	SLAVE_CHIPSET_ESP32,
	SLAVE_CHIPSET_ESP32C2,
	SLAVE_CHIPSET_ESP32C3,
	SLAVE_CHIPSET_ESP32C6,
	SLAVE_CHIPSET_ESP32S2,
	SLAVE_CHIPSET_ESP32S3,
	SLAVE_CHIPSET_MAX_SUPPORTED
};

#define ESP_CHIPSET_USED                             SLAVE_CHIPSET_ESP32C3
#ifndef ESP_CHIPSET_USED
#error "Choose **slave** ESP chipset type to use with this host"
#endif

#if (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32)
  #define CONFIG_IDF_TARGET_ESP32 1
#elif (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32C2)
  #define CONFIG_IDF_TARGET_ESP32C2 1
#elif (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32C3)
  #define CONFIG_IDF_TARGET_ESP32C3 1
#elif (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32C6)
  #define CONFIG_IDF_TARGET_ESP32C6 1
#elif (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32S2)
  #define CONFIG_IDF_TARGET_ESP32S2 1
#elif (ESP_CHIPSET_USED==SLAVE_CHIPSET_ESP32S3)
  #define CONFIG_IDF_TARGET_ESP32S3 1
#endif

/* SPI config */
#define H_GPIO_HANDSHAKE_Port                        NULL
#define H_GPIO_HANDSHAKE_Pin                         CONFIG_ESP_SPI_GPIO_HANDSHAKE
#define H_GPIO_DATA_READY_Port                       NULL
#define H_GPIO_DATA_READY_Pin                        CONFIG_ESP_SPI_GPIO_DATA_READY

#define H_GPIO_MOSI_Port                             NULL
#define H_GPIO_MOSI_Pin                              CONFIG_ESP_SPI_GPIO_MOSI
#define H_GPIO_MISO_Port                             NULL
#define H_GPIO_MISO_Pin                              CONFIG_ESP_SPI_GPIO_MISO
#define H_GPIO_SCLK_Port                             NULL
#define H_GPIO_SCLK_Pin                              CONFIG_ESP_SPI_GPIO_CLK
#define H_GPIO_CS_Port                               NULL
#define H_GPIO_CS_Pin                                CONFIG_ESP_SPI_GPIO_CS
#define H_GPIO_PIN_RESET_Port                        NULL
#define H_GPIO_PIN_RESET_Pin                         CONFIG_ESP_SPI_GPIO_RESET_SLAVE

#define SPI_MODE                                     3
#define SPI_INIT_CLK_MHZ                             10


/*  ========================== SPI Master Config end ========================  */


#define TIMEOUT_PSERIAL_RESP                         30

#define H_GPIO_LOW                                   0
#define H_GPIO_HIGH                                  1

#define PRE_FORMAT_NEWLINE_CHAR                      ""
#define POST_FORMAT_NEWLINE_CHAR                     "\n"

#define USE_STD_C_LIB_MALLOC                         1


#endif /*__ESP_HOSTED_CONFIG_H__*/
