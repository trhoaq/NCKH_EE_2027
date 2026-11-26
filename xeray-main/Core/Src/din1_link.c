#include "din1_link.h"
#include "main.h"

#define PWM_ADDRESS_7BIT       0x30U
#define I2C_HALF_PERIOD_US     5U
#define I2C_TRANSACTION_MS     25U
#define INPUT_SAMPLE_MS        1U
#define INPUT_DEBOUNCE_MS      20U
#define STATE_REFRESH_MS       100U

static uint32_t cycles_per_us;
static uint32_t last_sample_ms;
static uint32_t candidate_since_ms;
static uint32_t last_attempt_ms;
static uint32_t last_success_ms;
static uint8_t candidate_state;
static uint8_t stable_state;
static uint8_t input_ready;
static uint8_t state_pending;
static uint8_t retry_pending;

static void DelayUs(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = us * cycles_per_us;

  while ((uint32_t)(DWT->CYCCNT - start) < cycles)
  {
  }
}

static void SDA(GPIO_PinState state)
{
  HAL_GPIO_WritePin(PWM_SDA_GPIO_Port, PWM_SDA_Pin, state);
}

static void SCL(GPIO_PinState state)
{
  HAL_GPIO_WritePin(PWM_SCL_GPIO_Port, PWM_SCL_Pin, state);
}

static uint8_t SDAIsHigh(void)
{
  return HAL_GPIO_ReadPin(PWM_SDA_GPIO_Port, PWM_SDA_Pin) == GPIO_PIN_SET;
}

static uint8_t Expired(uint32_t started_ms)
{
  return (uint32_t)(HAL_GetTick() - started_ms) >= I2C_TRANSACTION_MS;
}

/* SET releases an open-drain pin; always read the actual SCL for stretching. */
static HAL_StatusTypeDef ReleaseSCL(uint32_t started_ms)
{
  SCL(GPIO_PIN_SET);
  while (HAL_GPIO_ReadPin(PWM_SCL_GPIO_Port, PWM_SCL_Pin) == GPIO_PIN_RESET)
  {
    if (Expired(started_ms))
    {
      return HAL_TIMEOUT;
    }
  }
  return Expired(started_ms) ? HAL_TIMEOUT : HAL_OK;
}

static void ReleaseBus(void)
{
  SDA(GPIO_PIN_SET);
  SCL(GPIO_PIN_SET);
}

static HAL_StatusTypeDef Stop(uint32_t started_ms)
{
  SCL(GPIO_PIN_RESET);
  SDA(GPIO_PIN_RESET);
  DelayUs(I2C_HALF_PERIOD_US);
  if (ReleaseSCL(started_ms) != HAL_OK)
  {
    ReleaseBus();
    return HAL_TIMEOUT;
  }
  DelayUs(I2C_HALF_PERIOD_US);
  SDA(GPIO_PIN_SET);
  DelayUs(I2C_HALF_PERIOD_US);
  return SDAIsHigh() ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef Start(uint32_t started_ms)
{
  SDA(GPIO_PIN_SET);
  if (ReleaseSCL(started_ms) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  DelayUs(I2C_HALF_PERIOD_US);

  /* Single-master bus: finish a byte left incomplete by a main-board reset. */
  if (!SDAIsHigh())
  {
    for (uint32_t pulse = 0U; pulse < 9U && !SDAIsHigh(); ++pulse)
    {
      SCL(GPIO_PIN_RESET);
      DelayUs(I2C_HALF_PERIOD_US);
      if (ReleaseSCL(started_ms) != HAL_OK)
      {
        return HAL_TIMEOUT;
      }
      DelayUs(I2C_HALF_PERIOD_US);
    }
    if (Stop(started_ms) != HAL_OK)
    {
      return HAL_ERROR;
    }
    DelayUs(I2C_HALF_PERIOD_US);
  }

  if (Expired(started_ms))
  {
    return HAL_TIMEOUT;
  }
  SDA(GPIO_PIN_RESET);
  DelayUs(I2C_HALF_PERIOD_US);
  SCL(GPIO_PIN_RESET);
  return HAL_OK;
}

static HAL_StatusTypeDef WriteByte(uint8_t value, uint32_t started_ms)
{
  for (uint32_t bit = 0U; bit < 8U; ++bit)
  {
    SDA((value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    DelayUs(I2C_HALF_PERIOD_US);
    if (ReleaseSCL(started_ms) != HAL_OK)
    {
      return HAL_TIMEOUT;
    }
    DelayUs(I2C_HALF_PERIOD_US);
    SCL(GPIO_PIN_RESET);
    value <<= 1;
  }

  SDA(GPIO_PIN_SET);
  DelayUs(I2C_HALF_PERIOD_US);
  if (ReleaseSCL(started_ms) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }
  DelayUs(I2C_HALF_PERIOD_US);
  uint8_t acknowledged = !SDAIsHigh();
  SCL(GPIO_PIN_RESET);
  return acknowledged ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef SendState(uint8_t state)
{
  uint32_t started_ms = HAL_GetTick();
  HAL_StatusTypeDef status = Start(started_ms);

  if (status == HAL_OK)
  {
    status = WriteByte((uint8_t)(PWM_ADDRESS_7BIT << 1), started_ms);
  }
  if (status == HAL_OK)
  {
    status = WriteByte(state, started_ms);
  }

  if (Expired(started_ms))
  {
    ReleaseBus();
    return HAL_TIMEOUT;
  }
  HAL_StatusTypeDef stop_status = Stop(started_ms);
  return status == HAL_OK ? stop_status : status;
}

static uint8_t ReadInput(void)
{
  /* HD8 IN_1 -> PC817 -> PE0: an energized input pulls PE0 low. */
  return HAL_GPIO_ReadPin(DIN1_GPIO_Port, DIN1_Pin) == GPIO_PIN_RESET;
}

void DIN1_Link_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
  ReleaseBus();

  last_sample_ms = HAL_GetTick();
  candidate_since_ms = last_sample_ms;
  candidate_state = ReadInput();
  stable_state = 0U;
  input_ready = 0U;
  state_pending = 0U;
  retry_pending = 0U;
  last_attempt_ms = last_sample_ms;
  last_success_ms = last_sample_ms;
}

void DIN1_Link_Poll(void)
{
  uint32_t now = HAL_GetTick();

  if ((uint32_t)(now - last_sample_ms) >= INPUT_SAMPLE_MS)
  {
    uint8_t sample = ReadInput();
    last_sample_ms = now;
    if (sample != candidate_state)
    {
      candidate_state = sample;
      candidate_since_ms = now;
    }
    if ((uint32_t)(now - candidate_since_ms) >= INPUT_DEBOUNCE_MS &&
        (!input_ready || stable_state != candidate_state))
    {
      stable_state = candidate_state;
      input_ready = 1U;
      state_pending = 1U;
    }
  }

  if (!input_ready ||
      (retry_pending && (uint32_t)(now - last_attempt_ms) < STATE_REFRESH_MS))
  {
    return;
  }
  if (state_pending || retry_pending ||
      (uint32_t)(now - last_success_ms) >= STATE_REFRESH_MS)
  {
    HAL_StatusTypeDef status = SendState(stable_state);
    last_attempt_ms = HAL_GetTick();
    if (status == HAL_OK)
    {
      last_success_ms = last_attempt_ms;
      state_pending = 0U;
      retry_pending = 0U;
    }
    else
    {
      retry_pending = 1U;
    }
  }
}
