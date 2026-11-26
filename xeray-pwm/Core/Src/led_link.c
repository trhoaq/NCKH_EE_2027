#include "led_link.h"
#include "main.h"

#define LINK_TIMEOUT_MS        500U
#define TRANSFER_TIMEOUT_MS    25U
#define RESTART_RETRY_MS       10U

extern I2C_HandleTypeDef hi2c1;

static uint8_t rx_byte;
static uint8_t tx_unused = 0xFFU;
static volatile uint8_t rx_pending;
static volatile uint8_t led_requested;
static volatile uint8_t have_valid_state;
static volatile uint8_t restart_requested;
static volatile uint8_t transfer_active;
static volatile uint32_t last_valid_ms;
static volatile uint32_t transfer_started_ms;
static uint32_t last_restart_ms;

static void RequestRestart(void)
{
  /* Quiesce the IRQ source; HAL deinit/init runs outside the ISR. */
  __HAL_I2C_DISABLE_IT(&hi2c1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR);
  __HAL_I2C_DISABLE(&hi2c1);
  rx_pending = 0U;
  transfer_active = 0U;
  restart_requested = 1U;
}

void LED_Link_Init(void)
{
  HAL_GPIO_WritePin(PWM_LED_GPIO_Port, PWM_LED_Pin, GPIO_PIN_RESET);
  rx_pending = 0U;
  led_requested = 0U;
  have_valid_state = 0U;
  transfer_active = 0U;
  restart_requested = 0U;
  last_valid_ms = HAL_GetTick();
  last_restart_ms = last_valid_ms - RESTART_RETRY_MS;
  if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
  {
    RequestRestart();
  }
}

void LED_Link_Poll(void)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint32_t now = HAL_GetTick();

  if (transfer_active &&
      (uint32_t)(now - transfer_started_ms) >= TRANSFER_TIMEOUT_MS)
  {
    RequestRestart();
  }

  /* Snapshot and write atomically with respect to the receive callbacks. */
  GPIO_PinState output = have_valid_state && led_requested &&
      (uint32_t)(now - last_valid_ms) < LINK_TIMEOUT_MS
      ? GPIO_PIN_SET : GPIO_PIN_RESET;
  HAL_GPIO_WritePin(PWM_LED_GPIO_Port, PWM_LED_Pin, output);
  uint8_t restart = restart_requested;
  if (!primask)
  {
    __enable_irq();
  }

  if (restart && (uint32_t)(now - last_restart_ms) >= RESTART_RETRY_MS)
  {
    last_restart_ms = now;
    if (HAL_I2C_DeInit(&hi2c1) == HAL_OK && HAL_I2C_Init(&hi2c1) == HAL_OK)
    {
      /* Clear before enabling interrupts, so a new IRQ error is not lost. */
      restart_requested = 0U;
      if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
      {
        RequestRestart();
      }
    }
  }
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c,
                          uint8_t direction, uint16_t address)
{
  (void)address;
  if (hi2c->Instance != I2C1)
  {
    return;
  }

  rx_pending = 0U;
  transfer_started_ms = HAL_GetTick();
  transfer_active = 1U;
  HAL_StatusTypeDef status;
  if (direction == I2C_DIRECTION_TRANSMIT)
  {
    status = HAL_I2C_Slave_Seq_Receive_IT(hi2c, &rx_byte, 1U,
                                       I2C_FIRST_AND_LAST_FRAME);
  }
  else
  {
    /* Unexpected reads release SCL but do not refresh the LED watchdog. */
    status = HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &tx_unused, 1U,
                                        I2C_FIRST_AND_LAST_FRAME);
  }
  if (status != HAL_OK)
  {
    RequestRestart();
  }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    rx_pending = rx_byte <= 1U;
  }
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance != I2C1)
  {
    return;
  }

  /* Commit only a completed write followed by STOP, without a bus error. */
  if (rx_pending && hi2c->ErrorCode == HAL_I2C_ERROR_NONE)
  {
    led_requested = rx_byte;
    last_valid_ms = HAL_GetTick();
    have_valid_state = 1U;
  }
  rx_pending = 0U;
  transfer_active = 0U;
  if (HAL_I2C_EnableListen_IT(hi2c) != HAL_OK)
  {
    RequestRestart();
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    RequestRestart();
  }
}
