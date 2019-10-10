#include <agile_led.h>
#include <drv_gpio.h>
#include <stdlib.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

#define LED0_PIN    GET_PIN(C, 6) 
#define LED1_PIN    GET_PIN(B, 15) 
#define LED2_PIN    GET_PIN(B, 14) 

static agile_led_t *led0 = RT_NULL;
static agile_led_t *led1 = RT_NULL;
static agile_led_t *led2 = RT_NULL;

static void led_create(void)
{
    if(led0 == RT_NULL)
    {
        led0 = agile_led_create(LED0_PIN, PIN_LOW, "100,200", -1);
    }

    if(led1 == RT_NULL)
    {
        led1 = agile_led_create(LED1_PIN, PIN_LOW, "200,100", -1);
    }

    if(led2 == RT_NULL)
    {
        led2 = agile_led_create(LED2_PIN, PIN_LOW, "100,100", -1);
    }
}

static void led_delete(void)
{
    if(led0)
    {
        agile_led_delete(led0);
        led0 = RT_NULL;
    }

    if(led1)
    {
        agile_led_delete(led1);
        led1 = RT_NULL;
    }

    if(led2)
    {
        agile_led_delete(led2);
        led2 = RT_NULL;
    }
}

static void led_start(int argc, char**argv)
{
    int led_index = 0;
    agile_led_t *led = RT_NULL;
    if(argc < 2)
    {
        rt_kprintf("led_start      --use led_start (0/1/2)\r\n");
        return;
    }
    led_index = atoi(argv[1]);
    switch (led_index)
    {
    case 0:
        led = led0;
        break;

    case 1:
        led = led1;
        break;

    case 2:
        led = led2;
        break;

    default:
        break;
    }
    if(led == RT_NULL)
        return;
    agile_led_start(led);
}

static void led_stop(int argc, char**argv)
{
    int led_index = 0;
    agile_led_t *led = RT_NULL;
    if(argc < 2)
    {
        rt_kprintf("led_stop      --use led_stop (0/1/2)\r\n");
        return;
    }
    led_index = atoi(argv[1]);
    switch (led_index)
    {
    case 0:
        led = led0;
        break;

    case 1:
        led = led1;
        break;

    case 2:
        led = led2;
        break;

    default:
        break;
    }
    if(led == RT_NULL)
        return;
    agile_led_stop(led);
}

static void led_set_mode(int argc, char**argv)
{
    int led_index = 0;
    int loop_cnt = 0;
    agile_led_t *led = RT_NULL;
    if(argc < 4)
    {
        rt_kprintf("led_set_mode      --use led_set_mode (0/1/2) [mode] [loop_cnt]\r\n");
        return;
    }
    led_index = atoi(argv[1]);
    loop_cnt = atoi(argv[3]);
    switch (led_index)
    {
    case 0:
        led = led0;
        break;

    case 1:
        led = led1;
        break;

    case 2:
        led = led2;
        break;

    default:
        break;
    }
    if(led == RT_NULL)
        return;
    agile_led_set_light_mode(led, argv[2], loop_cnt);
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(led_create, create led);
MSH_CMD_EXPORT(led_delete, delete led);
MSH_CMD_EXPORT(led_start, start led);
MSH_CMD_EXPORT(led_stop, stop led);
MSH_CMD_EXPORT(led_set_mode, set led mode);
#endif
