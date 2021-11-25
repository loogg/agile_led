/**
 * @file    agile_led.c
 * @brief   Agile Led 软件包源文件
 * @author  马龙伟 (2544047213@qq.com)
 * @version 1.1
 * @date    2021-11-24
 *
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 Ma Longwei.
 * All rights reserved.</center></h2>
 *
 */

#include <agile_led.h>
#include <stdlib.h>
#include <string.h>

/** @defgroup RT_Thread_DBG_Configuration RT-Thread DBG Configuration
 * @{
 */

/** @name RT-Thread DBG 功能配置
 * @{
 */
#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "agile_led"
#ifdef PKG_AGILE_LED_DEBUG
#define DBG_LEVEL DBG_LOG
#else
#define DBG_LEVEL DBG_INFO
#endif
#include <rtdbg.h>
/**
 * @}
 */

/**
 * @}
 */

/** @defgroup AGILE_LED_Configuration Agile Led Configuration
 * @{
 */

/** @name Agile Led 配置
 * @{
 */
#ifndef PKG_AGILE_LED_THREAD_STACK_SIZE
#define PKG_AGILE_LED_THREAD_STACK_SIZE 256 /**< Agile Led 线程堆栈大小 */
#endif

#ifndef PKG_AGILE_LED_THREAD_PRIORITY
#define PKG_AGILE_LED_THREAD_PRIORITY RT_THREAD_PRIORITY_MAX - 4 /**< Agile Led 线程优先级 */
#endif
/**
 * @}
 */

/**
 * @}
 */

/** @defgroup AGILE_LED_Private_Variables Agile Led Private Variables
 * @{
 */
static rt_slist_t agile_led_list = RT_SLIST_OBJECT_INIT(agile_led_list); /**< Agile Led 链表头节点 */
static rt_mutex_t lock_mtx = RT_NULL;                                    /**< Agile Led 互斥锁 */
static uint8_t is_initialized = 0;                                       /**< Agile Led 初始化完成标志 */
/**
 * @}
 */

/** @defgroup AGILE_LED_Private_Functions Agile Led Private Functions
 * @{
 */
static void agile_led_default_compelete_callback(agile_led_t *led);
static int agile_led_get_light_arr(agile_led_t *led, const char *light_mode);
static void led_process(void *parameter);
static int agile_led_init(void);
/**
 * @}
 */

/**
 * @brief   Agile Led 对象默认操作完成回调函数
 * @param   led Agile Led 对象指针
 */
static void agile_led_default_compelete_callback(agile_led_t *led)
{
    RT_ASSERT(led);
    LOG_D("led pin:%d compeleted.", led->pin);
}

/**
 * @brief   获取 Agile Led 对象闪烁模式
 * @param   led Agile Led 对象指针
 * @param   light_mode 闪烁模式字符串
 * @return
 * - RT_EOK:成功
 * - !=RT_EOK:异常
 */
static int agile_led_get_light_arr(agile_led_t *led, const char *light_mode)
{
    RT_ASSERT(led);
    RT_ASSERT(led->light_arr == RT_NULL);
    RT_ASSERT(led->arr_num == 0);
    const char *ptr = light_mode;
    while (*ptr) {
        if (*ptr == ',')
            led->arr_num++;
        ptr++;
    }
    if (*(ptr - 1) != ',')
        led->arr_num++;

    if (led->arr_num == 0)
        return -RT_ERROR;

    led->light_arr = (uint32_t *)rt_malloc(led->arr_num * sizeof(uint32_t));
    if (led->light_arr == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(led->light_arr, 0, led->arr_num * sizeof(uint32_t));
    ptr = light_mode;
    for (uint32_t i = 0; i < led->arr_num; i++) {
        led->light_arr[i] = atoi(ptr);
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }
    return RT_EOK;
}

/**
 * @brief   创建 Agile Led 对象
 * @param   pin 控制 led 的引脚
 * @param   active_logic led 有效电平 (PIN_HIGH/PIN_LOW)
 * @param   light_mode 闪烁模式字符串
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return
 * - !=RT_NULL:Agile Led 对象指针
 * - RT_NULL:异常
 */
agile_led_t *agile_led_create(uint32_t pin, uint32_t active_logic, const char *light_mode, int32_t loop_cnt)
{
    if (!is_initialized) {
        LOG_E("Agile led haven't initialized!");
        return RT_NULL;
    }
    agile_led_t *led = (agile_led_t *)rt_malloc(sizeof(agile_led_t));
    if (led == RT_NULL)
        return RT_NULL;
    led->active = 0;
    led->pin = pin;
    led->active_logic = active_logic;
    led->light_arr = RT_NULL;
    led->arr_num = 0;
    led->arr_index = 0;
    if (light_mode) {
        if (agile_led_get_light_arr(led, light_mode) < 0) {
            rt_free(led);
            return RT_NULL;
        }
    }
    led->loop_init = loop_cnt;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    led->compelete = agile_led_default_compelete_callback;
    rt_slist_init(&(led->slist));

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, !active_logic);
    return led;
}

/**
 * @brief   删除 Agile Led 对象
 * @param   led Agile Led 对象指针
 * @return
 * - RT_EOK:成功
 */
int agile_led_delete(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    rt_slist_remove(&(agile_led_list), &(led->slist));
    led->slist.next = RT_NULL;
    rt_mutex_release(lock_mtx);
    if (led->light_arr) {
        rt_free(led->light_arr);
        led->light_arr = RT_NULL;
    }
    rt_free(led);
    return RT_EOK;
}

/**
 * @brief   启动 Agile Led 对象,根据设置的模式执行动作
 * @param   led Agile Led 对象指针
 * @return
 * - RT_EOK:成功
 * - !=RT_OK:异常
 */
int agile_led_start(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    if (led->active) {
        rt_mutex_release(lock_mtx);
        return -RT_ERROR;
    }
    if ((led->light_arr == RT_NULL) || (led->arr_num == 0)) {
        rt_mutex_release(lock_mtx);
        return -RT_ERROR;
    }
    led->arr_index = 0;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    rt_slist_append(&(agile_led_list), &(led->slist));
    led->active = 1;
    rt_mutex_release(lock_mtx);
    return RT_EOK;
}

/**
 * @brief   停止 Agile Led 对象
 * @param   led Agile Led 对象指针
 * @return
 * - RT_EOK:成功
 */
int agile_led_stop(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    if (!led->active) {
        rt_mutex_release(lock_mtx);
        return RT_EOK;
    }
    rt_slist_remove(&(agile_led_list), &(led->slist));
    led->slist.next = RT_NULL;
    led->active = 0;
    rt_mutex_release(lock_mtx);
    return RT_EOK;
}

/**
 * @brief   设置 Agile Led 对象的模式
 * @param   led Agile Led 对象指针
 * @param   light_mode 闪烁模式字符串
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return
 * - RT_EOK:成功
 * - !=RT_EOK:异常
 */
int agile_led_set_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);

    if (light_mode) {
        if (led->light_arr) {
            rt_free(led->light_arr);
            led->light_arr = RT_NULL;
        }
        led->arr_num = 0;
        if (agile_led_get_light_arr(led, light_mode) < 0) {
            agile_led_stop(led);
            rt_mutex_release(lock_mtx);
            return -RT_ERROR;
        }
    }
    led->loop_init = loop_cnt;
    led->arr_index = 0;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    rt_mutex_release(lock_mtx);
    return RT_EOK;
}

/**
 * @brief   设置 Agile Led 对象操作完成的回调函数
 * @param   led Agile Led 对象指针
 * @param   compelete 操作完成回调函数
 * @return
 * - RT_EOK:成功
 */
int agile_led_set_compelete_callback(agile_led_t *led, void (*compelete)(agile_led_t *led))
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    led->compelete = compelete;
    rt_mutex_release(lock_mtx);
    return RT_EOK;
}

/**
 * @brief   Agile Led 对象电平翻转
 * @param   led Agile Led 对象指针
 */
void agile_led_toggle(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, !rt_pin_read(led->pin));
}

/**
 * @brief   Agile Led 对象亮
 * @param   led Agile Led 对象指针
 */
void agile_led_on(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, led->active_logic);
}

/**
 * @brief   Agile Led 对象灭
 * @param   led Agile Led 对象指针
 */
void agile_led_off(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, !led->active_logic);
}

/**
 * @brief   Agile Led 线程
 * @param   parameter 线程参数
 */
static void led_process(void *parameter)
{
    rt_slist_t *node;
    while (1) {
        rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
        rt_slist_for_each(node, &(agile_led_list))
        {
            agile_led_t *led = rt_slist_entry(node, agile_led_t, slist);
            if (led->loop_cnt == 0) {
                agile_led_stop(led);
                if (led->compelete) {
                    led->compelete(led);
                }
                node = &agile_led_list;
                continue;
            }
        __repeat:
            if ((rt_tick_get() - led->tick_timeout) < (RT_TICK_MAX / 2)) {
                if (led->arr_index < led->arr_num) {
                    if (led->light_arr[led->arr_index] == 0) {
                        led->arr_index++;
                        goto __repeat;
                    }
                    if (led->arr_index % 2) {
                        agile_led_off(led);
                    } else {
                        agile_led_on(led);
                    }
                    led->tick_timeout = rt_tick_get() + rt_tick_from_millisecond(led->light_arr[led->arr_index]);
                    led->arr_index++;
                } else {
                    led->arr_index = 0;
                    if (led->loop_cnt > 0)
                        led->loop_cnt--;
                }
            }
        }
        rt_mutex_release(lock_mtx);
        rt_thread_mdelay(5);
    }
}

/**
 * @brief   Agile Led 自动初始化
 * @return
 * - RT_EOK:成功
 * - !=RT_EOK:失败
 */
static int agile_led_init(void)
{
    rt_thread_t tid = RT_NULL;
    lock_mtx = rt_mutex_create("led_mtx", RT_IPC_FLAG_FIFO);
    if (lock_mtx == RT_NULL) {
        LOG_E("Agile led initialize failed! lock_mtx create failed!");
        return -RT_ENOMEM;
    }

    tid = rt_thread_create("agile_led", led_process, RT_NULL,
                           PKG_AGILE_LED_THREAD_STACK_SIZE, PKG_AGILE_LED_THREAD_PRIORITY, 100);
    if (tid == RT_NULL) {
        LOG_E("Agile led initialize failed! thread create failed!");
        rt_mutex_delete(lock_mtx);
        return -RT_ENOMEM;
    }
    rt_thread_startup(tid);
    is_initialized = 1;
    return RT_EOK;
}

/** @addtogroup RT_Thread_Auto_Init_APP
 * @{
 */
INIT_APP_EXPORT(agile_led_init);
/**
 * @}
 */
