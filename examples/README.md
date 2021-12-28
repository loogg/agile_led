# 示例说明

## 1、介绍

该示例提供动态创建和静态创建功能演示，需要 `FINSH` 的支持。请开启 `FINSH` 功能。

## 2、使用

### 2.1、动态创建

- 创建对象

  命令行输入 `dled_create [pin] [active]` ，默认 “100ms 亮，200ms 灭” 循环闪烁

  pin: 引脚号，查看驱动中 Led IO 对应的引脚号

  active: 有效电平 1/0

- 修改闪烁模式

  命令行输入 `dled_set_mode [mode] [loop_cnt]`

  mode: 闪烁模式字符串，如："100,200,300,400" ，按亮灭亮灭规律

  loop_cnt: 循环次数，负数则无限循环

- 删除对象

  命令行输入 `dled_delete`

- 启动运行

  命令行输入 `dled_start`

- 停止运行

  命令行输入 `dled_stop`

1. `dled_create [pin] [active]` 创建对象
2. `dled_start` 启动运行
3. `dled_set_mode [mode] [loop_cnt]` 修改模式
4. `dled_stop` 停止运行
5. `dled_delete` 删除对象

### 2.2、静态创建

- 初始化对象

  命令行输入 `sled_init [pin] [active]` ，默认 “100ms 亮，200ms 灭” 循环闪烁

  pin: 引脚号，查看驱动中 Led IO 对应的引脚号

  active: 有效电平 1/0

- 修改闪烁模式

  命令行输入 `sled_set_mode [mode1] [mode2] ... [loop_cnt]`

  mode: 亮灭时间，如：`sled_set_mode 100 200 300 400 -1` ，其中 `100 200 300 400` 为亮灭时间，按亮灭亮灭规律

  loop_cnt: 循环次数，负数则无限循环

- 启动运行

  命令行输入 `sled_start`

- 停止运行

  命令行输入 `sled_stop`

1. `sled_init [pin] [active]` 初始化对象
2. `sled_start` 启动运行
3. `sled_set_mode [mode1] [mode2] ... [loop_cnt]` 修改模式
4. `sled_stop` 停止运行
