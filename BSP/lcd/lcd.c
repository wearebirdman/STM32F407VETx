#include "lcd.h"
#include "lcd_font.h"

#include <stdarg.h>

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

/* GPIO 快速控制（BSRR 直写，单周期原子操作） */
#define LCD_CS_LOW()                                      \
    do                                                    \
    {                                                     \
        LCD_CS_PORT->BSRR = (uint32_t)LCD_CS_PIN << 16U;  \
    } while (0)
#define LCD_CS_HIGH()                              \
    do                                             \
    {                                              \
        LCD_CS_PORT->BSRR = (uint32_t)LCD_CS_PIN;  \
    } while (0)
#define LCD_DC_LOW()                                      \
    do                                                    \
    {                                                     \
        LCD_DC_PORT->BSRR = (uint32_t)LCD_DC_PIN << 16U;  \
    } while (0)
#define LCD_DC_HIGH()                              \
    do                                             \
    {                                              \
        LCD_DC_PORT->BSRR = (uint32_t)LCD_DC_PIN;  \
    } while (0)
#define LCD_RST_LOW()                                       \
    do                                                      \
    {                                                       \
        LCD_RST_PORT->BSRR = (uint32_t)LCD_RST_PIN << 16U;  \
    } while (0)
#define LCD_RST_HIGH()                               \
    do                                               \
    {                                                \
        LCD_RST_PORT->BSRR = (uint32_t)LCD_RST_PIN;  \
    } while (0)
#define LCD_BLK_OFF()                                       \
    do                                                      \
    {                                                       \
        LCD_BLK_PORT->BSRR = (uint32_t)LCD_BLK_PIN << 16U;  \
    } while (0)
#define LCD_BLK_ON()                                 \
    do                                               \
    {                                                \
        LCD_BLK_PORT->BSRR = (uint32_t)LCD_BLK_PIN;  \
    } while (0)

/* 使用 FreeRTOS 时的头文件 */
#ifdef LCD_USE_FREERTOS
#include "cmsis_os.h"
#endif

/* DMA 传输相关 */
#ifdef LCD_USE_DMA
#include "dma.h"
extern DMA_HandleTypeDef hdma_spi1_tx;

static volatile uint8_t s_dma_busy = 0;    // DMA 忙标志
static uint8_t s_dma_cs_ctrl = 0;          // CS 控制模式：0=回调管（图片/视频），1=调用者管（填充/图形库）

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != LCD_SPI_HANDLE.Instance)
        return;

    if (s_dma_cs_ctrl == 0)
        LCD_CS_HIGH(); // 单次模式：回调自动拉高 CS
    // 分段模式：CS 由调用者管理，回调不操作
    s_dma_busy = 0;
}
#endif

/* SPI 单字节发送（阻塞） */
static inline void Lcd_SpiTxByte(uint8_t dat)
{
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, &dat, 1, LCD_SPI_TIMEOUT);
}

/* SPI 发送一个 RGB565 颜色（阻塞） */
static inline void Lcd_SpiTxColor(uint16_t color)
{
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, buf, 2, LCD_SPI_TIMEOUT);
}

/* SPI 发送缓冲区（阻塞） */
static void Lcd_SpiTxBuf(const uint8_t *buf, uint16_t len)
{
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, (uint8_t *)buf, len, LCD_SPI_TIMEOUT);
}

/* DMA 发送缓冲区 */
#ifdef LCD_USE_DMA

/* 等待 DMA 传输完成，带超时保护 */
static void Lcd_WaitDma(void)
{
    uint32_t timeout = HAL_GetTick() + LCD_SPI_TIMEOUT;
    while (s_dma_busy)
    {
        if (HAL_GetTick() > timeout)
        {
            HAL_SPI_Abort(&LCD_SPI_HANDLE);
            s_dma_busy = 0;
            if (s_dma_cs_ctrl == 0)
                LCD_CS_HIGH(); // 仅单次模式超时需要拉高 CS
            break;
        }
#ifdef LCD_USE_FREERTOS
        // 调度器未启动时（Lcd_Init 阶段）不能调用 osThreadYield，改用忙等
        if (osKernelGetState() == osKernelRunning)
            osThreadYield();
#endif
    }
}

/* DMA 发送缓冲区，失败则回退阻塞式 */
static void Lcd_SpiTxBufDma(const uint8_t *buf, uint32_t len)
{
    if (len == 0)
        return;

    Lcd_WaitDma(); // 等待上一次传输结束
    s_dma_busy = 1;

    if (HAL_SPI_Transmit_DMA(&LCD_SPI_HANDLE, (uint8_t *)buf, len) != HAL_OK)
    {
        // DMA 启动失败 → 回退阻塞式
        HAL_SPI_Transmit(&LCD_SPI_HANDLE, (uint8_t *)buf, len, LCD_SPI_TIMEOUT);
        if (s_dma_cs_ctrl == 0)
            LCD_CS_HIGH();
        s_dma_busy = 0;
    }
}

#endif /* LCD_USE_DMA */

/* 发送单条命令（CS 自动管理） */
static void Lcd_Cmd(uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_LOW();
    Lcd_SpiTxByte(cmd);
    LCD_DC_HIGH();
    LCD_CS_HIGH();
}

/* 命令开始（拉低 CS，发送命令，DC 切到数据态，CS 保持低） */
static inline void Lcd_CmdBegin(uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_LOW();
    Lcd_SpiTxByte(cmd);
    LCD_DC_HIGH();
}

/* 命令结束（拉高 CS） */
static inline void Lcd_CmdEnd(void)
{
    LCD_CS_HIGH();
}

/* 发送命令 + 数据（CS 自动管理） */
static void Lcd_CmdData(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    Lcd_CmdBegin(cmd);
    if (len > 0)
        Lcd_SpiTxBuf(data, len);
    Lcd_CmdEnd();
}

/* 设置地址窗口，结束后 CS=LOW、DC=HIGH，等待像素数据输入；调用者需在数据发送完成后调用 LCD_CS_HIGH() */
static void Lcd_SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    uint8_t xbuf[4] = {xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF};
    uint8_t ybuf[4] = {ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF};

    LCD_CS_LOW();
    LCD_DC_LOW();
    Lcd_SpiTxByte(0x2A); // 设置列地址
    LCD_DC_HIGH();
    Lcd_SpiTxBuf(xbuf, 4);
    LCD_DC_LOW();
    Lcd_SpiTxByte(0x2B); // 设置行地址
    LCD_DC_HIGH();
    Lcd_SpiTxBuf(ybuf, 4);
    LCD_DC_LOW();
    Lcd_SpiTxByte(0x2C); // 准备写入像素数据
    LCD_DC_HIGH();
}

/* ST7735S 初始化序列 */
void Lcd_Init(void)
{
    /* 硬件复位 */
    LCD_RST_LOW();
    HAL_Delay(20);
    LCD_RST_HIGH();
    HAL_Delay(150);

    Lcd_Cmd(0x01);
    HAL_Delay(150); // 软件复位
    Lcd_Cmd(0x11);
    HAL_Delay(200); // 退出睡眠

    /* 显示区域 */
    { // CASET (0x2A)：列地址
        uint8_t col_data[] = {0x00, 0x00,
                              (uint8_t)((LCD_W - 1) >> 8), (uint8_t)(LCD_W - 1)};
        Lcd_CmdData(0x2A, col_data, sizeof(col_data));
    }
    { // RASET (0x2B)：行地址
        uint8_t row_data[] = {0x00, 0x00,
                              (uint8_t)((LCD_H - 1) >> 8), (uint8_t)(LCD_H - 1)};
        Lcd_CmdData(0x2B, row_data, sizeof(row_data));
    }

    /* 帧率控制 */
    {
        uint8_t frm_data[] = {0x01, 0x2C, 0x2D};
        Lcd_CmdData(0xB1, frm_data, sizeof(frm_data)); // FRMCTR1：正常模式帧率
        Lcd_CmdData(0xB2, frm_data, sizeof(frm_data)); // FRMCTR2：空闲模式帧率
        Lcd_CmdData(0xB3, frm_data, sizeof(frm_data)); // FRMCTR3：部分模式帧率
    }
    {
        uint8_t inv_data[] = {0x07};
        Lcd_CmdData(0xB4, inv_data, sizeof(inv_data)); // INVCTR：列反转控制
    }

    /* 电源序列 */
    {
        uint8_t pw1_data[] = {0xA2, 0x02, 0x84};
        Lcd_CmdData(0xC0, pw1_data, sizeof(pw1_data));
    }
    {
        uint8_t pw2_data[] = {0xC5};
        Lcd_CmdData(0xC1, pw2_data, sizeof(pw2_data));
    }
    {
        uint8_t pw3_data[] = {0x0A, 0x00};
        Lcd_CmdData(0xC2, pw3_data, sizeof(pw3_data));
    }
    {
        uint8_t pw4_data[] = {0x8A, 0x2A};
        Lcd_CmdData(0xC3, pw4_data, sizeof(pw4_data));
    }
    {
        uint8_t pw5_data[] = {0x8A, 0xEE};
        Lcd_CmdData(0xC4, pw5_data, sizeof(pw5_data));
    }
    {
        uint8_t vcm_data[] = {0x0E};
        Lcd_CmdData(0xC5, vcm_data, sizeof(vcm_data));
    }

    /* 显示方向 */
    {
        uint8_t madctl;
        switch (USE_HORIZONTAL)
        {
        case 0:
            madctl = 0x00;
            break; // 竖屏 正常
        case 1:
            madctl = 0xC0;
            break; // 竖屏 镜像
        case 2:
            madctl = 0x70;
            break; // 横屏 正常
        case 3:
            madctl = 0xA0;
            break; // 横屏 镜像
        default:
            madctl = 0x00;
            break;
        }
        Lcd_CmdData(0x36, &madctl, 1); // MADCTL：内存数据访问控制
    }

    /* Gamma */
    {
        uint8_t gamma_pos[] = {
            0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22,
            0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10};
        Lcd_CmdData(0xE0, gamma_pos, sizeof(gamma_pos)); // GMCTRP1：正 Gamma 校正
    }
    {
        uint8_t gamma_neg[] = {
            0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E,
            0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10};
        Lcd_CmdData(0xE1, gamma_neg, sizeof(gamma_neg)); // GMCTRN1：负 Gamma 校正
    }

    /* 像素格式：RGB565 */
    {
        uint8_t fmt_data[] = {0x05};
        Lcd_CmdData(0x3A, fmt_data, sizeof(fmt_data));
    }

    /* 普通显示模式 */
    Lcd_Cmd(0x13); // NORON：正常显示模式

    /* 打开显示 */
    Lcd_Cmd(0x29); // DISPON：打开显示
    HAL_Delay(50); // 等待显示稳定

    /* 清屏：GRAM 上电后内容随机，不清屏会花屏 */
    Lcd_Clear(LCD_BLACK);

    /* 清屏完成后再打开背光，避免用户看到花屏 */
    LCD_BLK_ON();
}

/* 清屏（全屏填充指定颜色） */
void Lcd_Clear(uint16_t color)
{
    Lcd_Fill(0, 0, LCD_W - 1, LCD_H - 1, color);
}

/* 区域填充（CS 期间 SPI 无间断） */
void Lcd_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t pixels;
    uint16_t batch;

    if (x1 > x2)
    {
        uint16_t t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2)
    {
        uint16_t t = y1;
        y1 = y2;
        y2 = t;
    }
    if (x1 > LCD_W - 1)
        x1 = LCD_W - 1;
    if (x2 > LCD_W - 1)
        x2 = LCD_W - 1;
    if (y1 > LCD_H - 1)
        y1 = LCD_H - 1;
    if (y2 > LCD_H - 1)
        y2 = LCD_H - 1;

    pixels = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    if (pixels == 0)
        return;

    Lcd_SetWindow(x1, y1, x2, y2);

#ifdef LCD_USE_DMA
    // DMA 路径：1024 字节静态缓冲区，颜色只预填一次
    {
        static uint8_t fill_buf[1024]; // 512 像素，占 RAM 1 KB
        uint16_t i;
        uint8_t hi = color >> 8, lo = color & 0xFF;

        // 颜色预填：1024 字节只做一次
        for (i = 0; i < sizeof(fill_buf); i += 2)
        {
            fill_buf[i] = hi;
            fill_buf[i + 1] = lo;
        }

        // 分段模式：CS 由调用者管理，多次 DMA 共享同一个窗口
        s_dma_cs_ctrl = 1;

        while (pixels > 0)
        {
            batch = (pixels > 512) ? 512 : (uint16_t)pixels;
            Lcd_SpiTxBufDma(fill_buf, (uint32_t)batch * 2);
            pixels -= batch;
        }
        Lcd_WaitDma(); // 确保最后一段传输完成

        s_dma_cs_ctrl = 0; // 恢复单次模式
    }
#else
    // 阻塞路径
    {
        uint8_t buf[128];
        uint8_t hi = color >> 8, lo = color & 0xFF;
        uint16_t i;
        for (i = 0; i < sizeof(buf); i += 2)
        {
            buf[i] = hi;
            buf[i + 1] = lo;
        }
        while (pixels > 0)
        {
            batch = (pixels > 64) ? 64 : (uint16_t)pixels;
            Lcd_SpiTxBuf(buf, batch * 2);
            pixels -= batch;
        }
    }
#endif

    LCD_CS_HIGH();
}

/* 画点 */
void Lcd_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_W || y >= LCD_H)
        return;
    Lcd_SetWindow(x, y, x, y);
    Lcd_SpiTxColor(color);
    LCD_CS_HIGH();
}

/* 画线（Bresenham 算法） */
void Lcd_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    int dx, dy, sx, sy, cx, cy, err, e2;
    uint32_t steps;

    // 快速剔除：两端点都在屏幕同一侧外部
    if (x1 >= LCD_W && x2 >= LCD_W)
        return; // 两点都在右侧外部
    if (x1 < 0 && x2 < 0)
        return; // 两点都在左侧外部
    if (y1 >= LCD_H && y2 >= LCD_H)
        return; // 两点都在下方外部
    if (y1 < 0 && y2 < 0)
        return; // 两点都在上方外部

    dx = (int)x2 - (int)x1;
    dy = (int)y2 - (int)y1;
    sx = (dx > 0) ? 1 : (dx < 0) ? -1
                                 : 0;
    sy = (dy > 0) ? 1 : (dy < 0) ? -1
                                 : 0;
    cx = x1;
    cy = y1;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    err = (dx > dy) ? dx : -dy;
    steps = (uint32_t)(dx > dy ? dx : dy) + 1;

    for (;;)
    {
        Lcd_DrawPoint(cx, cy, color);
        if (cx == x2 && cy == y2)
            break;
        if (--steps == 0)
            break; // 保底：防止意外死循环
        e2 = err * 2;
        if (e2 > -dy)
        {
            err -= dy;
            cx += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            cy += sy;
        }
    }
}

/* 画矩形 */
void Lcd_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    Lcd_DrawLine(x1, y1, x2, y1, color);
    Lcd_DrawLine(x1, y2, x2, y2, color);
    Lcd_DrawLine(x1, y1, x1, y2, color);
    Lcd_DrawLine(x2, y1, x2, y2, color);
}

/* 画圆（中点圆算法） */
void Lcd_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a = 0, b = r;
    int rr = (int)r * (int)r;

    if (r == 0)
        return;

    while (a <= b)
    {
        Lcd_DrawPoint(x0 - b, y0 - a, color);
        Lcd_DrawPoint(x0 + b, y0 - a, color);
        Lcd_DrawPoint(x0 - a, y0 + b, color);
        Lcd_DrawPoint(x0 - a, y0 - b, color);
        Lcd_DrawPoint(x0 + b, y0 + a, color);
        Lcd_DrawPoint(x0 + a, y0 - b, color);
        Lcd_DrawPoint(x0 + a, y0 + b, color);
        Lcd_DrawPoint(x0 - b, y0 + a, color);
        a++;
        if ((a * a + b * b) > rr)
            b--;
    }
}

/* 显示单个字符 */
void Lcd_ShowChar(uint16_t x, uint16_t y, char ch,
                  uint16_t fc, uint16_t bc, uint8_t size)
{
    uint8_t sizex, t, col;
    uint16_t bytes_per_char, i;
    const uint8_t *font;
    uint8_t temp;

    if (ch < ' ' || ch > '~')
        return;
    sizex = size / 2;
    if (x + sizex > LCD_W || y + size > LCD_H)
        return;
    if (size != 12 && size != 16)
        return;

    bytes_per_char = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * size;
    ch -= ' ';
    font = (size == 12) ? ascii_1206[(uint8_t)ch] : ascii_1608[(uint8_t)ch];

    Lcd_SetWindow(x, y, x + sizex - 1, y + size - 1);

    col = 0;
    for (i = 0; i < bytes_per_char; i++)
    {
        temp = font[i];
        for (t = 0; t < 8; t++)
        {
            Lcd_SpiTxColor((temp & (0x01 << t)) ? fc : bc);
            if (++col >= sizex)
            {
                col = 0;
                break;
            }
        }
    }
    LCD_CS_HIGH();
}

/* 将字符串第 row 行的像素按 RGB565 写入 buf（内部工具） */
static void Lcd_FillStringRow(uint8_t *buf, const char *str, uint16_t len,
                              uint8_t sizex, uint8_t size, uint8_t row,
                              uint8_t hi_fc, uint8_t lo_fc,
                              uint8_t hi_bc, uint8_t lo_bc)
{
    uint16_t i;
    uint8_t t;

    for (i = 0; i < len; i++)
    {
        uint8_t ch = (uint8_t)str[i];
        uint8_t byte = (ch >= ' ' && ch <= '~')
                           ? ((size == 12) ? ascii_1206[ch - ' '][row]
                                           : ascii_1608[ch - ' '][row])
                           : 0; // 不可打印字符按背景填充，保持行内字符位置

        for (t = 0; t < sizex; t++)
        {
            if (byte & (0x01U << t))
            {
                *buf++ = hi_fc;
                *buf++ = lo_fc;
            }
            else
            {
                *buf++ = hi_bc;
                *buf++ = lo_bc;
            }
        }
    }
}

/* 显示字符串（批量渲染，整个字符串只设置一次窗口） */
void Lcd_ShowString(uint16_t x, uint16_t y, const char *str,
                    uint16_t fc, uint16_t bc, uint8_t size)
{
    uint8_t sizex = size / 2;
    uint16_t len = 0, str_w;
    uint16_t max_w, row;
    uint8_t hi_fc, lo_fc, hi_bc, lo_bc;

    if (str == NULL)
        return;
    if (size != 12 && size != 16)
        return;
    if (x >= LCD_W || y + size > LCD_H)
        return;

    // 计算实际可显示的字符数（右侧裁剪）
    max_w = (uint16_t)(LCD_W - x);
    while (str[len] != '\0' && len < max_w / sizex)
        len++;
    if (len == 0)
        return;
    str_w = (uint16_t)(len * sizex);

    Lcd_SetWindow(x, y, (uint16_t)(x + str_w - 1), (uint16_t)(y + size - 1));

    hi_fc = fc >> 8;
    lo_fc = fc & 0xFF;
    hi_bc = bc >> 8;
    lo_bc = bc & 0xFF;

#ifdef LCD_USE_DMA
    {
        static uint8_t line_buf[2][LCD_W * 2]; // 双缓冲：构建与传输并行
        uint8_t buf_idx = 0;

        s_dma_cs_ctrl = 1; // 分段模式：CS 由本函数管理

        for (row = 0; row < size; row++)
        {
            Lcd_FillStringRow(line_buf[buf_idx], str, len, sizex, size, row,
                              hi_fc, lo_fc, hi_bc, lo_bc);
            Lcd_SpiTxBufDma(line_buf[buf_idx], (uint32_t)str_w * 2); // 内部等待上一行完成
            buf_idx ^= 1;
        }
        Lcd_WaitDma();      // 确保最后一行传输完成
        s_dma_cs_ctrl = 0; // 恢复单次模式
    }
#else
    {
        static uint8_t line_buf[LCD_W * 2];

        for (row = 0; row < size; row++)
        {
            Lcd_FillStringRow(line_buf, str, len, sizex, size, row,
                              hi_fc, lo_fc, hi_bc, lo_bc);
            Lcd_SpiTxBuf(line_buf, (uint16_t)(str_w * 2));
        }
    }
#endif

    LCD_CS_HIGH();
}

/* 无符号整数转字符串（替代 sprintf，无需 newlib） */
static void Lcd_Utoa(char *buf, unsigned int val)
{
    char tmp[11];
    int i = 0;
    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0)
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0)
        *buf++ = tmp[--i];
    *buf = '\0';
}

/* 有符号整数转字符串 */
static void Lcd_Itoa(char *buf, int val)
{
    char *p = buf;
    if (val < 0)
    {
        *p++ = '-';
        val = -val;
    }
    Lcd_Utoa(p, (unsigned int)val);
}

/* 十六进制转字符串（小写） */
static void Lcd_Xtoa(char *buf, unsigned int val)
{
    const char hex[] = "0123456789abcdef";
    char tmp[9];
    int i = 0;
    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0)
    {
        tmp[i++] = hex[val & 0x0F];
        val >>= 4;
    }
    while (i > 0)
        *buf++ = tmp[--i];
    *buf = '\0';
}

/* 类似 printf 的 LCD 显示函数的内部实现 */
static void Lcd_PrintInner(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t size, const char *fmt, va_list ap)
{
    uint8_t sizex = size / 2;
    char c;
    char buf[32];

    while ((c = *fmt++) != '\0')
    {
        if (c != '%')
        {
            Lcd_ShowChar(x, y, c, fc, bc, size);
            x += sizex;
            continue;
        }

        // 解析 % 后面的格式符
        c = *fmt++;
        switch (c)
        {
        case 'd':
        case 'i': {
            int val = va_arg(ap, int);
            Lcd_Itoa(buf, val);
            const char *s = buf;
            while (*s)
            {
                Lcd_ShowChar(x, y, *s++, fc, bc, size);
                x += sizex;
            }
            break;
        }
        case 'u': {
            unsigned int val = va_arg(ap, unsigned int);
            Lcd_Utoa(buf, val);
            const char *s = buf;
            while (*s)
            {
                Lcd_ShowChar(x, y, *s++, fc, bc, size);
                x += sizex;
            }
            break;
        }
        case 'x': {
            unsigned int val = va_arg(ap, unsigned int);
            Lcd_Xtoa(buf, val);
            const char *s = buf;
            while (*s)
            {
                Lcd_ShowChar(x, y, *s++, fc, bc, size);
                x += sizex;
            }
            break;
        }
        case 'c': {
            char ch = (char)va_arg(ap, int);
            Lcd_ShowChar(x, y, ch, fc, bc, size);
            x += sizex;
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == NULL)
                s = "(null)";
            while (*s)
            {
                Lcd_ShowChar(x, y, *s++, fc, bc, size);
                x += sizex;
            }
            break;
        }
        case 'f': {
            int ival = (int)(va_arg(ap, double) * 100 + 0.5);
            char *p = buf;
            if (ival < 0)
            {
                *p++ = '-';
                ival = -ival;
            }
            Lcd_Utoa(p, (unsigned int)(ival / 100));
            while (*p)
                p++; // 跳到末尾
            *p++ = '.';
            unsigned int frac = (unsigned int)(ival % 100);
            if (frac < 10)
                *p++ = '0';
            Lcd_Utoa(p, frac);
            const char *s = buf;
            while (*s)
            {
                Lcd_ShowChar(x, y, *s++, fc, bc, size);
                x += sizex;
            }
            break;
        }
        case '%':
            Lcd_ShowChar(x, y, '%', fc, bc, size);
            x += sizex;
            break;
        default:
            // 未知格式：原样输出
            Lcd_ShowChar(x, y, '%', fc, bc, size);
            x += sizex;
            Lcd_ShowChar(x, y, c, fc, bc, size);
            x += sizex;
            break;
        }
    }
}

/* 类似 printf 的 LCD 显示函数 */
void Lcd_Printf(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Lcd_PrintInner(x, y, fc, bc, size, fmt, ap);
    va_end(ap);
}

/* 显示图片（DMA），支持自动裁剪超出屏幕的部分 */
#ifdef LCD_USE_DMA
void Lcd_ShowPicture(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint8_t *pic, uint32_t data_len)
{
    int16_t row_start, row_end;
    int16_t col_start, col_end;
    uint16_t draw_w;
    uint16_t row;
    uint32_t byte_offset;

    if (pic == NULL || w == 0 || h == 0)
        return;

    // 数据长度校验：至少要有 w * h * 2 字节
    if (data_len != 0 && data_len < (uint32_t)w * (uint32_t)h * 2)
        return;

    // 裁剪：计算屏幕内可见区域
    row_start = 0;
    row_end = (int16_t)(h - 1);

    if (y < 0)
        row_start = -y; // 图片顶部在屏幕上方，跳过不可见行
    if (y + h > LCD_H)
        row_end = LCD_H - 1 - y; // 图片底部超出屏幕

    col_start = 0;
    col_end = (int16_t)(w - 1);

    if (x < 0)
        col_start = -x; // 图片左侧在屏幕左方
    if (x + w > LCD_W)
        col_end = LCD_W - 1 - x; // 图片右侧超出屏幕

    // 完全不可见
    if (row_start > row_end || col_start > col_end)
        return;

    draw_w = (uint16_t)(col_end - col_start + 1);

    // 逐行发送可见部分
    byte_offset = (uint32_t)col_start * 2; // 每行跳过的字节数
    for (row = (uint16_t)row_start; row <= (uint16_t)row_end; row++)
    {
        uint16_t lcd_y = (uint16_t)(y + row);

        Lcd_SetWindow((uint16_t)(x + col_start), lcd_y,
                      (uint16_t)(x + col_end), lcd_y);

        Lcd_SpiTxBufDma(&pic[(uint32_t)row * w * 2 + byte_offset],
                        (uint32_t)draw_w * 2);
        Lcd_WaitDma();
    }
}
#endif /* LCD_USE_DMA */
