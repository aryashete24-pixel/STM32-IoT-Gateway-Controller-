/**
 * dht11.h
 * Ported C Header file for the DHT11 library (STM32 HAL).
 */

#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "stm32f4xx_hal.h"

// Error codes
#define DHT11_SUCCESS          0
#define DHT11_ERROR_CHECKSUM 254
#define DHT11_ERROR_TIMEOUT  253

typedef struct {
    GPIO_TypeDef* port;       // GPIO Port (e.g., GPIOA)
    uint16_t pin;             // GPIO Pin (e.g., GPIO_PIN_1)
    TIM_HandleTypeDef* htim;  // Hardware Timer handle (TIM7)
    uint32_t delayMS;         // Internal delay
    int temperature;          // Stored temperature output (°C)
    int humidity;             // Stored humidity output (%)
} DHT11_HandleTypeDef;

void DHT11_Init(DHT11_HandleTypeDef *dht, TIM_HandleTypeDef *htim, GPIO_TypeDef* port, uint16_t pin);
void DHT11_setDelay(DHT11_HandleTypeDef *dht, uint32_t delay);

int DHT11_readRawData(DHT11_HandleTypeDef *dht, uint8_t data[5]);
int DHT11_readTemperature(DHT11_HandleTypeDef *dht);
int DHT11_readHumidity(DHT11_HandleTypeDef *dht);
int DHT11_readTemperatureHumidity(DHT11_HandleTypeDef *dht, int *temperature, int *humidity);

#endif /* INC_DHT11_H_ */
