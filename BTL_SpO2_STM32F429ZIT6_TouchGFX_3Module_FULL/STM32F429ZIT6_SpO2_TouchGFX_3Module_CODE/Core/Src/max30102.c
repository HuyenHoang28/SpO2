#include "max30102.h"

#define REG_INTR_STATUS_1   0x00
#define REG_INTR_STATUS_2   0x01
#define REG_INTR_ENABLE_1   0x02
#define REG_INTR_ENABLE_2   0x03
#define REG_FIFO_WR_PTR     0x04
#define REG_OVF_COUNTER     0x05
#define REG_FIFO_RD_PTR     0x06
#define REG_FIFO_DATA       0x07
#define REG_FIFO_CONFIG     0x08
#define REG_MODE_CONFIG     0x09
#define REG_SPO2_CONFIG     0x0A
#define REG_LED1_PA         0x0C   /* Red LED */
#define REG_LED2_PA         0x0D   /* IR LED */
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_PART_ID         0xFF

static MAX30102_Status_t write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    if (HAL_I2C_Mem_Write(hi2c, MAX30102_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100) != HAL_OK)
        return MAX30102_ERROR;
    return MAX30102_OK;
}

static MAX30102_Status_t read_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *value)
{
    if (HAL_I2C_Mem_Read(hi2c, MAX30102_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, value, 1, 100) != HAL_OK)
        return MAX30102_ERROR;
    return MAX30102_OK;
}

MAX30102_Status_t MAX30102_ReadPartID(I2C_HandleTypeDef *hi2c, uint8_t *part_id)
{
    return read_reg(hi2c, REG_PART_ID, part_id);
}

MAX30102_Status_t MAX30102_Reset(I2C_HandleTypeDef *hi2c)
{
    if (write_reg(hi2c, REG_MODE_CONFIG, 0x40) != MAX30102_OK) return MAX30102_ERROR;
    HAL_Delay(100);
    return MAX30102_OK;
}

MAX30102_Status_t MAX30102_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t part_id = 0;
    if (MAX30102_ReadPartID(hi2c, &part_id) != MAX30102_OK) return MAX30102_NOT_FOUND;
    if (part_id != MAX30102_PART_ID_VALUE) return MAX30102_NOT_FOUND;

    if (MAX30102_Reset(hi2c) != MAX30102_OK) return MAX30102_ERROR;

    /* Clear interrupt status */
    read_reg(hi2c, REG_INTR_STATUS_1, &part_id);
    read_reg(hi2c, REG_INTR_STATUS_2, &part_id);

    /* FIFO reset */
    write_reg(hi2c, REG_FIFO_WR_PTR, 0x00);
    write_reg(hi2c, REG_OVF_COUNTER, 0x00);
    write_reg(hi2c, REG_FIFO_RD_PTR, 0x00);

    /* FIFO config: sample avg = 4, FIFO rollover enabled, almost full = 17 */
    write_reg(hi2c, REG_FIFO_CONFIG, 0x4F);

    /* SpO2 mode */
    write_reg(hi2c, REG_MODE_CONFIG, 0x03);

    /* SpO2 config: ADC range 4096nA, sample rate 100Hz, pulse width 411us, 18-bit */
    write_reg(hi2c, REG_SPO2_CONFIG, 0x27);

    /* LED pulse amplitude. Tune from 0x1F to 0x7F if signal weak/saturated. */
    write_reg(hi2c, REG_LED1_PA, 0x24); /* Red */
    write_reg(hi2c, REG_LED2_PA, 0x24); /* IR */

    /* Enable interrupt for new FIFO data, optional */
    write_reg(hi2c, REG_INTR_ENABLE_1, 0xC0);
    write_reg(hi2c, REG_INTR_ENABLE_2, 0x00);

    return MAX30102_OK;
}

MAX30102_Status_t MAX30102_SetLedAmplitude(I2C_HandleTypeDef *hi2c, uint8_t red_pa, uint8_t ir_pa)
{
    if (write_reg(hi2c, REG_LED1_PA, red_pa) != MAX30102_OK) return MAX30102_ERROR;
    if (write_reg(hi2c, REG_LED2_PA, ir_pa) != MAX30102_OK) return MAX30102_ERROR;
    return MAX30102_OK;
}

MAX30102_Status_t MAX30102_Shutdown(I2C_HandleTypeDef *hi2c, uint8_t enable)
{
    return write_reg(hi2c, REG_MODE_CONFIG, enable ? 0x80 : 0x03);
}

MAX30102_Status_t MAX30102_ReadFIFO(I2C_HandleTypeDef *hi2c, MAX30102_Sample_t *sample)
{
    uint8_t data[6];
    if (HAL_I2C_Mem_Read(hi2c, MAX30102_I2C_ADDR, REG_FIFO_DATA, I2C_MEMADD_SIZE_8BIT, data, 6, 100) != HAL_OK)
        return MAX30102_ERROR;

    uint32_t red = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    uint32_t ir  = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    sample->red = red & 0x03FFFF;
    sample->ir  = ir  & 0x03FFFF;
    return MAX30102_OK;
}
