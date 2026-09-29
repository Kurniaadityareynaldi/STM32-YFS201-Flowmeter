/**
  ******************************************************************************
  * @file    flowmeter.c
  * @brief   YF-S201 water flow sensor library for STM32 (HAL, EXTI)
  ******************************************************************************
  */

#include "flowmeter.h"

void Flowmeter_Init(Flowmeter_HandleTypeDef *hflow, GPIO_TypeDef *port,
                    uint16_t pin, IRQn_Type irq,
                    float calibration, uint32_t sample_interval_ms)
{
  hflow->port = port;
  hflow->pin  = pin;
  hflow->irq  = irq;
  hflow->calibration        = (calibration > 0.0f) ? calibration : FLOWMETER_DEFAULT_CALIBRATION;
  hflow->sample_interval_ms = (sample_interval_ms > 0U) ? sample_interval_ms : FLOWMETER_DEFAULT_INTERVAL_MS;

  hflow->pulse_count    = 0;
  hflow->total_pulses   = 0;
  hflow->flow_rate_lpm  = 0.0f;
  hflow->total_volume_l = 0.0f;
  hflow->last_tick      = 0;
  hflow->running        = 0;
}

void Flowmeter_Start(Flowmeter_HandleTypeDef *hflow)
{
  hflow->pulse_count   = 0;
  hflow->flow_rate_lpm = 0.0f;
  hflow->last_tick     = HAL_GetTick();
  hflow->running       = 1;

  __HAL_GPIO_EXTI_CLEAR_IT(hflow->pin);
  HAL_NVIC_ClearPendingIRQ(hflow->irq);
  HAL_NVIC_EnableIRQ(hflow->irq);
}

void Flowmeter_Stop(Flowmeter_HandleTypeDef *hflow)
{
  HAL_NVIC_DisableIRQ(hflow->irq);
  hflow->running       = 0;
  hflow->flow_rate_lpm = 0.0f;
}

void Flowmeter_PulseCallback(Flowmeter_HandleTypeDef *hflow, uint16_t GPIO_Pin)
{
  if (hflow->running && (GPIO_Pin == hflow->pin))
  {
    hflow->pulse_count++;
  }
}

uint8_t Flowmeter_Update(Flowmeter_HandleTypeDef *hflow)
{
  if (!hflow->running)
  {
    return 0;
  }

  uint32_t now     = HAL_GetTick();
  uint32_t elapsed = now - hflow->last_tick;   /* safe against tick overflow */

  if (elapsed < hflow->sample_interval_ms)
  {
    return 0;
  }

  /* Read and reset the counter atomically */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint32_t pulses = hflow->pulse_count;
  hflow->pulse_count = 0;
  if (!primask)
  {
    __enable_irq();
  }

  hflow->last_tick = now;

  /* Frequency (Hz) -> Flow rate (L/min): Q = F / 7.5 */
  float frequency_hz = ((float)pulses * 1000.0f) / (float)elapsed;
  hflow->flow_rate_lpm = frequency_hz / hflow->calibration;

  /* Volume: pulses / (calibration * 60) Liters (YF-S201: 450 pulses = 1 L) */
  hflow->total_pulses   += pulses;
  hflow->total_volume_l += (float)pulses / (hflow->calibration * 60.0f);

  return 1;
}

float Flowmeter_GetFlowRate(const Flowmeter_HandleTypeDef *hflow)
{
  return hflow->flow_rate_lpm;
}

float Flowmeter_GetTotalVolume(const Flowmeter_HandleTypeDef *hflow)
{
  return hflow->total_volume_l;
}

uint32_t Flowmeter_GetTotalPulses(const Flowmeter_HandleTypeDef *hflow)
{
  return hflow->total_pulses;
}

void Flowmeter_ResetTotal(Flowmeter_HandleTypeDef *hflow)
{
  hflow->total_pulses   = 0;
  hflow->total_volume_l = 0.0f;
}
