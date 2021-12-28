#include <agile_led.h>
#include <stdlib.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

#ifdef RT_USING_HEAP

static agile_led_t *_dled = RT_NULL;

static void dled_create(int argc, char **argv)
{
    int pin = 0;
    int active = 0;

    if (argc < 3) {
        rt_kprintf("dled_create      --use dled_create [pin] [active]\r\n");
        return;
    }

    pin = atoi(argv[1]);
    active = atoi(argv[2]);

    if (_dled) {
        agile_led_delete(_dled);
        _dled = RT_NULL;
    }

    _dled = agile_led_create(pin, active, "100,200", -1);
}

static void dled_delete(void)
{
    if (_dled) {
        agile_led_delete(_dled);
        _dled = RT_NULL;
    }
}

static void dled_start(void)
{
    if (_dled)
        agile_led_start(_dled);
}

static void dled_stop(void)
{
    if (_dled)
        agile_led_stop(_dled);
}

static void dled_set_mode(int argc, char **argv)
{
    int loop_cnt = 0;

    if (argc < 3) {
        rt_kprintf("dled_set_mode      --use dled_set_mode [mode] [loop_cnt]\r\n");
        return;
    }

    loop_cnt = atoi(argv[2]);

    if (_dled)
        agile_led_dynamic_change_light_mode(_dled, argv[1], loop_cnt);
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(dled_create, create led);
MSH_CMD_EXPORT(dled_delete, delete led);
MSH_CMD_EXPORT(dled_start, start led);
MSH_CMD_EXPORT(dled_stop, stop led);
MSH_CMD_EXPORT(dled_set_mode, set led mode);
#endif

#endif /* RT_USING_HEAP */

static agile_led_t _sled;
static uint32_t _sled_light_arr[10] = {100, 200};

static void sled_init(int argc, char **argv)
{
    int pin = 0;
    int active = 0;

    if (argc < 3) {
        rt_kprintf("sled_init      --use sled_init [pin] [active]\r\n");
        return;
    }

    pin = atoi(argv[1]);
    active = atoi(argv[2]);

    agile_led_stop(&_sled);

    agile_led_init(&_sled, pin, active, _sled_light_arr, 2, -1);
}

static void sled_start(void)
{
    agile_led_start(&_sled);
}

static void sled_stop(void)
{
    agile_led_stop(&_sled);
}

static void sled_set_mode(int argc, char **argv)
{
    int loop_cnt = 0;
    int arr_num = 0;

    if (argc < 3) {
        rt_kprintf("sled_set_mode      --use sled_set_mode [mode1] [mode2] ... [loop_cnt]\r\n");
        return;
    }

    arr_num = argc - 2;
    if (arr_num > 10)
        arr_num = 10;

    for (int i = 0; i < arr_num; i++) {
        _sled_light_arr[i] = atoi(argv[i + 1]);
    }

    loop_cnt = atoi(argv[argc - 1]);

    agile_led_static_change_light_mode(&_sled, _sled_light_arr, arr_num, loop_cnt);
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(sled_init, init led);
MSH_CMD_EXPORT(sled_start, start led);
MSH_CMD_EXPORT(sled_stop, stop led);
MSH_CMD_EXPORT(sled_set_mode, set led mode);
#endif
