/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "adc.h"
#include "st7789.h"
#include "fonts.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define ADC_RESOLUTION 4095
#define ADC_SAMPLES 100
#define TS_CAL1 *((uint16_t*)0x1FFF7A2C) // Factory calibration value at 30°C
#define TS_CAL2 *((uint16_t*)0x1FFF7A2E) // Factory calibration value at 110°C

#define VREFINT_CAL_ADDR ((uint16_t*)0x1FFF7A2A)
#define RX_BUFFER_SIZE 21  // 20 ký tự + 1 ký tự null
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_index = 0;


//#define VREFINT_CAL_ADDR ((uint16_t*)0x1FFF7A2A) // Address for VREFINT_CAL

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
float temperature = 0.0f;
float vdda = 3.3f; // Supply voltage, will be calculated using VREFINT

uint16_t adc_buffer[ADC_SAMPLES * 2 * 2] = { 0 };

osMessageQId uartQueueHandle;

void TurnOffLCD(void)
{
    ST7789_Fill_Color(BLACK); // Đặt toàn màn hình thành màu đen
}

void EnterSleepMode(void)
{
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI); // Chế độ Sleep
}




static void ReadTemperature(void)
{
  uint32_t adc_temp, adc_vref;

  // Read VREFINT channel
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  adc_vref = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  // Calculate VDDA
  vdda = 3.3f * ((float)(*VREFINT_CAL_ADDR) / (float)adc_vref);

  // Read Temperature channel
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  adc_temp = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  // Calculate temperature
  float ts_voltage = ((float)adc_temp * vdda) / ADC_RESOLUTION;
  temperature = ((ts_voltage - 0.76f) / 0.0025f) + 25.0f;
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TempHandle;
osThreadId LCDHandle;
osThreadId SoundHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask01(void const * argument);
void StartTask02(void const * argument);
void StartTask03(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */


	osMessageQDef(uartQueue, 20, char*); // Hàng đợi chứa con trỏ chuỗi
	uartQueueHandle = osMessageCreate(osMessageQ(uartQueue), NULL);


  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of Temp */
  osThreadDef(Temp, StartTask01, osPriorityIdle, 0, 128);
  TempHandle = osThreadCreate(osThread(Temp), NULL);

  /* definition and creation of LCD */
  osThreadDef(LCD, StartTask02, osPriorityIdle, 0, 128);
  LCDHandle = osThreadCreate(osThread(LCD), NULL);

  /* definition and creation of Sound */
  osThreadDef(Sound, StartTask03, osPriorityIdle, 0, 128);
  SoundHandle = osThreadCreate(osThread(Sound), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rx_data; // Biến nhận ký tự mới nhất

    if (huart->Instance == USART2) // Xử lý UART2
    {
        if (rx_data == '\n' || rx_data == '\r') // Ký tự kết thúc chuỗi
        {
            rx_buffer[rx_index] = '\0'; // Thêm ký tự kết thúc chuỗi
            xQueueSendFromISR(uartQueueHandle, rx_buffer, NULL); // Gửi toàn bộ chuỗi vào hàng đợi
            rx_index = 0; // Reset chỉ số buffer
        }
        else if (rx_index < RX_BUFFER_SIZE - 1) // Chỉ thêm ký tự nếu còn không gian
        {
            rx_buffer[rx_index++] = rx_data; // Lưu ký tự vào buffer
        }

        // Tiếp tục nhận ký tự tiếp theo
        HAL_UART_Receive_IT(&huart2, &rx_data, 1); // Khởi động nhận ký tự tiếp theo
    }
}

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask01 */
/**
* @brief Function implementing the Temp thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask01 */
void StartTask01(void const * argument)
{
  /* USER CODE BEGIN StartTask01 */
  /* Infinite loop */
  for(;;)
  {

	HAL_GPIO_TogglePin(GPIOB, LED0_Pin);
    osDelay(1500);
  }
  /* USER CODE END StartTask01 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the LCD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the Sound thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void const * argument)
{
  /* USER CODE BEGIN StartTask03 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
