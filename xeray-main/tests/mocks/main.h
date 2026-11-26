#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>

typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef unsigned GPIO_TypeDef;
typedef struct { uint32_t CTRL, CYCCNT; } DWT_Type;
typedef struct { uint32_t DEMCR; } CoreDebug_Type;
extern GPIO_TypeDef mock_a, mock_e;
extern CoreDebug_Type mock_debug;
DWT_Type *MockDWT(void);
#define DWT MockDWT()
#define CoreDebug (&mock_debug)
#define CoreDebug_DEMCR_TRCENA_Msk 1U
#define DWT_CTRL_CYCCNTENA_Msk 1U
#define PWM_SDA_GPIO_Port (&mock_a)
#define PWM_SCL_GPIO_Port (&mock_a)
#define DIN1_GPIO_Port (&mock_e)
#define PWM_SDA_Pin 256U
#define PWM_SCL_Pin 512U
#define DIN1_Pin 1U

uint32_t HAL_GetTick(void);
uint32_t HAL_RCC_GetHCLKFreq(void);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState value);
#endif
