/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include "usart.h"
#include "stdio.h"
#include "string.h"
void uart1_send_str(char *data);

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */
  RTC_DateTypeDef sDate = {0};
  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = 32767;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
	//The RTC frequency reaches 1 Hz (32,768 / (32,767 + 1) = 1 Hz).
		
	uart1_send_str("MX_RTC_Init started...\r\n");
	
	////: Enable access to backup registers (to save history)
  HAL_PWR_EnableBkUpAccess(); 
	//////setting time and date if the below line is uncommentd
//	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x0);
	
		uint32_t backup_value = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    char buf[100];
    sprintf(buf, "Backup register value: 0x%04X\r\n", backup_value);
    uart1_send_str(buf);
	
    if (backup_value != 0x32F2) {
        uart1_send_str("Setting initial date and time...\r\n");
        sTime.Hours = 9;
        sTime.Minutes = 49;
        sTime.Seconds = 15;
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }

        sDate.WeekDay = RTC_WEEKDAY_MONDAY;
        sDate.Year = 25;
        sDate.Month = RTC_MONTH_SEPTEMBER;
        sDate.Date = 8;
        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }

        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);

        // The values are stored in backup registers (DR3 to DR6).
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, sDate.Year);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, sDate.Month);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, sDate.Date);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, sDate.WeekDay);
        uart1_send_str("Initial date and time set successfully\r\n");
    } else {
			/*
			Retrieving the date:

			If RTC_BKP_DR1 has the value 0x32F2 (i.e. RTC is already set):

			The date is read from the backup registers (DR3 to DR6).
			The date is set in the RTC with HAL_RTC_SetDate to be retained after reset.
			*/
			
			
        uart1_send_str("Backup register already set (0x32F2), keeping current RTC time\r\n");

        // ?????? ????? ?? ?????? ???? ? ????? ??
        uart1_send_str("Restoring date from backup registers...\r\n");
        sDate.Year = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
        sDate.Month = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
        sDate.Date = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);
        
			// ?????? ?????? WeekDay
				sDate.WeekDay = calculate_weekday(sDate.Date, sDate.Month, 2000 + sDate.Year);
			  sDate.WeekDay = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);
			
			
        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }

//      uart1_send_str("Backup register already set (0x32F2), keeping current RTC time\r\n");

			// ????? ????? ? WeekDay ?? ?????? ?? ???
			// ??? ?????? ?? ????? ?????? SetDate ???
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
//  sTime.Hours = 0x0;
//  sTime.Minutes = 0x0;
//  sTime.Seconds = 0x0;

//  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  DateToUpdate.WeekDay = RTC_WEEKDAY_SUNDAY;
//  DateToUpdate.Month = RTC_MONTH_AUGUST;
//  DateToUpdate.Date = 0x17;
//  DateToUpdate.Year = 0x25;

//  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
//  {
//    Error_Handler();
//  }
  /* USER CODE BEGIN RTC_Init 2 */
	/* ??? ???? ? ????? ??? ?? ???????? ????? */
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    sprintf(buf, "After init: %02d/%02d/20%02d %02d:%02d:%02d\r\n",
            sDate.Date, sDate.Month, sDate.Year,
            sTime.Hours, sTime.Minutes, sTime.Seconds);
    uart1_send_str(buf);
	
	
	// Enable second interrupt:
	/*

	HAL_RTCEx_SetSecond_IT: Enables the RTC interrupt so that the RTC_IRQHandler function is called every second.
	If an error occurs, it goes to Error_Handler.
	*/
	
//	if (HAL_RTCEx_SetSecond_IT(&hrtc) != HAL_OK) {
//					Error_Handler();
//			}
	
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_IRQn);
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint8_t calculate_weekday(uint8_t day, uint8_t month, uint16_t year) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int k = year % 100;
    int j = year / 100;
    int h = (day + 13 * (month + 1) / 5 + k + k/4 + j/4 + 5*j) % 7;

    uint8_t weekday = ((h + 5) % 7) + 1;
    return weekday;
}

/* USER CODE END 1 */
