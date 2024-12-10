/**
 * @file hal_common_define.h
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-08-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once


#define HAL_PIN_PWR_HOLDING     46
#ifdef CONFIG_USING_SIMULATOR
#define HAL_PIN_PWR_WAKE_UP     14
#else
#define HAL_PIN_PWR_WAKE_UP     42
#endif

#ifdef CONFIG_USING_SIMULATOR
#define HAL_PIN_ENCODER_A       12
#define HAL_PIN_ENCODER_B       11
#else
#define HAL_PIN_ENCODER_A       41
#define HAL_PIN_ENCODER_B       40
#endif

#define HAL_PIN_TP_I2C_SCL      12
#define HAL_PIN_TP_I2C_SDA      11
#define HAL_PIN_TP_INT          14

#define HAL_PIN_GROVE_I2C_SCL   15
#define HAL_PIN_GROVE_I2C_SDA   13

#define HAL_PIN_BUZZER          3
