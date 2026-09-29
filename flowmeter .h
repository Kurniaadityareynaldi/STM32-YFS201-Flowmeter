/**
  ******************************************************************************
  * @file    flowmeter.h
  * @brief   YF-S201 water flow sensor library for STM32 (HAL, EXTI)
  ******************************************************************************
  * YF-S201 formulas:
  *   F (Hz) = 7.5 * Q (L/min)   ->   Q = F / 7.5
  *   1 liter = 7.5 * 60 = 450 pulses
  ******************************************************************************
  */

#ifndef FLOWMETER_H
#define FLOWMETER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* Default YF-S201 calibration factor (Hz per L/min) */
#define FLOWMETER_DEFAULT_CALIBRATION   7.5f
/* Default calculation interval (ms) */
#define FLOWMETER_DEFAULT_INTERVAL_MS   1000U

typedef struct
{
  /* Configuration */
  GPIO_TypeDef *port;             /* Sensor port, e.g. GPIOA             */
  uint16_t      pin;              /* Sensor pin, e.g. GPIO_PIN_0         */
  IRQn_Type     irq;              /* EXTI IRQ, e.g. EXTI0_IRQn           */
  float         calibration;      /* Hz per L/min (YF-S201 = 7.5)        */
  uint32_t      sample_interval_ms;

  /* Data */
  volatile uint32_t pulse_count;  /* Pulses in current interval (ISR)    */
  uint32_t      total_pulses;     /* Accumulated pulses since start/reset*/
  float         flow_rate_lpm;    /* Flow rate (L/min)                   */
  float         total_volume_l;   /* Total volume (Liters)               */

  /* Internal */
  uint32_t      last_tick;
  uint8_t       running;
} Flowmeter_HandleTypeDef;

/* Initialize the handle (GPIO & EXTI are still configured via CubeMX / MX_GPIO_Init) */
void    Flowmeter_Init(Flowmeter_HandleTypeDef *hflow, GPIO_TypeDef *port,
                       uint16_t pin, IRQn_Type irq,
                       float calibration, uint32_t sample_interval_ms);

void    Flowmeter_Start(Flowmeter_HandleTypeDef *hflow);
void    Flowmeter_Stop(Flowmeter_HandleTypeDef *hflow);

/* Call inside HAL_GPIO_EXTI_Callback() */
void    Flowmeter_PulseCallback(Flowmeter_HandleTypeDef *hflow, uint16_t GPIO_Pin);

/* Call periodically in while(1). Returns 1 when a new result is available. */
uint8_t Flowmeter_Update(Flowmeter_HandleTypeDef *hflow);

float    Flowmeter_GetFlowRate(const Flowmeter_HandleTypeDef *hflow);    /* L/min  */
float    Flowmeter_GetTotalVolume(const Flowmeter_HandleTypeDef *hflow); /* Liters */
uint32_t Flowmeter_GetTotalPulses(const Flowmeter_HandleTypeDef *hflow);
void     Flowmeter_ResetTotal(Flowmeter_HandleTypeDef *hflow);

#ifdef __cplusplus
}
#endif

#endif /* FLOWMETER_H */
