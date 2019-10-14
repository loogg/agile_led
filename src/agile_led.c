/*************************************************
 All rights reserved.
 File name:     agile_led.c
 Description:
 History:
 1. Version:      v1.0.0
    Date:         2019-10-09
    Author:       Longwei Ma
    Modification: 新建版本
*************************************************/

#include <agile_led.h>
#include <stdlib.h>
#include <string.h>

#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME    "agile_led"
#ifdef PKG_AGILE_LED_DEBUG
#define DBG_LEVEL           DBG_LOG
#else
#define DBG_LEVEL           DBG_INFO
#endif
#include <rtdbg.h>

// agile_led 线程堆栈大小
#ifndef PKG_AGILE_LED_THREAD_STACK_SIZE
#define PKG_AGILE_LED_THREAD_STACK_SIZE 256
#endif

// agile_led 线程优先级
#ifndef PKG_AGILE_LED_THREAD_PRIORITY
#define PKG_AGILE_LED_THREAD_PRIORITY RT_THREAD_PRIORITY_MAX - 4
#endif

// agile_led 单向链表
static rt_slist_t agile_led_list = RT_SLIST_OBJECT_INIT(agile_led_list);
// agile_led 互斥锁
static rt_mutex_t lock_mtx = RT_NULL;
// agile_led 初始化完成标志
static uint8_t is_initialized = 0;

/**
* Name:             agile_led_default_compelete_callback
* Brief:            led对象默认操作完成回调函数
* Input:
*   @led:           agile_led对象指针
* Output:           none
*/
static void agile_led_default_compelete_callback(agile_led_t *led)
{
    RT_ASSERT(led);
    LOG_D("led pin:%d compeleted.", led->pin);
}

/**
* Name:             agile_led_get_light_arr
* Brief:            获取led对象闪烁模式
* Input:
*   @led:           agile_led对象指针
*   @light_mode:    闪烁模式字符串
* Output:           RT_EOK:成功
*                   !=RT_EOK:异常
*/
static int agile_led_get_light_arr(agile_led_t *led, const char *light_mode)
{
    RT_ASSERT(led);
    RT_ASSERT(led->light_arr == RT_NULL);
    RT_ASSERT(led->arr_num == 0);
    const char *ptr = light_mode;
    while (*ptr)
    {
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
    for (uint32_t i = 0; i < led->arr_num; i++)
    {
        led->light_arr[i] = atoi(ptr);
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }
    return RT_EOK;
}

/**
* Name:             agile_led_create
* Brief:            创建led对象
* Input:
*   @pin:           控制led的引脚
*   @active_logic:  led有效电平(PIN_HIGH/PIN_LOW)
*   @light_mode:    闪烁模式字符串
*   @loop_cnt:      循环次数
* Output:           !=RT_NULL:agile_led对象指针
*                   RT_NULL:异常
*/
agile_led_t *agile_led_create(rt_base_t pin, rt_base_t active_logic, const char *light_mode, int32_t loop_cnt)
{
    if (!is_initialized)
    {
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
    if (light_mode)
    {
        if (agile_led_get_light_arr(led, light_mode) < 0)
        {
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
* Name:             agile_led_delete
* Brief:            删除led对象
* Input:
*   @led:           led对象指针
* Output:           RT_EOK:成功
*/
int agile_led_delete(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    rt_slist_remove(&(agile_led_list), &(led->slist));
    led->slist.next = RT_NULL;
    rt_mutex_release(lock_mtx);
    if(led->light_arr)
    {
        rt_free(led->light_arr);
        led->light_arr = RT_NULL;
    }
    rt_free(led);
    return RT_EOK;
}

/**
* Name:             agile_led_start
* Brief:            启动led对象,根据设置的模式执行动作
* Input:
*   @led:           led对象指针
* Output:           RT_EOK:成功
*                   !=RT_OK:异常
*/
int agile_led_start(agile_led_t *led)
{ 
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    if(led->active)
    {
        rt_mutex_release(lock_mtx);
        return -RT_ERROR;
    }
    if((led->light_arr == RT_NULL) || (led->arr_num == 0))
    {
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
* Name:             agile_led_stop
* Brief:            停止led对象
* Input:
*   @led:           led对象指针
* Output:           RT_EOK:成功
*/
int agile_led_stop(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    if(!led->active)
    {
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
* Name:             agile_led_set_light_mode
* Brief:            设置led对象的模式
* Input:
*   @led:           led对象指针
*   @light_mode:    闪烁模式字符串
*   @loop_cnt:      循环次数
* Output:           RT_EOK:成功
*                   !=RT_EOK:异常
*/
int agile_led_set_light_mode(agile_led_t *led, const char *light_mode, int32_t loop_cnt)
{
    RT_ASSERT(led);
    rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
    
    if (light_mode)
    {
        if (led->light_arr)
        {
            rt_free(led->light_arr);
            led->light_arr = RT_NULL;
        }
        led->arr_num = 0;
        if (agile_led_get_light_arr(led, light_mode) < 0)
        {
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
* Name:             agile_led_set_compelete_callback
* Brief:            设置led对象操作完成的回调函数
* Input:
*   @led:           led对象指针
*   @compelete:     操作完成回调函数
* Output:           RT_EOK:成功
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
* Name:             agile_led_toggle
* Brief:            led对象电平翻转
* Input:
*   @led:           led对象指针
* Output:           none
*/
void agile_led_toggle(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, !rt_pin_read(led->pin));
}

/**
* Name:             agile_led_on
* Brief:            led对象亮
* Input:
*   @led:           led对象指针
* Output:           none
*/
void agile_led_on(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, led->active_logic);
}

/**
* Name:             agile_led_off
* Brief:            led对象灭
* Input:
*   @led:           led对象指针
* Output:           none
*/
void agile_led_off(agile_led_t *led)
{
    RT_ASSERT(led);
    rt_pin_write(led->pin, !led->active_logic);
}

/**
* Name:             led_process
* Brief:            agile_led线程
* Input:
*   @parameter:     线程参数
* Output:           none
*/
static void led_process(void *parameter)
{
    rt_slist_t *node;
    while (1)
    {
        rt_mutex_take(lock_mtx, RT_WAITING_FOREVER);
        rt_slist_for_each(node, &(agile_led_list))
        {
            agile_led_t *led = rt_slist_entry(node, agile_led_t, slist);
            if(led->loop_cnt == 0)
            {
                agile_led_stop(led);
                if(led->compelete)
                {
                    led->compelete(led);
                }
                node = &agile_led_list;
                continue;
            }
        __repeat:
            if((rt_tick_get() - led->tick_timeout) < (RT_TICK_MAX / 2))
            {
                if(led->arr_index < led->arr_num)
                {
                    if (led->light_arr[led->arr_index] == 0)
                    {
                        led->arr_index++;
                        goto __repeat;
                    }
                    if(led->arr_index % 2)
                    {
                        agile_led_off(led);
                    }
                    else
                    {
                        agile_led_on(led);
                    }
                    led->tick_timeout = rt_tick_get() + rt_tick_from_millisecond(led->light_arr[led->arr_index]);
                    led->arr_index++;
                }
                else
                {
                    led->arr_index = 0;
                    if(led->loop_cnt > 0)
                        led->loop_cnt--;
                }
            }
        }
        rt_mutex_release(lock_mtx);
        rt_thread_mdelay(5);
    }
}

/**
* Name:             agile_led_init
* Brief:            agile_led初始化
* Input:            none
* Output:           RT_EOK:成功
*                   !=RT_EOK:失败
*/
static int agile_led_init(void)
{
    rt_thread_t tid = RT_NULL;
    lock_mtx = rt_mutex_create("led_mtx", RT_IPC_FLAG_FIFO);
    if (lock_mtx == RT_NULL)
    {
        LOG_E("Agile led initialize failed! lock_mtx create failed!");
        return -RT_ENOMEM;
    }

    tid = rt_thread_create("agile_led", led_process, RT_NULL,
                           PKG_AGILE_LED_THREAD_STACK_SIZE, PKG_AGILE_LED_THREAD_PRIORITY, 100);
    if (tid == RT_NULL)
    {
        LOG_E("Agile led initialize failed! thread create failed!");
        rt_mutex_delete(lock_mtx);
        return -RT_ENOMEM;
    }
    rt_thread_startup(tid);
    is_initialized = 1;
    return RT_EOK;
}
INIT_APP_EXPORT(agile_led_init);
