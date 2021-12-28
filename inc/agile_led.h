/**
 * @file    agile_led.h
 * @brief   Agile Led 软件包头文件
 * @author  马龙伟 (2544047213@qq.com)
 * @version 1.1.1
 * @date    2021-12-28
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#ifndef __PKG_AGILE_LED_H
#define __PKG_AGILE_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

/** @defgroup AGILE_LED_Exported_Types Agile Led Exported Types
 * @{
 */
#define AGILE_LED_TYPE_DYNAMIC 0x00 /**< 动态类型 */
#define AGILE_LED_TYPE_STATIC  0x01 /**< 静态类型 */

typedef struct agile_led agile_led_t; /**< Agile Led 结构体 */

/**
 * @brief   Agile Led 结构体
 */
struct agile_led {
    uint8_t type;                        /**< 对象类型 (静态或动态) */
    uint8_t active;                      /**< 激活标志 */
    uint32_t pin;                        /**< 控制引脚 */
    uint32_t active_logic;               /**< 有效电平 (PIN_HIGH/PIN_LOW) */
    const uint32_t *light_arr;           /**< 闪烁数组 */
    uint32_t arr_num;                    /**< 数组元素数目 */
    uint32_t arr_index;                  /**< 数组索引 */
    int32_t loop_init;                   /**< 循环次数 */
    int32_t loop_cnt;                    /**< 循环次数计数 */
    rt_tick_t tick_timeout;              /**< 超时时间 */
    void (*compelete)(agile_led_t *led); /**< 操作完成回调函数 */
    rt_slist_t slist;                    /**< 单向链表节点 */
};
/**
 * @}
 */

/** @addtogroup AGILE_LED_Exported_Functions
 * @{
 */
#ifdef RT_USING_HEAP
agile_led_t *agile_led_create(uint32_t pin, uint32_t active_logic, const char *light_mode, int32_t loop_cnt);
int agile_led_delete(agile_led_t *led);
int agile_led_dynamic_change_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt);
int agile_led_set_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt);
#endif

int agile_led_init(agile_led_t *led, uint32_t pin, uint32_t active_logic, const uint32_t *light_array, int array_size, int32_t loop_cnt);
int agile_led_static_change_light_mode(agile_led_t *led, const uint32_t *light_array, int array_size, int32_t loop_cnt);
int agile_led_start(agile_led_t *led);
int agile_led_stop(agile_led_t *led);
int agile_led_set_compelete_callback(agile_led_t *led, void (*compelete)(agile_led_t *led));
void agile_led_toggle(agile_led_t *led);
void agile_led_on(agile_led_t *led);
void agile_led_off(agile_led_t *led);
void agile_led_process(void);
void agile_led_env_init(void);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __PKG_AGILE_LED_H */
