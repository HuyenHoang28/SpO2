#include "uart_stream.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* USART1 on PA9(TX)/PA10(RX), TX via DMA2 Stream7 Channel4.
 *
 * The vendor HAL UART driver (stm32f4xx_hal_uart.c) is not present in this
 * project's Drivers/ tree, so we drive USART1 through direct register access
 * and only rely on HAL_DMA_* for the DMA stream. This keeps the change
 * self-contained: no HAL source files need to be added to the build. */

DMA_HandleTypeDef hdma_usart1_tx;

#define UART_TX_BUFFER_SIZE   64U
#define UART_BAUD             115200U

static char     s_tx_buffer[UART_TX_BUFFER_SIZE];
static volatile bool s_tx_busy = false;
static volatile uint32_t s_dropped_count = 0U;

static void UartStream_DmaTxCplt(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    /* Wait for TC in USART_SR to guarantee the last byte has left the shift
     * register before we release the busy flag. */
    while ((USART1->SR & USART_SR_TC) == 0U) { /* spin */ }
    USART1->CR3 &= ~USART_CR3_DMAT;
    s_tx_busy = false;
}

static void UartStream_DmaTxError(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    USART1->CR3 &= ~USART_CR3_DMAT;
    s_tx_busy = false;
    ++s_dropped_count;
}

void UartStream_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* PA9=USART1_TX, PA10=USART1_RX (RX unused). */
    gpio.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* USART1 sits on APB2. HAL_RCC_GetPCLK2Freq() returns the APB2 clock
     * regardless of the specific system clock configuration. */
    const uint32_t pclk = HAL_RCC_GetPCLK2Freq();
    /* OVER8=0, so BRR = fCK / baud (rounded). Standard 16x oversampling. */
    const uint32_t usartdiv = (pclk + (UART_BAUD / 2U)) / UART_BAUD;
    USART1->BRR = usartdiv;
    USART1->CR2 = 0U;                                  /* 1 stop bit */
    USART1->CR3 = 0U;                                  /* no flow control */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;         /* 8N1, TX enabled */

    /* DMA2 Stream7 Channel4 = USART1_TX. */
    hdma_usart1_tx.Instance                 = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&hdma_usart1_tx);

    hdma_usart1_tx.XferCpltCallback  = UartStream_DmaTxCplt;
    hdma_usart1_tx.XferErrorCallback = UartStream_DmaTxError;

    /* IRQ priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
     * (default 5) to remain compatible with FreeRTOS API calls in callbacks. */
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

bool UartStream_SendSpO2(int16_t hr_bpm,
                         int16_t spo2_percent,
                         bool    low_spo2,
                         bool    abnormal_hr,
                         uint32_t timestamp_ms)
{
    if (s_tx_busy)
    {
        ++s_dropped_count;
        return false;
    }

    unsigned flags = 0U;
    if (low_spo2)    { flags |= 0x1U; }
    if (abnormal_hr) { flags |= 0x2U; }

    int n = snprintf(s_tx_buffer, sizeof(s_tx_buffer),
                     "$SPO2,%lu,%d,%d,%u\r\n",
                     (unsigned long)timestamp_ms,
                     (int)hr_bpm,
                     (int)spo2_percent,
                     flags);
    if (n <= 0)
    {
        return false;
    }
    if ((size_t)n >= sizeof(s_tx_buffer))
    {
        n = (int)sizeof(s_tx_buffer) - 1;
    }

    s_tx_busy = true;

    /* Clear TC before starting a new frame. */
    USART1->SR &= ~USART_SR_TC;

    if (HAL_DMA_Start_IT(&hdma_usart1_tx,
                         (uint32_t)(uintptr_t)s_tx_buffer,
                         (uint32_t)(uintptr_t)&USART1->DR,
                         (uint32_t)n) != HAL_OK)
    {
        s_tx_busy = false;
        ++s_dropped_count;
        return false;
    }
    USART1->CR3 |= USART_CR3_DMAT;
    return true;
}
