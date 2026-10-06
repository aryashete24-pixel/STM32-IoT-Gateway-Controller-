/**
 * dht11.c
 * Ported C Source file for the DHT11 library (STM32 HAL).
 */

#include "dht11.h"

/* Microsecond delay using TIM7 counter */
static void delayMicroseconds(TIM_HandleTypeDef *htim, uint16_t us) {
    __HAL_TIM_SET_COUNTER(htim, 0);
    while (__HAL_TIM_GET_COUNTER(htim) < us);
}

/* Transmit start pulse to wake the DHT11 */
static void startSignal(DHT11_HandleTypeDef *dht) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Configure as Output Push-Pull
    GPIO_InitStruct.Pin = dht->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(dht->port, &GPIO_InitStruct);

    // 2. Pull LOW for 18 ms
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_RESET);
    HAL_Delay(18);

    // 3. Pull HIGH for 30 us
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_SET);
    delayMicroseconds(dht->htim, 30);

    // 4. Switch pin to Input mode with internal pull-up
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(dht->port, &GPIO_InitStruct);
}

/* Read a single byte (8 bits) from the sensor */
static uint8_t readByte(DHT11_HandleTypeDef *dht) {
    uint8_t value = 0;

    for (int i = 0; i < 8; i++) {
        uint16_t timeout = 0;
        // Wait for pin to go HIGH (start of the bit pulse)
        while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET) {
            if (++timeout > 200) return 0;
            delayMicroseconds(dht->htim, 1);
        }

        // Measure pulse width: DHT11 transmits ~28us for '0' and ~70us for '1'.
        // Waiting 40us lets us sample the level right in between.
        delayMicroseconds(dht->htim, 40);

        if (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) {
            value |= (1 << (7 - i));
        }

        // Wait for pin to go LOW again (end of the bit pulse)
        timeout = 0;
        while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) {
            if (++timeout > 200) return 0;
            delayMicroseconds(dht->htim, 1);
        }
    }
    return value;
}

void DHT11_Init(DHT11_HandleTypeDef *dht, TIM_HandleTypeDef *htim, GPIO_TypeDef* port, uint16_t pin) {
    dht->htim = htim;
    dht->port = port;
    dht->pin = pin;
    dht->delayMS = 500;
    dht->temperature = 0;
    dht->humidity = 0;

    HAL_TIM_Base_Start(dht->htim);
}

void DHT11_setDelay(DHT11_HandleTypeDef *dht, uint32_t delay) {
    dht->delayMS = delay;
}

int DHT11_readRawData(DHT11_HandleTypeDef *dht, uint8_t data[5]) {
    // 1. Send start signal with interrupts enabled (HAL_Delay requires SysTick/TIM6)
    startSignal(dht);

    // 2. Disable interrupts ONLY during microsecond data reception
    __disable_irq();

    // 3. Wait for sensor response: DHT11 pulls low (80us) then high (80us)
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) {
        if (++timeout > 200) {
            __enable_irq();
            return DHT11_ERROR_TIMEOUT;
        }
        delayMicroseconds(dht->htim, 1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET) {
        if (++timeout > 200) {
            __enable_irq();
            return DHT11_ERROR_TIMEOUT;
        }
        delayMicroseconds(dht->htim, 1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) {
        if (++timeout > 200) {
            __enable_irq();
            return DHT11_ERROR_TIMEOUT;
        }
        delayMicroseconds(dht->htim, 1);
    }

    // 4. Read 5 bytes (40 bits)
    for (int i = 0; i < 5; i++) {
        data[i] = readByte(dht);
    }

    // 5. Re-enable interrupts immediately after timing-critical reads
    __enable_irq();

    // 6. Verify checksum: Byte 4 must equal sum of bytes 0..3
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return DHT11_SUCCESS;
    } else {
        return DHT11_ERROR_CHECKSUM;
    }
}

int DHT11_readTemperatureHumidity(DHT11_HandleTypeDef *dht, int *temperature, int *humidity) {
    uint8_t data[5] = {0};
    int error = DHT11_readRawData(dht, data);

    if (error != DHT11_SUCCESS) {
        return error;
    }

    *humidity = data[0];
    *temperature = data[2];

    dht->humidity = data[0];
    dht->temperature = data[2];

    return DHT11_SUCCESS;
}
