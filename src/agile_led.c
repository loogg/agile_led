/**
 * @file    agile_led.c
 * @brief   Agile Led 软件包源文件
 * @author  马龙伟 (2544047213@qq.com)
 * @version 1.1.1
 * @date    2021-12-28
 *
 @verbatim
    使用：
    如果未使能 PKG_AGILE_LED_USING_THREAD_AUTO_INIT:
    1. agile_led_env_init 初始化环境
    2. 创建一个线程，周期调用 agile_led_process，建议周期时间不要太长

    - agile_led_create / agile_led_init 创建 / 初始化对象
    - agile_led_start 启动运行
    - agile_led_dynamic_change_light_mode / agile_led_static_change_light_mode 更改模式
      该操作也可在启动运行前执行
    - 如果需要感知对象执行结束，agile_led_set_compelete_callback 设置回调函数
    - 过程中需要强制停止，使用 agile_led_stop
    - agile_led_on / agile_led_off / agile_led_toggle 单独操作对象

 @endverbatim
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

#ifdef PKG_AGILE_LED_USING_THREAD_AUTO_INIT

/** @defgroup AGILE_LED_Thread_Auto_Init Agile Led Thread Auto Init
 * @{
 */

/** @defgroup AGILE_LED_Thread_Auto_Init_Configuration Agile Led Thread Auto Init Configuration
 * @{
 */

/** @name Agile Led 自动初始化线程配置
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

/**
 * @}
 */

#endif /* PKG_AGILE_LED_USING_THREAD_AUTO_INIT */

/** @defgroup AGILE_LED_Private_Variables Agile Led Private Variables
 * @{
 */
ALIGN(RT_ALIGN_SIZE)
static rt_slist_t _slist_head = RT_SLIST_OBJECT_INIT(_slist_head); /**< Agile Led 链表头节点 */
static struct rt_mutex _mtx;                                       /**< Agile Led 互斥锁 */
static uint8_t _is_init = 0;                                       /**< Agile Led 初始化完成标志 */

#ifdef PKG_AGILE_LED_USING_THREAD_AUTO_INIT
static struct rt_thread _thread;                               /**< Agile Led 线程控制块 */
static uint8_t _thread_stack[PKG_AGILE_LED_THREAD_STACK_SIZE]; /**< Agile Led 线程堆栈 */
#endif
/**
 * @}
 */

/** @defgroup AGILE_LED_Private_Functions Agile Led Private Functions
 * @{
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

#ifdef RT_USING_HEAP

/**
 * @brief   解析字符串获取 Agile Led 对象 light_arr 和 arr_num
 * @param   led Agile Led 对象指针
 * @param   light_mode 闪烁模式字符串
 @verbatim
    例子:
    "100,200,100,200"
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @return  RT_EOK:成功; !=RT_EOK:异常
 */
static int agile_led_get_light_arr(agile_led_t *led, const char *light_mode)
{
    RT_ASSERT(led);
    RT_ASSERT(led->type == AGILE_LED_TYPE_DYNAMIC);
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

    uint32_t *light_arr = rt_malloc(led->arr_num * sizeof(uint32_t));
    if (light_arr == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(light_arr, 0, led->arr_num * sizeof(uint32_t));

    ptr = light_mode;
    for (uint32_t i = 0; i < led->arr_num; i++) {
        light_arr[i] = atoi(ptr);
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }

    led->light_arr = light_arr;

    return RT_EOK;
}

#endif /* RT_USING_HEAP */

/**
 * @}
 */

/** @defgroup AGILE_LED_Exported_Functions Agile Led Exported Functions
 * @{
 */

#ifdef RT_USING_HEAP

/**
 * @brief   创建 Agile Led 对象
 * @param   pin 控制 led 的引脚
 * @param   active_logic led 有效电平 (PIN_HIGH/PIN_LOW)
 * @param   light_mode 闪烁模式字符串
 @verbatim
    例子:
    "100,200,100,200"
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return  !=RT_NULL:Agile Led 对象指针; RT_NULL:异常
 */
agile_led_t *agile_led_create(uint32_t pin, uint32_t active_logic, const char *light_mode, int32_t loop_cnt)
{
    if (!_is_init) {
        LOG_E("Please call agile_led_env_init first.");
        return RT_NULL;
    }

    agile_led_t *led = (agile_led_t *)rt_malloc(sizeof(agile_led_t));
    if (led == RT_NULL)
        return RT_NULL;

    led->type = AGILE_LED_TYPE_DYNAMIC;
    led->active = 0;
    led->pin = pin;
    led->active_logic = active_logic;
    led->light_arr = RT_NULL;
    led->arr_num = 0;
    led->arr_index = 0;
    if (light_mode) {
        if (agile_led_get_light_arr(led, light_mode) != RT_EOK) {
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
 * @return  RT_EOK:成功
 */
int agile_led_delete(agile_led_t *led)
{
    RT_ASSERT(led);
    RT_ASSERT(led->type == AGILE_LED_TYPE_DYNAMIC);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    rt_slist_remove(&_slist_head, &(led->slist));
    led->slist.next = RT_NULL;
    rt_mutex_release(&_mtx);

    if (led->light_arr) {
        rt_free((uint32_t *)led->light_arr);
        led->light_arr = RT_NULL;
    }
    rt_free(led);

    return RT_EOK;
}

/**
 * @brief   动态设置 Agile Led 对象的模式
 * @note    Agile Led 对象必须是使用 agile_led_create 创建的
 * @param   led Agile Led 对象指针
 * @param   light_mode 闪烁模式字符串
 @verbatim
    例子:
    "100,200,100,200"
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return  RT_EOK:成功; !=RT_EOK:异常
 */
int agile_led_dynamic_change_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt)
{
    RT_ASSERT(led);
    RT_ASSERT(led->type == AGILE_LED_TYPE_DYNAMIC);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    if (light_mode) {
        if (led->light_arr) {
            rt_free((uint32_t *)led->light_arr);
            led->light_arr = RT_NULL;
        }
        led->arr_num = 0;
        if (agile_led_get_light_arr(led, light_mode) != RT_EOK) {
            agile_led_stop(led);
            rt_mutex_release(&_mtx);
            return -RT_ERROR;
        }
    }
    led->loop_init = loop_cnt;
    led->arr_index = 0;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    rt_mutex_release(&_mtx);

    return RT_EOK;
}

/**
 * @brief   动态设置 Agile Led 对象的模式
 * @note    该 API 已被 agile_led_dynamic_change_light_mode 替代，目前版本仍兼容，但不保证后续版本不会抛弃
 * @param   led Agile Led 对象指针
 * @param   light_mode 闪烁模式字符串
 @verbatim
    例子:
    "100,200,100,200"
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return  RT_EOK:成功; !=RT_EOK:异常
 */
int agile_led_set_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt)
{
    return agile_led_dynamic_change_light_mode(led, light_mode, loop_cnt);
}

#endif /* RT_USING_HEAP */

/**
 * @brief   初始化 Agile Led 对象
 * @param   led Agile Led 对象指针
 * @param   pin 控制 led 的引脚
 * @param   active_logic led 有效电平 (PIN_HIGH/PIN_LOW)
 * @param   light_array 闪烁数组
 @verbatim
    例子:
    [100, 200, 100, 200]
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @param   array_size 闪烁数组数目
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return  RT_EOK:成功; !=RT_EOK:异常
 */
int agile_led_init(agile_led_t *led, uint32_t pin, uint32_t active_logic, const uint32_t *light_array, int array_size, int32_t loop_cnt)
{
    RT_ASSERT(led);

    if (!_is_init) {
        LOG_E("Please call agile_led_env_init first.");
        return -RT_ERROR;
    }

    led->type = AGILE_LED_TYPE_STATIC;
    led->active = 0;
    led->pin = pin;
    led->active_logic = active_logic;
    led->light_arr = light_array;
    led->arr_num = array_size;
    led->arr_index = 0;
    led->loop_init = loop_cnt;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    led->compelete = agile_led_default_compelete_callback;
    rt_slist_init(&(led->slist));

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, !active_logic);

    return RT_EOK;
}

/**
 * @brief   静态设置 Agile Led 对象的模式
 * @note    Agile Led 对象必须是静态的
 * @param   led Agile Led 对象指针
 * @param   light_array 闪烁数组
 @verbatim
    例子:
    [100, 200, 100, 200]
    只支持正整数，按照亮灭亮灭规律

 @endverbatim
 * @param   array_size 闪烁数组数目
 * @param   loop_cnt 循环次数 (负数为永久循环)
 * @return  RT_EOK:成功
 */
int agile_led_static_change_light_mode(agile_led_t *led, const uint32_t *light_array, int array_size, int32_t loop_cnt)
{
    RT_ASSERT(led);
    RT_ASSERT(led->type == AGILE_LED_TYPE_STATIC);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    led->light_arr = light_array;
    led->arr_num = array_size;
    led->arr_index = 0;
    led->loop_init = loop_cnt;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    rt_mutex_release(&_mtx);

    return RT_EOK;
}

/**
 * @brief   启动 Agile Led 对象,根据设置的模式执行动作
 * @param   led Agile Led 对象指针
 * @return  RT_EOK:成功; !=RT_OK:异常
 */
int agile_led_start(agile_led_t *led)
{
    RT_ASSERT(led);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    if (led->active) {
        rt_mutex_release(&_mtx);
        return -RT_ERROR;
    }
    if ((led->light_arr == RT_NULL) || (led->arr_num == 0)) {
        rt_mutex_release(&_mtx);
        return -RT_ERROR;
    }
    led->arr_index = 0;
    led->loop_cnt = led->loop_init;
    led->tick_timeout = rt_tick_get();
    rt_slist_append(&_slist_head, &(led->slist));
    led->active = 1;
    rt_mutex_release(&_mtx);

    return RT_EOK;
}

/**
 * @brief   停止 Agile Led 对象
 * @param   led Agile Led 对象指针
 * @return  RT_EOK:成功
 */
int agile_led_stop(agile_led_t *led)
{
    RT_ASSERT(led);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    if (!led->active) {
        rt_mutex_release(&_mtx);
        return RT_EOK;
    }
    rt_slist_remove(&_slist_head, &(led->slist));
    led->slist.next = RT_NULL;
    led->active = 0;
    rt_mutex_release(&_mtx);

    return RT_EOK;
}

/**
 * @brief   设置 Agile Led 对象操作完成的回调函数
 * @param   led Agile Led 对象指针
 * @param   compelete 操作完成回调函数
 * @return  RT_EOK:成功
 */
int agile_led_set_compelete_callback(agile_led_t *led, void (*compelete)(agile_led_t *led))
{
    RT_ASSERT(led);

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    led->compelete = compelete;
    rt_mutex_release(&_mtx);

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
 * @brief   处理所有 Agile Led 对象
 * @note    如果使能 PKG_AGILE_LED_USING_THREAD_AUTO_INIT, 这个函数将被自动初始化线程 5ms 周期调用。
 *          用户调用需要创建一个线程并将这个函数放入 while (1) {} 中。
 */
void agile_led_process(void)
{
    rt_slist_t *node;

    rt_mutex_take(&_mtx, RT_WAITING_FOREVER);
    rt_slist_for_each(node, &_slist_head)
    {
        agile_led_t *led = rt_slist_entry(node, agile_led_t, slist);
        if (led->loop_cnt == 0) {
            agile_led_stop(led);
            if (led->compelete) {
                led->compelete(led);
            }

            node = &_slist_head;
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
    rt_mutex_release(&_mtx);
}

/**
 * @brief   Agile Led 环境初始化
 * @note    使用其他 API 之前该函数必须被调用。
 *          如果使能 PKG_AGILE_LED_USING_THREAD_AUTO_INIT, 这个函数将被自动调用。
 */
void agile_led_env_init(void)
{
    if (_is_init)
        return;

    rt_mutex_init(&_mtx, "led_mtx", RT_IPC_FLAG_FIFO);

    _is_init = 1;
}

/**
 * @}
 */

#ifdef PKG_AGILE_LED_USING_THREAD_AUTO_INIT

/** @addtogroup AGILE_LED_Thread_Auto_Init
 * @{
 */

/** @defgroup AGILE_LED_Thread_Auto_Init_Functions Agile Led Thread Auto Init Functions
 * @{
 */

/**
 * @brief   Agile Led 内部线程函数入口
 * @param   parameter 线程参数
 */
static void agile_led_auto_thread_entry(void *parameter)
{
    while (1) {
        agile_led_process();
        rt_thread_mdelay(5);
    }
}

/**
 * @brief   Agile Led 内部线程初始化
 * @return  RT_EOK:成功
 */
static int agile_led_auto_thread_init(void)
{
    agile_led_env_init();

    rt_thread_init(&_thread,
                   "agled",
                   agile_led_auto_thread_entry,
                   RT_NULL,
                   &_thread_stack[0],
                   sizeof(_thread_stack),
                   PKG_AGILE_LED_THREAD_PRIORITY,
                   100);

    rt_thread_startup(&_thread);

    return RT_EOK;
}
INIT_APP_EXPORT(agile_led_auto_thread_init);

/**
 * @}
 */

/**
 * @}
 */

#endif /* PKG_AGILE_LED_USING_THREAD_AUTO_INIT */
