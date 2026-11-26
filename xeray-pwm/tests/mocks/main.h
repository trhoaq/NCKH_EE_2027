#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>

typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef unsigned GPIO_TypeDef;
typedef struct { uint32_t CR1, CR2; } I2C_TypeDef;
typedef struct { I2C_TypeDef *Instance; uint32_t ErrorCode; } I2C_HandleTypeDef;
extern I2C_TypeDef mock_i2c;
extern GPIO_TypeDef mock_c;
#define I2C1 (&mock_i2c)
#define I2C_IT_EVT 1U
#define I2C_IT_BUF 2U
#define I2C_IT_ERR 4U
#define I2C_CR1_ACK 0x400U
#define I2C_FIRST_AND_LAST_FRAME 8U
#define I2C_DIRECTION_TRANSMIT 0U
#define HAL_I2C_ERROR_NONE 0U
#define PWM_LED_GPIO_Port (&mock_c)
#define PWM_LED_Pin 8192U
#define CLEAR_BIT(reg, bit) ((reg) &= ~(bit))
#define __HAL_I2C_DISABLE_IT(h, mask) ((h)->Instance->CR2 &= ~(mask))
#define __HAL_I2C_DISABLE(h) ((h)->Instance->CR1 &= ~1U)

uint32_t HAL_GetTick(void);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __enable_irq(void);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState value);
HAL_StatusTypeDef HAL_I2C_EnableListen_IT(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_Slave_Seq_Receive_IT(I2C_HandleTypeDef *h, uint8_t *p, uint16_t n, uint32_t options);
HAL_StatusTypeDef HAL_I2C_Slave_Seq_Transmit_IT(I2C_HandleTypeDef *h, uint8_t *p, uint16_t n, uint32_t options);
#endif
