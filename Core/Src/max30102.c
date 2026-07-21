#include "max30102.h"

#include <stddef.h>

#define REG_INTR_STATUS_1      0x00U
#define REG_INTR_STATUS_2      0x01U
#define REG_INTR_ENABLE_1      0x02U
#define REG_INTR_ENABLE_2      0x03U
#define REG_FIFO_WR_PTR        0x04U
#define REG_OVF_COUNTER        0x05U
#define REG_FIFO_RD_PTR        0x06U
#define REG_FIFO_DATA          0x07U
#define REG_FIFO_CONFIG        0x08U
#define REG_MODE_CONFIG        0x09U
#define REG_SPO2_CONFIG        0x0AU
#define REG_LED1_PA            0x0CU
#define REG_LED2_PA            0x0DU
#define REG_REV_ID             0xFEU
#define REG_PART_ID            0xFFU

#define MODE_SHUTDOWN_BIT      0x80U
#define MODE_RESET_BIT         0x40U
#define MODE_SPO2              0x03U

#define I2C_TIMEOUT_MS         100U
#define I2C_TRANSACTION_RETRIES 3U
#define RESET_TIMEOUT_MS       150U
#define DEVICE_READY_TRIALS    5U
#define DEVICE_READY_TIMEOUT   50U
/* SparkFun/Maxim reference profile: 60/255 of the 50 mA range, about
 * 12 mA. Higher values can saturate the ADC when a finger is present. */
#define DEFAULT_LED_CURRENT    0x3CU

static MAX30102_Status write_reg(MAX30102_Handle *dev,
                                 uint8_t reg,
                                 uint8_t value)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    for (uint8_t attempt = 0U; attempt < I2C_TRANSACTION_RETRIES; ++attempt) {
        if (HAL_I2C_Mem_Write(dev->hi2c,
                              dev->address,
                              reg,
                              I2C_MEMADD_SIZE_8BIT,
                              &value,
                              1U,
                              I2C_TIMEOUT_MS) == HAL_OK) {
            return MAX30102_OK;
        }
        HAL_Delay(1U);
    }
    return MAX30102_ERROR_I2C;
}

static MAX30102_Status read_regs(MAX30102_Handle *dev,
                                 uint8_t reg,
                                 uint8_t *data,
                                 uint16_t length)
{
    if ((dev == NULL) || (dev->hi2c == NULL) ||
        (data == NULL) || (length == 0U)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    for (uint8_t attempt = 0U; attempt < I2C_TRANSACTION_RETRIES; ++attempt) {
        if (HAL_I2C_Mem_Read(dev->hi2c,
                             dev->address,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             data,
                             length,
                             I2C_TIMEOUT_MS) == HAL_OK) {
            return MAX30102_OK;
        }
        HAL_Delay(1U);
    }
    return MAX30102_ERROR_I2C;
}

MAX30102_Status MAX30102_ReadPartId(MAX30102_Handle *dev,
                                    uint8_t *part_id)
{
    MAX30102_Status status;

    if (part_id == NULL) {
        return MAX30102_ERROR_ARGUMENT;
    }

    status = read_regs(dev, REG_PART_ID, part_id, 1U);
    if (status == MAX30102_OK) {
        dev->part_id = *part_id;
    }
    return status;
}

MAX30102_Status MAX30102_Reset(MAX30102_Handle *dev)
{
    uint8_t mode = MODE_RESET_BIT;
    uint32_t start_tick;
    MAX30102_Status status;

    status = write_reg(dev, REG_MODE_CONFIG, mode);
    if (status != MAX30102_OK) {
        return status;
    }

    start_tick = HAL_GetTick();
    do {
        status = read_regs(dev, REG_MODE_CONFIG, &mode, 1U);
        if (status != MAX30102_OK) {
            return status;
        }
        if ((mode & MODE_RESET_BIT) == 0U) {
            return MAX30102_OK;
        }
        HAL_Delay(1U);
    } while ((HAL_GetTick() - start_tick) < RESET_TIMEOUT_MS);

    return MAX30102_ERROR_TIMEOUT;
}

MAX30102_Status MAX30102_ClearFIFO(MAX30102_Handle *dev)
{
    uint8_t values[3] = {0U, 0U, 0U};

    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    for (uint8_t attempt = 0U; attempt < I2C_TRANSACTION_RETRIES; ++attempt) {
        if (HAL_I2C_Mem_Write(dev->hi2c,
                              dev->address,
                              REG_FIFO_WR_PTR,
                              I2C_MEMADD_SIZE_8BIT,
                              values,
                              sizeof(values),
                              I2C_TIMEOUT_MS) == HAL_OK) {
            return MAX30102_OK;
        }
        HAL_Delay(1U);
    }
    return MAX30102_ERROR_I2C;
}

MAX30102_Status MAX30102_DiagnoseBus(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL) {
        return MAX30102_ERROR_ARGUMENT;
    }

    /* PA8/PC9 are open-drain I2C3 pins. Reading the input level while they
       are in alternate-function mode is valid and immediately identifies a
       line held low by a device, short circuit, or failed level shifter. */
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == GPIO_PIN_RESET) {
        return MAX30102_ERROR_SDA_STUCK_LOW;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET) {
        return MAX30102_ERROR_SCL_STUCK_LOW;
    }

    /* If the MAX address ACKs, let normal initialization determine whether
       the Part ID and register transactions are valid. */
    if (HAL_I2C_IsDeviceReady(hi2c,
                              MAX30102_I2C_ADDRESS_HAL,
                              3U,
                              30U) == HAL_OK) {
        return MAX30102_OK;
    }

    /* The STMPE811 touch controller is physically present on the Discovery
       board at 0x41. The RTC is normally at 0x68. If either one ACKs, the
       selected I2C3 peripheral and PA8/PC9 bus are proven to be working, but
       no device is responding at 0x57. */
    if ((HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(0x41U << 1U), 2U, 20U) == HAL_OK) ||
        (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(0x68U << 1U), 2U, 20U) == HAL_OK)) {
        return MAX30102_ERROR_NO_57_ON_BUS;
    }

    return MAX30102_ERROR_BUS_EMPTY;
}

MAX30102_Status MAX30102_InitDefault(MAX30102_Handle *dev,
                                      I2C_HandleTypeDef *hi2c)
{
    uint8_t part_id = 0U;
    uint8_t clear_status[2];
    MAX30102_Status status;

    if ((dev == NULL) || (hi2c == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    dev->hi2c = hi2c;
    dev->address = MAX30102_I2C_ADDRESS_HAL;
    dev->part_id = 0U;
    dev->initialized = false;

    if (HAL_I2C_IsDeviceReady(hi2c,
                              dev->address,
                              DEVICE_READY_TRIALS,
                              DEVICE_READY_TIMEOUT) != HAL_OK) {
        return MAX30102_DiagnoseBus(hi2c);
    }

    status = MAX30102_ReadPartId(dev, &part_id);
    if (status != MAX30102_OK) {
        return status;
    }
    if (part_id != MAX30102_PART_ID_EXPECTED) {
        return MAX30102_ERROR_BAD_PART_ID;
    }

    status = MAX30102_Reset(dev);
    if (status != MAX30102_OK) {
        return status;
    }

    /* Some low-cost breakouts need a short settling time after reset. */
    HAL_Delay(10U);

    /* Disable interrupts. This project polls the FIFO from Model::tick(). */
    if (write_reg(dev, REG_INTR_ENABLE_1, 0x00U) != MAX30102_OK ||
        write_reg(dev, REG_INTR_ENABLE_2, 0x00U) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }

    /*
     * SMP_AVE = 000: NO averaging (100 sps real-time).
     * FIFO_ROLLOVER_EN = 1: preserve newest data if GUI is briefly busy.
     * FIFO_A_FULL = 0x0F.
     */
    status = write_reg(dev, REG_FIFO_CONFIG, 0x1FU);
    if (status != MAX30102_OK) {
        return status;
    }

    /* Start SpO2 conversion before programming the optical channel. This
       ordering follows the proven MAX3010x reference sequence and is more
       tolerant of low-cost clone breakout boards. */
    status = write_reg(dev, REG_MODE_CONFIG, MODE_SPO2);
    if (status != MAX30102_OK) {
        return status;
    }

    /* ADC range 4096 nA, 100 sps, 411 us / 18-bit. */
    status = write_reg(dev, REG_SPO2_CONFIG, 0x27U);
    if (status != MAX30102_OK) {
        return status;
    }

    /* Reference LED current. The application can raise/lower it at runtime
       when the raw signal is too weak or near saturation. */
    status = MAX30102_SetLedCurrent(dev,
                                    DEFAULT_LED_CURRENT,
                                    DEFAULT_LED_CURRENT);
    if (status != MAX30102_OK) {
        return status;
    }

    status = MAX30102_ClearFIFO(dev);
    if (status != MAX30102_OK) {
        return status;
    }

    /* Give 4-sample averaging enough time to commit more than one FIFO item. */
    HAL_Delay(120U);

    /* Read-to-clear any power-ready/data-ready flags. */
    (void)read_regs(dev, REG_INTR_STATUS_1, clear_status, sizeof(clear_status));

    dev->initialized = true;
    return MAX30102_OK;
}

MAX30102_Status MAX30102_SetLedCurrent(MAX30102_Handle *dev,
                                       uint8_t red_current,
                                       uint8_t ir_current)
{
    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    if ((write_reg(dev, REG_LED1_PA, red_current) != MAX30102_OK) ||
        (write_reg(dev, REG_LED2_PA, ir_current) != MAX30102_OK)) {
        return MAX30102_ERROR_I2C;
    }

    return MAX30102_OK;
}

MAX30102_Status MAX30102_Shutdown(MAX30102_Handle *dev, bool enable)
{
    uint8_t mode;
    MAX30102_Status status = read_regs(dev, REG_MODE_CONFIG, &mode, 1U);

    if (status != MAX30102_OK) {
        return status;
    }

    if (enable) {
        mode |= MODE_SHUTDOWN_BIT;
    } else {
        mode &= (uint8_t)~MODE_SHUTDOWN_BIT;
    }
    return write_reg(dev, REG_MODE_CONFIG, mode);
}

MAX30102_Status MAX30102_ReadAvailable(MAX30102_Handle *dev,
                                       MAX30102_Sample *samples,
                                       uint8_t capacity,
                                       uint8_t *sample_count)
{
    uint8_t pointers[3];
    uint8_t fifo_bytes[MAX30102_FIFO_DEPTH * 6U];
    uint8_t available;
    uint8_t to_read;
    uint8_t i;
    MAX30102_Status status;

    if ((dev == NULL) || (samples == NULL) ||
        (sample_count == NULL) || (capacity == 0U)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    *sample_count = 0U;
    status = read_regs(dev, REG_FIFO_WR_PTR, pointers, sizeof(pointers));
    if (status != MAX30102_OK) {
        return status;
    }

    if ((pointers[0] & 0x1FU) == (pointers[2] & 0x1FU)) {
        available = ((pointers[1] & 0x1FU) != 0U) ? MAX30102_FIFO_DEPTH : 0U;
    } else {
        available = (uint8_t)(((pointers[0] & 0x1FU) -
                               (pointers[2] & 0x1FU)) & 0x1FU);
    }

    to_read = (available < capacity) ? available : capacity;
    if (to_read == 0U) {
        return MAX30102_OK;
    }

    status = read_regs(dev, REG_FIFO_DATA, fifo_bytes,
                       (uint16_t)to_read * 6U);
    if (status != MAX30102_OK) {
        return status;
    }

    for (i = 0U; i < to_read; ++i) {
        const uint8_t *p = &fifo_bytes[i * 6U];
        samples[i].red = ((((uint32_t)p[0] << 16U) |
                           ((uint32_t)p[1] << 8U) |
                           (uint32_t)p[2]) & 0x3FFFFU);
        samples[i].ir = ((((uint32_t)p[3] << 16U) |
                          ((uint32_t)p[4] << 8U) |
                          (uint32_t)p[5]) & 0x3FFFFU);
    }

    *sample_count = to_read;
    return MAX30102_OK;
}


MAX30102_Status MAX30102_ReadDiagnostic(MAX30102_Handle *dev,
                                         MAX30102_Diagnostic *diagnostic)
{
    uint8_t pointers[3];
    uint8_t config[2];
    uint8_t led[2];
    MAX30102_Status status;

    if ((dev == NULL) || (diagnostic == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    {
        uint8_t intr[2];
        status = read_regs(dev, REG_INTR_STATUS_1, intr, 2U);
        if (status != MAX30102_OK) {
            return status;
        }
        diagnostic->int_status_1 = intr[0];
        diagnostic->int_status_2 = intr[1];
    }

    status = read_regs(dev, REG_FIFO_WR_PTR, pointers, 3U);
    if (status != MAX30102_OK) {
        return status;
    }
    diagnostic->fifo_write_pointer = pointers[0] & 0x1FU;
    diagnostic->fifo_overflow_counter = pointers[1] & 0x1FU;
    diagnostic->fifo_read_pointer = pointers[2] & 0x1FU;

    status = read_regs(dev, REG_FIFO_CONFIG, &diagnostic->fifo_config, 1U);
    if (status != MAX30102_OK) {
        return status;
    }
    status = read_regs(dev, REG_MODE_CONFIG, config, 2U);
    if (status != MAX30102_OK) {
        return status;
    }
    diagnostic->mode_config = config[0];
    diagnostic->spo2_config = config[1];

    status = read_regs(dev, REG_LED1_PA, led, 2U);
    if (status != MAX30102_OK) {
        return status;
    }
    diagnostic->led1_current = led[0];
    diagnostic->led2_current = led[1];
    return MAX30102_OK;
}

MAX30102_Status MAX30102_ForceMeasurement(MAX30102_Handle *dev,
                                           uint8_t led_current)
{
    MAX30102_Status status;
    uint8_t clear_status[2];

    if ((dev == NULL) || (dev->hi2c == NULL)) {
        return MAX30102_ERROR_ARGUMENT;
    }

    /* Explicitly leave shutdown/reset, then replay a minimal known-good
       SpO2 configuration. This repairs modules whose MODE register accepted
       the I2C write but whose conversion engine did not start. */
    status = write_reg(dev, REG_MODE_CONFIG, 0x00U);
    if (status != MAX30102_OK) return status;
    HAL_Delay(2U);

    if (write_reg(dev, REG_INTR_ENABLE_1, 0x00U) != MAX30102_OK ||
        write_reg(dev, REG_INTR_ENABLE_2, 0x00U) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (write_reg(dev, REG_FIFO_CONFIG, 0x1FU) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (write_reg(dev, REG_MODE_CONFIG, MODE_SPO2) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (write_reg(dev, REG_SPO2_CONFIG, 0x27U) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (MAX30102_SetLedCurrent(dev, led_current, led_current) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (MAX30102_ClearFIFO(dev) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }

    (void)read_regs(dev, REG_INTR_STATUS_1, clear_status, sizeof(clear_status));
    HAL_Delay(160U);
    return MAX30102_OK;
}
