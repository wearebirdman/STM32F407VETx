/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "key.h"
#include "lcd.h"
#include "led.h"
#include "bl_param.h"
#include "bl_jump.h"
#include "bl_update_uart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 界面布局常量（128x160竖屏） */
#define UI_TX       14
#define UI_TY       10
#define UI_STAT_Y   96          /* 状态文字行 */
#define UI_BAR_X    8
#define UI_BAR_Y    126
#define UI_BAR_W    112
#define UI_BAR_H    10

static KeyMsg_t s_key_msg;
static uint32_t s_bl_cmd;

/* 主菜单界面 */
static void Menu_Show(void)
{
    Lcd_Clear(LCD_BLACK);
    Lcd_ShowString(UI_TX, 8, "BOOTLOADER", LCD_YELLOW, LCD_BLACK, 16);
    Lcd_DrawLine(6, 26, 122, 26, LCD_BLUE);
    Lcd_ShowString(10, 40, "WKUP  Jump APP", LCD_WHITE, LCD_BLACK, 12);
    Lcd_ShowString(10, 62, "KEY1  UART Upd", LCD_WHITE, LCD_BLACK, 12);
    Lcd_ShowString(10, 140, "v1.0", LCD_GRAY, LCD_BLACK, 12);
}

/* 升级进度回调
 *   total < 600  视为"等待倒计时"（单位秒），received为剩余秒数
 *   否则为字节进度，绘制百分比+进度条 */
static void Bl_Prog(uint32_t received, uint32_t total, void *ctx)
{
    (void)ctx;
    if (total < 600U)                    /* 连接倒计时阶段 */
    {
        Lcd_Printf(10, UI_STAT_Y, LCD_YELLOW, LCD_BLACK, 12, "Waiting... %lus", received);
        return;
    }

    unsigned int pct = total ? (unsigned int)(received * 100U / total) : 0U;
    if (pct > 100U) pct = 100U;
    Lcd_Printf(10, UI_STAT_Y, LCD_WHITE, LCD_BLACK, 12, "Writing... %u%%", pct);

    /* 重绘进度条 */
    Lcd_Fill(UI_BAR_X, UI_BAR_Y, UI_BAR_X + UI_BAR_W, UI_BAR_Y + UI_BAR_H, LCD_BLACK);
    Lcd_DrawRectangle(UI_BAR_X, UI_BAR_Y, UI_BAR_X + UI_BAR_W, UI_BAR_Y + UI_BAR_H, LCD_BLUE);
    if (pct > 0U)
    {
        uint16_t fill = (uint16_t)((UI_BAR_W - 2U) * pct / 100U);
        Lcd_Fill(UI_BAR_X + 1, UI_BAR_Y + 1, UI_BAR_X + 1U + fill, UI_BAR_Y + UI_BAR_H - 1U, LCD_GREEN);
    }
}

/* 升级结果文字 */
static const char *Bl_ResultStr(BlUpdStatus_t st)
{
    switch (st)
    {
        case BL_UPD_OK:           return "Upgrade OK";
        case BL_UPD_ERR_OPEN:     return "ERR: open source";
        case BL_UPD_ERR_HEADER:   return "ERR: bad header";
        case BL_UPD_ERR_SIZE:     return "ERR: size";
        case BL_UPD_ERR_CRC:      return "ERR: CRC";
        case BL_UPD_ERR_FLASH:    return "ERR: flash";
        case BL_UPD_ERR_ABORT:    return "ERR: aborted";
        case BL_UPD_ERR_TIMEOUT:  return "ERR: timeout";
        case BL_UPD_ERR_PROTO:    return "ERR: protocol";
        case BL_UPD_ERR_PARAM:    return "ERR: param";
        default:                  return "ERR: unknown";
    }
}

/* 结果界面：成功绿色SUCCESS，失败红色FAILED+原因 */
static void Show_Result(BlUpdStatus_t st, const char *title)
{
    uint16_t fg = (st == BL_UPD_OK) ? LCD_GREEN : LCD_RED;
    Lcd_Clear(LCD_BLACK);
    Lcd_ShowString(UI_TX, UI_TY, title, LCD_YELLOW, LCD_BLACK, 16);
    Lcd_ShowString(UI_TX, 46, (st == BL_UPD_OK) ? "SUCCESS!" : "FAILED", fg, LCD_BLACK, 16);
    Lcd_ShowString(10, 80, Bl_ResultStr(st), LCD_WHITE, LCD_BLACK, 12);
    if (st == BL_UPD_OK)
        Lcd_ShowString(10, 120, "Jumping to APP...", LCD_GRAY, LCD_BLACK, 12);
    else
        Lcd_ShowString(10, 120, "Press KEY1 back", LCD_GRAY, LCD_BLACK, 12);
}

/* 等待指定按键短按 */
static void Wait_Key(uint8_t id)
{
    KeyMsg_t m;
    for (;;)
    {
        m = Key_Scan();
        if ((m.key_id == id) && (m.event == KEY_EVENT_SHORT_PRESS))
            break;
        HAL_Delay(10);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_CRC_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  Key_Init();
  Lcd_Init();

  /* 读参数区命令字：无升级命令且APP有效则直接跳转 */
  BlParam_ReadCmd(&s_bl_cmd);
  if ((s_bl_cmd == BL_CMD_NONE) && Bl_AppValid())
  {
      Bl_JumpToApp();
  }
  Menu_Show();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    s_key_msg = Key_Scan();
    switch (s_key_msg.key_id)
    {
      case KEY_WK_ID:
        if (s_key_msg.event == KEY_EVENT_SHORT_PRESS)
        {
          if (Bl_AppValid())
            Bl_JumpToApp();
          else
            Lcd_Printf(10, 130, LCD_WHITE, LCD_BLACK, 12, "No valid APP");
        }
        break;
      case KEY_1_ID:
        if (s_key_msg.event == KEY_EVENT_SHORT_PRESS)
        {
          BlUpdStatus_t st;
          Lcd_Clear(LCD_BLACK);
          Lcd_ShowString(UI_TX, UI_TY, "UART Firmware", LCD_YELLOW, LCD_BLACK, 16);
          Lcd_ShowString(10, 40, "Send FW.BIN via", LCD_GRAY, LCD_BLACK, 12);
          Lcd_ShowString(10, 56, "KEY1 : Give Up", LCD_WHITE, LCD_BLACK, 12);
          st = Bl_UpdateUartStart(Bl_Prog, 0);
          if (st == BL_UPD_OK)
          {
            Show_Result(st, "UART Firmware");
            HAL_Delay(500);
            if (Bl_AppValid())
              Bl_JumpToApp();
          }
          else
          {
            Show_Result(st, "UART Firmware");
            Wait_Key(KEY_1_ID);
            Menu_Show();
          }
        }
        break;
      default:
        break;
    }
    HAL_Delay(10);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
