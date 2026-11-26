#include <assert.h>
#include <stdio.h>
#include "main.h"
#include "../Core/Src/led_link.c"

I2C_TypeDef mock_i2c;
GPIO_TypeDef mock_c;
I2C_HandleTypeDef hi2c1 = { &mock_i2c, 0 };
static uint32_t tick, irq_mask;
static unsigned listen_count, init_count, deinit_count, read_count;
static GPIO_PinState led;
static uint8_t *receive_buffer;
static HAL_StatusTypeDef listen_result, receive_result, init_result;

uint32_t HAL_GetTick(void) { return tick; }
uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1; }
void __enable_irq(void) { irq_mask = 0; }
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState value)
{
  assert(port == &mock_c && pin == PWM_LED_Pin);
  led = value;
}
HAL_StatusTypeDef HAL_I2C_EnableListen_IT(I2C_HandleTypeDef *h)
{
  assert(h == &hi2c1);
  ++listen_count;
  return listen_result;
}
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h)
{
  assert(h == &hi2c1);
  ++deinit_count;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h)
{
  assert(h == &hi2c1);
  ++init_count;
  h->ErrorCode = 0;
  return init_result;
}
HAL_StatusTypeDef HAL_I2C_Slave_Seq_Receive_IT(I2C_HandleTypeDef *h,
    uint8_t *p, uint16_t n, uint32_t options)
{
  assert(h == &hi2c1 && n == 1 && options == I2C_FIRST_AND_LAST_FRAME);
  receive_buffer = p;
  return receive_result;
}
HAL_StatusTypeDef HAL_I2C_Slave_Seq_Transmit_IT(I2C_HandleTypeDef *h,
    uint8_t *p, uint16_t n, uint32_t options)
{
  assert(h == &hi2c1 && n == 1 && *p == 0xFF && options == I2C_FIRST_AND_LAST_FRAME);
  ++read_count;
  return HAL_OK;
}

static void Reset(uint32_t now)
{
  tick = now;
  irq_mask = listen_count = init_count = deinit_count = read_count = 0;
  listen_result = receive_result = init_result = HAL_OK;
  hi2c1.ErrorCode = 0;
  LED_Link_Init();
  assert(led == GPIO_PIN_RESET && listen_count == 1);
}

static void ReceiveWithoutStop(uint8_t value)
{
  HAL_I2C_AddrCallback(&hi2c1, I2C_DIRECTION_TRANSMIT, 0x60);
  *receive_buffer = value;
  HAL_I2C_SlaveRxCpltCallback(&hi2c1);
}

static void Receive(uint8_t value)
{
  ReceiveWithoutStop(value);
  HAL_I2C_ListenCpltCallback(&hi2c1);
  LED_Link_Poll();
}

int main(void)
{
  Reset(0);
  ReceiveWithoutStop(1);
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET); /* No command committed before STOP. */
  HAL_I2C_ListenCpltCallback(&hi2c1);
  LED_Link_Poll();
  assert(led == GPIO_PIN_SET && listen_count == 2);
  tick = 499;
  LED_Link_Poll();
  assert(led == GPIO_PIN_SET);
  tick = 500;
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET);
  Receive(1);
  assert(led == GPIO_PIN_SET); /* Reconnect restores current state. */
  Receive(0);
  assert(led == GPIO_PIN_RESET);

  Reset(0);
  Receive(1);
  tick = 400;
  Receive(0xFF);
  assert(led == GPIO_PIN_SET && last_valid_ms == 0);
  tick = 500;
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET);

  Reset(0);
  Receive(1);
  tick = 400;
  Receive(1);
  tick = 800;
  LED_Link_Poll();
  assert(led == GPIO_PIN_SET); /* Heartbeats extend the watchdog. */
  tick = 900;
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET);

  Reset(0);
  ReceiveWithoutStop(1);
  hi2c1.ErrorCode = 1;
  HAL_I2C_ErrorCallback(&hi2c1);
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET && init_count == 1 && !restart_requested);
  Receive(1);
  assert(led == GPIO_PIN_SET);

  Reset(0);
  HAL_I2C_AddrCallback(&hi2c1, I2C_DIRECTION_TRANSMIT, 0x60);
  tick = 24;
  LED_Link_Poll();
  assert(init_count == 0);
  tick = 25;
  LED_Link_Poll();
  assert(init_count == 1 && !transfer_active && led == GPIO_PIN_RESET);

  Reset(0);
  receive_result = HAL_ERROR;
  HAL_I2C_AddrCallback(&hi2c1, I2C_DIRECTION_TRANSMIT, 0x60);
  init_result = HAL_ERROR;
  LED_Link_Poll();
  assert(init_count == 1 && restart_requested);
  tick = 9;
  LED_Link_Poll();
  assert(init_count == 1);
  tick = 10;
  init_result = HAL_OK;
  LED_Link_Poll();
  assert(init_count == 2 && !restart_requested);

  Reset(0);
  listen_result = HAL_ERROR;
  HAL_I2C_ListenCpltCallback(&hi2c1);
  assert(restart_requested);
  listen_result = HAL_OK;
  LED_Link_Poll();
  assert(!restart_requested);

  Reset(0);
  HAL_I2C_AddrCallback(&hi2c1, 1, 0x60);
  HAL_I2C_ListenCpltCallback(&hi2c1);
  assert(read_count == 1 && !have_valid_state);
  irq_mask = 1;
  LED_Link_Poll();
  assert(irq_mask == 1);

  Reset(UINT32_MAX - 250U);
  Receive(1);
  tick += 499U;
  LED_Link_Poll();
  assert(led == GPIO_PIN_SET);
  tick += 1U;
  LED_Link_Poll();
  assert(led == GPIO_PIN_RESET);
  puts("PASS PWM: STOP commit, on/off, watchdog, invalid byte, heartbeat, reconnect, error/retry, stalled transfer, read, tick wrap");
  return 0;
}
