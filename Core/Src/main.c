/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "iwdg.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stm32f1xx_hal.h"  // ?? ??? MCU ????

extern RTC_HandleTypeDef hrtc; // ????? ????? hrtc ?? rtc.c
volatile uint8_t alarm_flag = 0; // ???? ?????
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2; // UART2 ???? SIM800



RTC_DateTypeDef sDate= {0};
RTC_TimeTypeDef sTime = {0};

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint16_t rx_index = 0;
char uart_buffer[256];
uint8_t alarm_duration = 2;

uint8_t led1_end_hour;
uint8_t led1_end_minute; // ????? ??
uint8_t led2_end_hour;
uint8_t led2_end_minute;
uint8_t led3_end_hour;
uint8_t led3_end_minute;



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// ?????? (?? ???? ???? ????? ????????? ???? ????? ???)
#define LED1_GPIO GPIOB
#define LED1_PIN  GPIO_PIN_12
#define LED2_GPIO GPIOB
#define LED2_PIN  GPIO_PIN_13   // ????? ??? ??? ?? ????? ??
#define LED3_GPIO GPIOB
#define LED3_PIN  GPIO_PIN_14   // ????? ??? ??? ?? ????? ??


// ????? ? ??????? LED??
volatile uint8_t led1_on = 0, led2_on = 0, led3_on = 0;
uint8_t led1_hour, led1_minute = 0, led1_duration = 0;
uint8_t led2_hour, led2_minute = 0, led2_duration = 0;
uint8_t led3_hour, led3_minute = 0, led3_duration = 0;



uint32_t last_door_event_time = 0;   // ???? ????? ?????? ??
uint8_t door_event_stage = 0; // 0=????? ???? ????? 1=???? ???????? 2=???? ?????
uint8_t door_sensor_enabled = 0;  // 1=????? 0=??? ????
uint8_t alarms_enabled = 0;  // 1=????? 0=??? ????


volatile uint8_t sms_received_flag = 0;
volatile uint8_t sms_index = 0;
// ???? ????? ???-blocking ????? ?????
volatile uint8_t status_sms_pending = 0;      // flag ???? ????? ????? ????? Status ????? ????? ???
char status_sms_msg[160];      // ??? ????? Status

// 1 ???? ????? ???? LED1 ???? 2 ???? ???? LED2
volatile uint8_t current_alarm_led = 1;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

void Set_Alarm_Daily(uint8_t hour, uint8_t minute, uint8_t second);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void uart1_send_str(char *data);


void RTC_IRQHandler(void);

void SystemClock_Config(void);
static void MX_UART2_Init(void);
void uart1_send_str(char *data);
void sim800_send_sms(const char *phone_number, const char *message);
void sim800_send_sms1(const char *phone_number, const char *message);
void Error_Handler(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t alarm_triggered = 0;
volatile uint8_t led_on = 0;

//volatile uint8_t led2_on = 0;
volatile uint8_t alarm_triggered_led2 = 0;

//void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
//{
//  alarm_triggered = 1; // ??? ????? ????
//}


//void Set_Alarm_Daily(uint8_t hour, uint8_t minute, uint8_t second)
//{
//    RTC_AlarmTypeDef sAlarm = {0};
//    sAlarm.AlarmTime.Hours = hour;
//    sAlarm.AlarmTime.Minutes = minute;
//    sAlarm.AlarmTime.Seconds = second;
//    sAlarm.Alarm = RTC_ALARM_A;
//    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
//    {
//        Error_Handler();
//    }
//}
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//    static char buffer[32];
//    static uint8_t idx = 0;

//    if(huart->Instance == USART2) {  // ??? UART ??? USART2 ???
//        buffer[idx++] = uart_buffer[0];

//        // ????? ?? +CMTI
//        if(idx >= 5 && strstr(buffer, "+CMTI: \"SM\",")) {
//            char *p = strchr(buffer, ',');
//            if(p) sms_index = atoi(p+1);
//            sms_received_flag = 1;
//            idx = 0; // ???? buffer
//        }

//        // ???? ???? ???? ?????? ?? ????
//        HAL_UART_Receive_IT(&huart2, (uint8_t*)uart_buffer, 1);
//    }
//}

void uart1_send_str(char *data)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), 100);
}


// ????? ???? ???? ?? UART ?????? ?? ???????? ??????????
uint8_t sim800_wait_for(const char *expected, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    char buffer[128];
    uint16_t index = 0;
    uint8_t byte;

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart2, &byte, 1, 10) == HAL_OK)
        {
            if (index < sizeof(buffer) - 1)
                buffer[index++] = byte;
            buffer[index] = '\0';

            if (strstr(buffer, expected) != NULL)
                return 1;  // ???? ???? ??
        }
    }
    return 0; // ???????? ??
}

void sim800_receive_response(char *buffer, uint16_t buffer_size, uint32_t timeout)
{
    uint16_t index = 0;
    uint32_t start_time = HAL_GetTick();
    uint8_t byte;

    while ((HAL_GetTick() - start_time) < timeout)
    {
        if (HAL_UART_Receive(&huart2, &byte, 1, 10) == HAL_OK)
        {
            if (index < buffer_size - 1)
            {
                buffer[index++] = byte;
            }
        }
    }
    buffer[index] = '\0';

    // ????? ???? ??? UART1
    char debug_buf[256];
//    sprintf(debug_buf, "SIM800 Response: %s\r\n", buffer);
//    uart1_send_str(debug_buf);
		  snprintf(debug_buf, sizeof(debug_buf), "SIM800 Response: %s\r\n", buffer);
			uart1_send_str(debug_buf);

    // ??? ???? ERROR ???? ????? ?? ???? ??
    if (strstr(buffer, "ERROR") != NULL)
    {
        uart1_send_str("SIM800 ERROR detected, resetting module...\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CFUN=1,1\r\n", strlen("AT+CFUN=1,1\r\n"), 100);
        HAL_Delay(10000); // ??? ???? ????
    }
}
void sim800_cleanup_sms(void) {
    char buffer[200];
    char cmd[20];
    int index = 1;

    while (1) {
        // ?????? ????? ?? ????? index
        sprintf(cmd, "AT+CMGR=%d\r\n", index);
        HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 100);
        sim800_receive_response(buffer, sizeof(buffer), 2000);

        if (strstr(buffer, "+CMGR:") == NULL) {
            // ???? ?????? ???? ?????
            break;
        }

        // ??? ???? ?????
        sprintf(cmd, "AT+CMGD=%d\r\n", index);
        HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 100);
        HAL_Delay(200); // ??? ??? ???? ????? ??????
        index++;
    }

    uart1_send_str("All old SMS cleared.\r\n");
}
//void sim800_cleanup_sms(void)
//{
//    char cmd[100];
//	  sprintf(cmd, "AT+CMGD=1,4");
//        HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 100);
//   
//    // 1,4 ???? ????? ??????? ??? ???
//}

////////////////////////////////////

// ????? ???? ?????? ???? ?? Backup Register
// =======================================================
// ??????? ??????? ?? Backup Register
// =======================================================

//void save_settings_to_backup(void) {
//    HAL_PWR_EnableBkUpAccess();  // ????? ??? ?? ?????
//    
//    uint32_t flags = ((door_sensor_enabled & 0x01) << 0) |
//                     ((alarms_enabled & 0x01) << 1);
//	  HAL_Delay(1);
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6, flags);

//    uint32_t led1_val = ((uint32_t)led1_hour << 16) |
//                        ((uint32_t)led1_minute << 8) |
//                        led1_duration;
//	  HAL_Delay(1);
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR7, led1_val);

//    uint32_t led2_val = ((uint32_t)led2_hour << 16) |
//                        ((uint32_t)led2_minute << 8) |
//                        led2_duration;
//	  HAL_Delay(1);
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8, led2_val);

//    uint32_t led3_val = ((uint32_t)led3_hour << 16) |
//                        ((uint32_t)led3_minute << 8) |
//                        led3_duration;
//		HAL_Delay(1);
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR9, led3_val);
//}
//void save_settings_to_backup(void) {
//    HAL_PWR_EnableBkUpAccess();  // ???? ???? ?????? ?? Backup Registers
//    
//    // ????? ????? ????? ? ?????
//    uint32_t flags = ((door_sensor_enabled & 0x01) << 0) |
//                     ((alarms_enabled & 0x01) << 1);
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6, flags);

//    HAL_Delay(1);

//    // ????? ???? ??????? LED1 ?? DR7
//    uint32_t led1_val = ((uint32_t)led1_hour << 16) |
//                        ((uint32_t)led1_minute << 8) |
//                        led1_duration;
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR7, led1_val);

//    HAL_Delay(1);

//    // ????? ??????? ??? ???? LED1 ?? DR10 (???? ???)
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR10, (uint32_t)led1_hour);

//    HAL_Delay(1);

//    // ????? ??????? LED2
//    uint32_t led2_val = ((uint32_t)led2_hour << 16) |
//                        ((uint32_t)led2_minute << 8) |
//                        led2_duration;
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8, led2_val);

//    HAL_Delay(1);

//    // ????? ??????? LED3
//    uint32_t led3_val = ((uint32_t)led3_hour << 16) |
//                        ((uint32_t)led3_minute << 8) |
//                        led3_duration;
//    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR9, led3_val);

//    HAL_Delay(1);
//}
// ********* Save Settings *********
void save_settings_to_backup(void) {
    HAL_PWR_EnableBkUpAccess();

    uint8_t b0 = (uint8_t)((door_sensor_enabled & 0x01) | ((alarms_enabled & 0x01) << 1)); // flags
    uint8_t b1 = (uint8_t)led1_hour;
    uint8_t b2 = (uint8_t)led1_minute;
    uint8_t b3 = (uint8_t)led1_duration;
    uint8_t b4 = (uint8_t)led2_hour;
    uint8_t b5 = (uint8_t)led2_minute;
    uint8_t b6 = (uint8_t)led2_duration;
    uint8_t b7 = (uint8_t)led3_hour;
    uint8_t b8 = (uint8_t)led3_minute;
    uint8_t b9 = (uint8_t)led3_duration;

    // ????????? ?? ?? ???? ?? ?? ??????
    uint16_t dr6  = ((uint16_t)b1 << 8) | (uint16_t)b0; // flags + led1_hour
    uint16_t dr7  = ((uint16_t)b3 << 8) | (uint16_t)b2; // led1_minute + led1_duration
    uint16_t dr8  = ((uint16_t)b5 << 8) | (uint16_t)b4; // led2_hour + led2_minute
    uint16_t dr9  = ((uint16_t)b7 << 8) | (uint16_t)b6; // led2_duration + led3_hour
    uint16_t dr10 = ((uint16_t)b9 << 8) | (uint16_t)b8; // led3_minute + led3_duration

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6,  dr6);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR7,  dr7);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8,  dr8);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR9,  dr9);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR10, dr10);

    // Marker ???? ??????? ?? ????? ???? ????
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
}



// =======================================================
// ???????? ??????? ?? Backup Register
// =======================================================
//void load_settings_from_backup(void) {
//    HAL_PWR_EnableBkUpAccess(); // ?????? ?? Backup Registers

//    // ??????? ????? ??????? ? ????????
//    uint32_t flags = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR7);
//    door_sensor_enabled = (flags >> 0) & 0x01;
//    alarms_enabled = (flags >> 1) & 0x01;

//    // ??????? LED1
//    uint32_t led1_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR8);
//    led1_hour = (led1_val >> 16) & 0xFF;
//    led1_minute = (led1_val >> 8) & 0xFF;
//    led1_duration = led1_val & 0xFF;

//    // ??????? LED2
//    uint32_t led2_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR9);
//    led2_hour = (led2_val >> 16) & 0xFF;
//    led2_minute = (led2_val >> 8) & 0xFF;
//    led2_duration = led2_val & 0xFF;

//    // ??????? LED3
//    uint32_t led3_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR10);
//    led3_hour = (led3_val >> 16) & 0xFF;
//    led3_minute = (led3_val >> 8) & 0xFF;
//    led3_duration = led3_val & 0xFF;
//}

//void load_settings_from_backup(void) {
//    HAL_PWR_EnableBkUpAccess(); // ?????? ?? Backup domain

//    uint32_t flags = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR6);

//    // ??? Backup Register ???? ?? ????? ?????
//    if (flags == 0xFFFFFFFF || flags == 0x0) {
//        door_sensor_enabled = 0;
//        alarms_enabled = 0;

//        led1_hour = 8; led1_minute = 20; led1_duration = 4;
//        led2_hour = 21; led2_minute = 0; led2_duration = 55;
//        led3_hour = 23; led3_minute = 0; led3_duration = 55;

//        save_settings_to_backup(); // ????? ?????? ???????
//        HAL_Delay(1); // ????? ????? ???? ???????
//        return;
//    }

//    // ??????? ??????
//    door_sensor_enabled = (flags >> 0) & 0x01;
//    alarms_enabled = (flags >> 1) & 0x01;

//    uint32_t led1_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR7);
//    led1_hour = (led1_val >> 16) & 0xFF;
//    led1_minute = (led1_val >> 8) & 0xFF;
//    led1_duration = led1_val & 0xFF;

//    uint32_t led2_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR8);
//    led2_hour = (led2_val >> 16) & 0xFF;
//    led2_minute = (led2_val >> 8) & 0xFF;
//    led2_duration = led2_val & 0xFF;

//    uint32_t led3_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR9);
//    led3_hour = (led3_val >> 16) & 0xFF;
//    led3_minute = (led3_val >> 8) & 0xFF;
//    led3_duration = led3_val & 0xFF;
//}
//void load_settings_from_backup(void) {
//    HAL_PWR_EnableBkUpAccess();

//    uint32_t flags = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR6);

//    // ??? ?????? ???? ??? ? ?????? ???????
//    if (flags == 0xFFFFFFFF || flags == 0x0) {
//        door_sensor_enabled = 0;
//        alarms_enabled = 0;

//        led1_hour = 8; led1_minute = 20; led1_duration = 4;
//        led2_hour = 21; led2_minute = 0; led2_duration = 55;
//        led3_hour = 23; led3_minute = 0; led3_duration = 55;

//        save_settings_to_backup();
//        HAL_Delay(1);
//        return;
//    }

//    // ??????? ??????
//    door_sensor_enabled = (flags >> 0) & 0x01;
//    alarms_enabled = (flags >> 1) & 0x01;

//    // ??????? LED1
//    uint32_t led1_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR7);
//    led1_hour = (led1_val >> 16) & 0xFF;
//    led1_minute = (led1_val >> 8) & 0xFF;
//    led1_duration = led1_val & 0xFF;

//    // ???: ????? ???? ?? ?????? ?? DR10 ????? ? ??? ??
//    uint32_t backup_hour = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR10);
//    char debug_msg[50];
//    sprintf(debug_msg, "LED1 Hour: %u (DR10)\r\n", backup_hour);
//    uart1_send_str(debug_msg);

//    // ??????? LED2
//    uint32_t led2_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR8);
//    led2_hour = (led2_val >> 16) & 0xFF;
//    led2_minute = (led2_val >> 8) & 0xFF;
//    led2_duration = led2_val & 0xFF;

//    // ??????? LED3
//    uint32_t led3_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR9);
//    led3_hour = (led3_val >> 16) & 0xFF;
//    led3_minute = (led3_val >> 8) & 0xFF;
//    led3_duration = led3_val & 0xFF;
//}
// ********* Load Settings *********
void load_settings_from_backup(void) {
    HAL_PWR_EnableBkUpAccess();

    uint32_t marker = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    if (marker != 0x32F2) {
        // ???? ????? ???? => ?????? ???????
        door_sensor_enabled = 0;
        alarms_enabled = 0;

        led1_hour = 8;  led1_minute = 20; led1_duration = 4;
        led2_hour = 21; led2_minute = 0;  led2_duration = 55;
        led3_hour = 23; led3_minute = 0;  led3_duration = 55;

        save_settings_to_backup();
        return;
    }

    uint16_t dr6  = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR6);
    uint16_t dr7  = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR7);
    uint16_t dr8  = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR8);
    uint16_t dr9  = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR9);
    uint16_t dr10 = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR10);

    // ??????? ??????
    uint8_t b0 = dr6 & 0xFF;       // flags
    uint8_t b1 = (dr6 >> 8) & 0xFF; // led1_hour
    uint8_t b2 = dr7 & 0xFF;       // led1_minute
    uint8_t b3 = (dr7 >> 8) & 0xFF; // led1_duration
    uint8_t b4 = dr8 & 0xFF;       // led2_hour
    uint8_t b5 = (dr8 >> 8) & 0xFF; // led2_minute
    uint8_t b6 = dr9 & 0xFF;       // led2_duration
    uint8_t b7 = (dr9 >> 8) & 0xFF; // led3_hour
    uint8_t b8 = dr10 & 0xFF;      // led3_minute
    uint8_t b9 = (dr10 >> 8) & 0xFF; // led3_duration

    // ?????????? ?????? ?? ???????
    door_sensor_enabled = (b0 >> 0) & 0x01;
    alarms_enabled      = (b0 >> 1) & 0x01;

    led1_hour = b1;
    led1_minute = b2;
    led1_duration = b3;

    led2_hour = b4;
    led2_minute = b5;
    led2_duration = b6;

    led3_hour = b7;
    led3_minute = b8;
    led3_duration = b9;
}



///////////////////////////////////////////

void sim800_check_sms(char *sms_content, uint16_t size) {
    char buffer[200];
	  const char *weekdays[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

//	  HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGF=1\r\n", strlen("AT+CMGF=1\r\n"), 100);
//    HAL_Delay(500);
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGR=1\r\n", strlen("AT+CMGR=1\r\n"), 100);
    sim800_receive_response(buffer, sizeof(buffer), 2000);

	///////////////////////////////////////////////////////////////////////////////////////////////////////
  
	///////////////////////////////////////////////////////////////////////////////////////////////////////  
	
	 strncpy(sms_content, buffer, size-1);
   sms_content[size-1]='\0';
	
	
    if (strstr(sms_content, "Water on")) {
        HAL_GPIO_WritePin(LED1_GPIO, LED1_PIN, GPIO_PIN_SET);
        led1_on = 1;
        sim800_send_sms1("+989124661714", "Water turned ON via SMS");
			
			  
    } 
    else if (strstr(sms_content, "Water off")) {
        HAL_GPIO_WritePin(LED1_GPIO, LED1_PIN, GPIO_PIN_RESET);
        led1_on = 0;
        sim800_send_sms1("+989124661714", "Water turned OFF via SMS");
    }
		else if (strstr(sms_content, "Led2 on")) {
        HAL_GPIO_WritePin(LED2_GPIO, LED2_PIN, GPIO_PIN_SET);
        led2_on = 1;
        sim800_send_sms1("+989124661714", "LED2 turned ON via SMS");
    } 
    else if (strstr(sms_content, "Led2 off")) {
        HAL_GPIO_WritePin(LED2_GPIO, LED2_PIN, GPIO_PIN_RESET);
        led2_on = 0;
        sim800_send_sms1("+989124661714", "LED2 turned OFF via SMS");
    }
		else if (strstr(sms_content, "Led3 on")) {
        HAL_GPIO_WritePin(LED3_GPIO, LED3_PIN, GPIO_PIN_SET);
        led3_on = 1;
        sim800_send_sms1("+989124661714", "LED3 turned ON via SMS");
    } 
    else if (strstr(sms_content, "Led3 off")) {
        HAL_GPIO_WritePin(LED3_GPIO, LED3_PIN, GPIO_PIN_RESET);
        led3_on = 0;
        sim800_send_sms1("+989124661714", "LED3 turned OFF via SMS");
    }
    else if (strstr(sms_content, "Status")) {
        RTC_TimeTypeDef sTime;
        RTC_DateTypeDef sDate;
        char msg[160];
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        snprintf(msg, sizeof(msg),
            "%02d:%02d:%02d %.2s %02d/%02d/%02d\n"
				    "Sensors:%s\n"
				    "Alarms:%s\n"
            ">L1:%s A%02d:%02d D%u\n"
				    ">L2:%s A%02d:%02d D%u\n"
            ">L3:%s A%02d:%02d D%u",
            sTime.Hours, sTime.Minutes, sTime.Seconds, weekdays[sDate.WeekDay],
            sDate.Date, sDate.Month, sDate.Year,
				    (door_sensor_enabled ? "ON" : "OFF"),
				    (alarms_enabled ? "ON" : "OFF"),
            (led1_on ? "ON" : "OFF"), led1_hour, led1_minute, led1_duration,
            (led2_on ? "ON" : "OFF"), led2_hour, led2_minute, led2_duration,
				    (led3_on ? "ON" : "OFF"), led3_hour, led3_minute, led3_duration
						);

        status_sms_pending = 1;
        strncpy(status_sms_msg, msg, sizeof(status_sms_msg)-1);
        status_sms_msg[sizeof(status_sms_msg)-1] = '\0';
    }
				else if (strstr(sms_content, "Alarms") != NULL) {
								char *p = strstr(sms_content, "Alarms");
								if (p) {
								p += strlen("Alarms");  // ???? ?? ??? ?? ???? "Alarms"
								
								// ??? ????????? ???? ?????
								while (*p == ' ') p++;

								if (strncasecmp(p, "on", 2) == 0) {   // ??? ??? ?? "Alarms" ???? "on" ???
										alarms_enabled = 1;
										sim800_send_sms1("+989124661714", "Alarms enabled");
									
									  save_settings_to_backup();
								} 
								else if (strncasecmp(p, "off", 3) == 0) { // ??? ??? ?? "Alarms" ???? "off" ???
										alarms_enabled = 0;
										sim800_send_sms1("+989124661714", "Alarms disabled");
									
									  save_settings_to_backup();
								} 
								else {
										sim800_send_sms1("+989124661714", "Invalid Alarms command! Use: Alarms on/off");
								}
						}
				}
				
		else if (strstr(sms_content, "Alarm3") != NULL) {
				int h, m, d;
				char *p = strstr(sms_content, "Alarm3");
				if (p) {
						p += strlen("Alarm3");

						if (sscanf(p, " %d:%d,%d", &h, &m, &d) == 3) {
								if (h < 24 && m < 60 && d > 0) {
//										led3_hour = (uint8_t)h;
//										led3_minute = (uint8_t)m;
//										led3_duration = (uint8_t)d;
									
								  	 led3_hour = h;
										led3_minute = m;
										led3_duration = d;

										// ?????? ???? ????? LED ?? ?? ??? ????? ???? ?? ???? ? ?????
										uint16_t total_minutes = led3_minute + led3_duration;
										led3_end_hour = led3_hour + total_minutes / 60;
										led3_end_minute = total_minutes % 60;
										if (led3_end_hour >= 24) led3_end_hour -= 24;
									  
									  save_settings_to_backup();

										char ack[64];
										snprintf(ack, sizeof(ack),
														 "Alarm LED3 set %02d:%02d, Dur %u mins -> End %02d:%02d",
														 led3_hour, led3_minute, led3_duration, led3_end_hour, led3_end_minute);
										sim800_send_sms1("+989124661714", ack);
										
										
										
								} else {
										sim800_send_sms1("+989124661714", "Invalid Alarm3 values!");
								}
						} else {
								sim800_send_sms1("+989124661714", "Invalid Alarm3 format! Use: Alarm3 HH:MM,D");
						}
				}
		}
		
		else if (strstr(sms_content, "Alarm2") != NULL) {
				int h, m, d;
				char *p = strstr(sms_content, "Alarm2");
				if (p) {
						p += strlen("Alarm2");

						if (sscanf(p, " %d:%d,%d", &h, &m, &d) == 3) {
								if (h < 24 && m < 60 && d > 0) {
//										led2_hour = (uint8_t)h;
//										led2_minute = (uint8_t)m;
//										led2_duration = (uint8_t)d;
									  led2_hour = h;
										led2_minute = m;
										led2_duration = d;

										// ?????? ???? ????? LED ?? ?? ??? ????? ???? ?? ???? ? ?????
										uint16_t total_minutes = led2_minute + led2_duration;
										led2_end_hour = led2_hour + total_minutes / 60;
										led2_end_minute = total_minutes % 60;
										if (led2_end_hour >= 24) led2_end_hour -= 24;
                    
									  save_settings_to_backup();
									
										char ack[64];
										snprintf(ack, sizeof(ack),
														 "Alarm LED2 set %02d:%02d, Dur %u mins -> End %02d:%02d",
														 led2_hour, led2_minute, led2_duration, led2_end_hour, led2_end_minute);
										sim800_send_sms1("+989124661714", ack);
										
										
										
										
								} else {
										sim800_send_sms1("+989124661714", "Invalid Alarm2 values!");
								}
						} else {
								sim800_send_sms1("+989124661714", "Invalid Alarm2 format! Use: Alarm3 HH:MM,D");
						}
				}
		}

		else if (strstr(sms_content, "Alarm1") != NULL) {
				int h, m, d;
				char *p = strstr(sms_content, "Alarm1");
				if (p) {
						p += strlen("Alarm1");

						if (sscanf(p, " %d:%d,%d", &h, &m, &d) == 3) {
								if (h < 24 && m < 60 && d > 0) {
//										led1_hour = (uint8_t)h;
//										led1_minute = (uint8_t)m;
//										led1_duration = (uint8_t)d;
									led1_hour = h;
									led1_minute = m;
									led1_duration = d;

										// ?????? ???? ????? LED ?? ?? ??? ????? ???? ?? ???? ? ?????
										uint16_t total_minutes = led1_minute + led1_duration;
										led1_end_hour = led1_hour + total_minutes / 60;
										led1_end_minute = total_minutes % 60;
										if (led1_end_hour >= 24) led1_end_hour -= 24;

									  save_settings_to_backup();
									
										char ack[64];
										snprintf(ack, sizeof(ack),
														 "Alarm LED1 set %02d:%02d, Dur %u mins -> End %02d:%02d",
														 led1_hour, led1_minute, led1_duration, led1_end_hour, led1_end_minute);
										sim800_send_sms1("+989124661714", ack);
										
										
										
								} else {
										sim800_send_sms1("+989124661714", "Invalid Alarm1 values!");
								}
						} else {
								sim800_send_sms1("+989124661714", "Invalid Alarm1 format! Use: Alarm3 HH:MM,D");
						}
				}
		}

		
    else if (strstr(sms_content, "Reset") != NULL){
        sim800_send_sms1("+989124661714","System will reset now...");
        HAL_Delay(200);
        NVIC_SystemReset();
    }
		else if (strstr(sms_content, "Sensors on") != NULL) {
    door_sensor_enabled = 1;
    sim800_send_sms1("+989124661714", "Door sensor enabled");
			
		save_settings_to_backup();
			
}
		else if (strstr(sms_content, "Sensors off") != NULL) {
				door_sensor_enabled = 0;
				sim800_send_sms1("+989124661714", "Door sensor disabled");
			
			  save_settings_to_backup();
			 
		}


//    strncpy(sms_content, buffer, size-1);
//    sms_content[size-1]='\0';
    HAL_UART_Transmit(&huart2,(uint8_t*)"AT+CMGD=1,4\r\n",strlen("AT+CMGD=1,4\r\n"),100);
    HAL_Delay(500);
}



void sim800_send_sms(const char *phone_number, const char *message)
{
    char buf[128];
    char response[100];

    // ??? ?????? ?? SIM800
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT\r\n", strlen("AT\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
    
    // ????? ???? ???? SMS
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGF=1\r\n", strlen("AT+CMGF=1\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
	
		    // ????? ???? ???? SMS
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CSQ\r\n", strlen("AT+CSQ\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
	
	  HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CSCA?\r\n", strlen("AT+CSCA?\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
	
		HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CPIN?\r\n", strlen("AT+CPIN?\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
    

      snprintf(buf, sizeof(buf), "SMS sent to %s: %s\r\n", phone_number, message);
			uart1_send_str(buf);
}


// ???? ????? SMS ??? ??????????
void sim800_send_sms1(const char *phone_number, const char *message)
{
    char buf[128];

    // ????? 1: AT
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT\r\n", 3, 100);
    HAL_Delay(50);  // ????? ? ??? ??????????

    // ????? 2: SMS Text mode
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGF=1\r\n", 11, 100);
    HAL_Delay(50);

    // ????? 3: ???? ????? SMS
    snprintf(buf, sizeof(buf), "AT+CMGS=\"%s\"\r\n", phone_number);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
    HAL_Delay(50);

    // ????? 4: ??? ????
    HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), 100);
    HAL_Delay(50);

    // ????? 5: CTRL+Z
    HAL_UART_Transmit(&huart2, (uint8_t*)"\x1A", 1, 100);

    // ????? 6: ????? Delay ???? ???? SIM800
    HAL_Delay(500);

    // ????? ???? ??? UART1
    snprintf(buf, sizeof(buf), "SMS sent to %s: %s\r\n", phone_number, message);
    uart1_send_str(buf);
}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
char sending_data_str[60];
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
	uart1_send_str("Program started...\r\n"); // ???? ?????
////////////////////////////////////////////////////////////////////////////////////////	
//  MX_RTC_Init();
	  
		load_settings_from_backup();
		
	 // ???? ???? ????? SMS ???? ??? UART
		HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CNMI=2,2,0,0,0\r\n", strlen("AT+CNMI=2,2,0,0,0\r\n"), 100);
		HAL_Delay(500);

	 
	// ??? ????? ?????? ?? SIM800
    char response[100];
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT\r\n", strlen("AT\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);
    
		HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGF=1\r\n", strlen("AT+CMGF=1\r\n"), 100);
    sim800_receive_response(response, sizeof(response), 1000);

    // ????? ????? ????? ?? ????? ? ????
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    const char *weekdays[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, sDate.Year);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, sDate.Month);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, sDate.Date);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, sDate.WeekDay);
		
		
		char sms_message[100];
    sprintf(sms_message, "Ready: %02d/%02d/20%02d %s %02d:%02d:%02d",
            sDate.Date, sDate.Month, sDate.Year,
            weekdays[sDate.WeekDay],
            sTime.Hours, sTime.Minutes, sTime.Seconds);
//    sim800_send_sms("+989379855298", sms_message); // ??????? ????? ????
	
//  Set_Alarm_Daily(9,45, 00);   // set alarm daily for 8:20:00
		
		   // ***** ?????? ???? ????? ??? LED ?? *****
    uint16_t total_minutes;

    // LED1
    total_minutes = led1_minute + led1_duration;
    led1_end_hour = led1_hour + total_minutes / 60;
    led1_end_minute = total_minutes % 60;
    if (led1_end_hour >= 24) led1_end_hour -= 24;

    // LED2
    total_minutes = led2_minute + led2_duration;
    led2_end_hour = led2_hour + total_minutes / 60;
    led2_end_minute = total_minutes % 60;
    if (led2_end_hour >= 24) led2_end_hour -= 24;

    // LED3
    total_minutes = led3_minute + led3_duration;
    led3_end_hour = led3_hour + total_minutes / 60;
    led3_end_minute = total_minutes % 60;
    if (led3_end_hour >= 24) led3_end_hour -= 24;
    // ***************************************
		
		
  
	char buf[60];

   
		unsigned char previous_second=250;
		
		
		uart1_send_str("Mostafa Ataee\r\n\r\n");
//		sim800_send_sms1("+989124661714", "Micro is Reseted,\n Alarms off,\n Sensors off");

   char sms_content[160];
		
//		HAL_UART_Receive_IT(&huart2, (uint8_t*)uart_buffer, 1);  // ?????? ??????? 1 ????
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
/* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
	
//	///////////////////////////////////////////////////////////////////////////
    // ?????? ???? ? ????? ????
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
		
//		HAL_GPIO_WritePin(LED1_GPIO, LED1_PIN, GPIO_PIN_SET);
//		HAL_Delay(500);
//		HAL_GPIO_WritePin(LED2_GPIO, LED2_PIN, GPIO_PIN_SET);
//		HAL_Delay(500);
//		HAL_GPIO_WritePin(LED3_GPIO, LED3_PIN, GPIO_PIN_SET);
//		HAL_Delay(500);
//    sim800_receive_response(sms_content, sizeof(sms_content), 2000);
//    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET) {
//    HAL_UART_Transmit(&huart2, (uint8_t*)"Pin LOW!\r\n", strlen("Pin LOW!\r\n"), 1000);
//}
//      HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x12345678);
//		HAL_Delay(500);
//		char buf[32];
//uint32_t value = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
//sprintf(buf, "Value: 0x%08X\r\n", value);
//uart1_send_str(buf);
    // ??????? ???????? ??? ?? ??????
//    sim800_cleanup_sms();
    HAL_UART_Transmit(&huart2, (uint8_t*)"AT+CMGF=1\r\n", strlen("AT+CMGF=1\r\n"), 100);
    HAL_Delay(100);
		
		if (status_sms_pending) {
    sim800_send_sms1("+989124661714", status_sms_msg);
    status_sms_pending = 0; // ????????? flag ?? ???? ??
		}
		
		sim800_check_sms(sms_content, sizeof(sms_content));

		
		if (alarms_enabled) {
					// LED1
					if (!led1_on && sTime.Hours == led1_hour && sTime.Minutes == led1_minute) {
							HAL_GPIO_WritePin(LED1_GPIO, LED1_PIN, GPIO_PIN_SET);
							led1_on = 1;
							
					//	  sprintf(sms_message, "LED2 is ON: %02d/%02d/20%02d %s %02d:%02d",
					//						sDate.Date, sDate.Month, sDate.Year,
					//						weekdays[sDate.WeekDay],
					//						sTime.Hours, sTime.Minutes);
							snprintf(sms_message, sizeof(sms_message),
									 "LED1 is ON: %02d/%02d/20%02d %s %02d:%02d",
									 sDate.Date, sDate.Month, sDate.Year,
									 weekdays[sDate.WeekDay],
									 sTime.Hours, sTime.Minutes);
							sim800_send_sms1("+989124661714", sms_message);
						
							led1_end_minute = (led1_minute + led1_duration) % 60;
					}

					if (led1_on && sTime.Hours == led1_end_hour && sTime.Minutes == led1_end_minute) {
							HAL_GPIO_WritePin(LED1_GPIO, LED1_PIN, GPIO_PIN_RESET);
							led1_on = 0;
							sim800_send_sms1("+989124661714", "LED1 is OFF");
					}

					// LED2
					if (!led2_on && sTime.Hours == led2_hour && sTime.Minutes == led2_minute) {
							HAL_GPIO_WritePin(LED2_GPIO, LED2_PIN, GPIO_PIN_SET);
							led2_on = 1;
							
					//	  sprintf(sms_message, "LED2 is ON: %02d/%02d/20%02d %s %02d:%02d",
					//						sDate.Date, sDate.Month, sDate.Year,
					//						weekdays[sDate.WeekDay],
					//						sTime.Hours, sTime.Minutes);
							snprintf(sms_message, sizeof(sms_message),
									 "LED2 is ON: %02d/%02d/20%02d %s %02d:%02d",
									 sDate.Date, sDate.Month, sDate.Year,
									 weekdays[sDate.WeekDay],
									 sTime.Hours, sTime.Minutes);
							sim800_send_sms1("+989124661714", sms_message);
						
							led2_end_minute = (led2_minute + led2_duration) % 60;
					}

					if (led2_on && sTime.Hours == led2_end_hour && sTime.Minutes == led2_end_minute) {
							HAL_GPIO_WritePin(LED2_GPIO, LED2_PIN, GPIO_PIN_RESET);
							led2_on = 0;
							sim800_send_sms1("+989124661714", "LED2 is OFF");
					}

					// LED3
					if (!led3_on && sTime.Hours == led3_hour && sTime.Minutes == led3_minute) {
							HAL_GPIO_WritePin(LED3_GPIO, LED3_PIN, GPIO_PIN_SET);
							led3_on = 1;
						
					//	  sprintf(sms_message, "LED3 is ON: %02d/%02d/20%02d %s %02d:%02d",
					//						sDate.Date, sDate.Month, sDate.Year,
					//						weekdays[sDate.WeekDay],
					//						sTime.Hours, sTime.Minutes);
							snprintf(sms_message, sizeof(sms_message),
									 "LED3 is ON: %02d/%02d/20%02d %s %02d:%02d",
									 sDate.Date, sDate.Month, sDate.Year,
									 weekdays[sDate.WeekDay],
									 sTime.Hours, sTime.Minutes);
							sim800_send_sms1("+989124661714", sms_message);
						
							led3_end_minute = (led3_minute + led3_duration) % 60;
					}

					if (led3_on && sTime.Hours == led3_end_hour && sTime.Minutes == led3_end_minute) {
							HAL_GPIO_WritePin(LED3_GPIO, LED3_PIN, GPIO_PIN_RESET);
							led3_on = 0;
							sim800_send_sms1("+989124661714", "LED3 is OFF");
					}

		   }
		 else
		 {
			 __NOP();
		 }
if (door_sensor_enabled) {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET) {
        uint32_t now = HAL_GetTick();

        if ((now - last_door_event_time) > (2*60*60*1000)) {
            last_door_event_time = now;
            door_event_stage = 0;
        }

        if (door_event_stage == 0) {
            sim800_send_sms1("+989124661714", "Door is open");
            HAL_Delay(2000);
            door_event_stage = 1;
        }
        else if (door_event_stage == 1) {
            HAL_UART_Transmit(&huart2, (uint8_t*)"ATD+989124661714;\r\n",
                              strlen("ATD+989124661714;\r\n"), 1000);
            door_event_stage = 2;
        }
    }
}


				////////////////////////////////////////////////////////////////////////////////////////////////////
				
//        if (sTime.Hours == 0 && sTime.Minutes == 0 && sTime.Seconds == 0) {
//            uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//            if (sDate.Year % 4 == 0 && (sDate.Year % 100 != 0 || sDate.Year % 400 == 0)) {
//                days_in_month[1] = 29;
//            }
//            sDate.Date += 1;
//            if (sDate.Date > days_in_month[sDate.Month - 1]) {
//                sDate.Date = 1;
//                sDate.Month += 1;
//                if (sDate.Month > 12) {
//                    sDate.Month = 1;
//                    sDate.Year += 1;
//                }
//            }
//            sDate.WeekDay = (sDate.WeekDay % 7) + 1;
//            if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
//                Error_Handler();
//            }
//            HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, sDate.Year);
//            HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, sDate.Month);
//            HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, sDate.Date);
//            HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6, sDate.WeekDay);
//        }

//        const char *weekdays[] = {
//            "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
//        };
        sprintf(sending_data_str, "%02d/%02d/20%02d %s %02d:%02d:%02d\r\n",
                sDate.Date, sDate.Month, sDate.Year,
                weekdays[sDate.WeekDay],
                sTime.Hours, sTime.Minutes, sTime.Seconds);
        uart1_send_str(sending_data_str);
        HAL_Delay(500);
				
				HAL_IWDG_Refresh(&hiwdg);  // reset Watchdog
	
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	//Print error message and blink LED if an error occurs.
  __disable_irq();
	uart1_send_str("Error occurred!\r\n");
  while (1)
  {
	 HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
   HAL_Delay(500);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
