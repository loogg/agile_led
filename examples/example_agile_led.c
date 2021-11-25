#include <agile_led.h>
#include <stdlib.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

static agile_led_t *_led = RT_NULL;

static void led_create(int argc, char **argv)
{
    int pin = 0;
    int active = 0;

    if (argc < 3) {
        rt_kprintf("led_create      --use led_create [pin] [active]\r\n");
        return;
    }

    pin = atoi(argv[1]);
    active = atoi(argv[2]);

    if (_led) {
        agile_led_delete(_led);
        _led = RT_NULL;
    }

    _led = agile_led_create(pin, active, "100,200", -1);
}

static void led_delete(void)
{
    if (_led) {
        agile_led_delete(_led);
        _led = RT_NULL;
    }
}

static void led_start(void)
{
    if (_led)
        agile_led_start(_led);
}

static void led_stop(void)
{
    if (_led)
        agile_led_stop(_led);
}

static void led_set_mode(int argc, char **argv)
{
    int loop_cnt = 0;

    if (argc < 3) {
        rt_kprintf("led_set_mode      --use led_set_mode [mode] [loop_cnt]\r\n");
        return;
    }

    loop_cnt = atoi(argv[2]);

    if (_led)
        agile_led_set_light_mode(_led, argv[1], loop_cnt);
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(led_create, create led);
MSH_CMD_EXPORT(led_delete, delete led);
MSH_CMD_EXPORT(led_start, start led);
MSH_CMD_EXPORT(led_stop, stop led);
MSH_CMD_EXPORT(led_set_mode, set led mode);
#endif
