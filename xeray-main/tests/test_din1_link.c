#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "../Core/Src/din1_link.c"

GPIO_TypeDef mock_a, mock_e;
CoreDebug_Type mock_debug;
static DWT_Type mock_dwt;
static uint32_t tick;
static uint8_t input_active, sda, scl, active, bit_count, bits, ack_phase;
static uint8_t nack_byte, scl_stuck, sda_stuck, release_after_pulses;
static unsigned stretch_reads;
static unsigned recovery_pulses, starts, stops, byte_count;
static uint8_t bytes[128];

DWT_Type *MockDWT(void)
{
  mock_dwt.CYCCNT += 168U;
  return &mock_dwt;
}

uint32_t HAL_GetTick(void)
{
  if (scl_stuck) ++tick;
  return tick;
}

uint32_t HAL_RCC_GetHCLKFreq(void) { return 168000000U; }

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  if (port == &mock_e) return input_active ? GPIO_PIN_RESET : GPIO_PIN_SET;
  if (pin == PWM_SCL_Pin)
  {
    if (stretch_reads) { --stretch_reads; return GPIO_PIN_RESET; }
    return scl && !scl_stuck ? GPIO_PIN_SET : GPIO_PIN_RESET;
  }
  if (sda_stuck) return GPIO_PIN_RESET;
  if (ack_phase && active && (nack_byte == 0U || byte_count != nack_byte))
    return GPIO_PIN_RESET;
  return sda ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState value)
{
  assert(port == &mock_a);
  if (pin == PWM_SDA_Pin)
  {
    if (scl && !scl_stuck && sda != (uint8_t)value)
    {
      if (value == GPIO_PIN_RESET)
      {
        active = 1U;
        bit_count = bits = ack_phase = 0U;
        ++starts;
      }
      else
      {
        active = 0U;
        ++stops;
      }
    }
    sda = (uint8_t)value;
  }
  else
  {
    assert(pin == PWM_SCL_Pin);
    if (!scl && value == GPIO_PIN_SET && !scl_stuck)
    {
      if (sda_stuck && sda)
      {
        ++recovery_pulses;
        if (release_after_pulses && recovery_pulses >= release_after_pulses)
          sda_stuck = 0U;
      }
      if (active)
      {
        if (bit_count == 8U)
        {
          assert(byte_count < sizeof(bytes));
          bytes[byte_count++] = bits;
          bit_count = bits = 0U;
          ack_phase = 1U;
        }
        else
        {
          bits = (uint8_t)((bits << 1) | sda);
          ++bit_count;
          ack_phase = 0U;
        }
      }
    }
    if (value == GPIO_PIN_RESET) ack_phase = 0U;
    scl = (uint8_t)value;
  }
}

static void Reset(uint32_t now)
{
  tick = now;
  input_active = active = bit_count = bits = ack_phase = 0U;
  nack_byte = scl_stuck = sda_stuck = release_after_pulses = 0U;
  sda = scl = 1U;
  starts = stops = byte_count = recovery_pulses = 0U;
  stretch_reads = 0U;
  memset(bytes, 0, sizeof(bytes));
  DIN1_Link_Init();
}

static void PollFor(unsigned ms)
{
  while (ms--) { ++tick; DIN1_Link_Poll(); }
}

int main(void)
{
  Reset(0);
  PollFor(19);
  assert(starts == 0);
  PollFor(1);
  assert(starts == 1 && stops == 1 && byte_count == 2);
  assert(bytes[0] == 0x60 && bytes[1] == 0);
  PollFor(99);
  assert(starts == 1);
  PollFor(1);
  assert(starts == 2 && bytes[3] == 0);

  input_active = 1;
  PollFor(10);
  input_active = 0;
  PollFor(21);
  assert(starts == 2); /* Short pulse ignored. */
  input_active = 1;
  PollFor(20);
  assert(starts == 2);
  PollFor(1);
  assert(starts == 3 && bytes[5] == 1);
  input_active = 0;
  PollFor(21);
  assert(starts == 4 && bytes[7] == 0);

  Reset(0);
  input_active = 1;
  PollFor(21);
  assert(bytes[1] == 1); /* Energized at startup. */

  Reset(0);
  nack_byte = 1;
  PollFor(20);
  assert(starts == 1 && stops == 1 && byte_count == 1);
  assert(retry_pending);
  PollFor(99);
  assert(starts == 1);
  nack_byte = 0;
  PollFor(1);
  assert(starts == 2 && !retry_pending && bytes[2] == 0);

  Reset(0);
  nack_byte = 2;
  assert(SendState(1) == HAL_ERROR);
  assert(stops == 1 && sda && scl);

  Reset(0);
  scl_stuck = 1;
  uint32_t before = tick;
  assert(SendState(1) == HAL_TIMEOUT);
  assert((uint32_t)(tick - before) <= I2C_TRANSACTION_MS + 3U);
  assert(sda && scl);

  Reset(0);
  stretch_reads = 20U;
  assert(SendState(1) == HAL_OK);
  assert(stretch_reads == 0 && bytes[0] == 0x60 && bytes[1] == 1);

  Reset(0);
  sda_stuck = 1;
  release_after_pulses = 9;
  assert(SendState(1) == HAL_OK);
  assert(recovery_pulses == 9 && bytes[0] == 0x60 && bytes[1] == 1);

  Reset(0);
  sda_stuck = 1;
  assert(SendState(1) == HAL_ERROR);
  assert(recovery_pulses == 9 && starts == 0 && sda && scl);

  Reset(UINT32_MAX - 10U);
  input_active = 1;
  PollFor(21);
  assert(bytes[1] == 1 && !retry_pending);
  PollFor(100);
  assert(starts == 2);
  puts("PASS main: debounce, on/off, heartbeat, startup, address/data NACK, clock stretching, SCL timeout, SDA recovery, tick wrap");
  return 0;
}
