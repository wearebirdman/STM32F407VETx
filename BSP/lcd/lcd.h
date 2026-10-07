#ifndef __LCD_H
#define __LCD_H

#include "main.h"

/* 功能开关（按需取消注释） */
#define LCD_USE_DMA
/* #define LCD_USE_FREERTOS */

/* SPI 句柄（CubeMX 生成的全局变量名） */
#define LCD_SPI_HANDLE hspi2

/* 引脚定义（GPIO 端口 + 引脚号） */
#define LCD_CS_PORT   GPIOB /* 片选，低电平有效 */
#define LCD_CS_PIN    GPIO_PIN_14
#define LCD_DC_PORT   GPIOC /* 数据/命令选择，低=命令 高=数据 */
#define LCD_DC_PIN    GPIO_PIN_6
#define LCD_RST_PORT  GPIOB /* 复位，低电平有效 */
#define LCD_RST_PIN   GPIO_PIN_12
#define LCD_BLK_PORT  GPIOC /* 背光，高电平亮 */
#define LCD_BLK_PIN   GPIO_PIN_7

/* 显示方向：0/1=竖屏(128×160)，2/3=横屏(160×128) */
#define USE_HORIZONTAL 1

/* I2C/SPI 超时（ms） */
#define LCD_SPI_TIMEOUT 100

/* 屏幕分辨率（由 USE_HORIZONTAL 决定） */
#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 128
#define LCD_H 160
#else
#define LCD_W 160
#define LCD_H 128
#endif

/* 颜色定义（RGB565） */
#define LCD_WHITE   0xFFFF
#define LCD_BLACK   0x0000
#define LCD_BLUE    0x001F
#define LCD_RED     0xF800
#define LCD_GREEN   0x07E0
#define LCD_CYAN    0x7FFF
#define LCD_YELLOW  0xFFE0
#define LCD_MAGENTA 0xF81F
#define LCD_GRAY    0x8430
#define LCD_ORANGE  0xFC00

/* 函数接口 */
void Lcd_Init(void);
void Lcd_Clear(uint16_t color);
void Lcd_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void Lcd_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void Lcd_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void Lcd_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void Lcd_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void Lcd_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc, uint8_t size);
void Lcd_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc, uint8_t size);
void Lcd_Printf(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t size, const char *fmt, ...);

/* DMA 扩展（需定义 LCD_USE_DMA，需 CubeMX 配置 DMA） */
#ifdef LCD_USE_DMA
/* x, y 允许负数（图片部分在屏幕外时自动裁剪），data_len=0 则不校验数据长度 */
void Lcd_ShowPicture(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t *pic, uint32_t data_len);
#endif

#endif /* __LCD_H */
