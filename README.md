# Agile Led

## 1、介绍

Agile Led 是基于 RT-Thread 实现的 led 软件包，提供 led 操作的 API。

### 1.1、特性

1. 代码简洁易懂，充分使用 RT-Thread 提供的 API
2. 详细注释
3. 线程安全
4. 断言保护
5. API 操作简单

### 1.2、目录结构

| 名称 | 说明 |
| ---- | ---- |
| doc | 文档目录 |
| examples | 例子目录 |
| inc  | 头文件目录 |
| src  | 源代码目录 |

### 1.3、许可证

Agile Led package 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.4、依赖

- RT-Thread 3.0+
- RT-Thread 4.0+

## 2、如何打开 Agile Led

使用 Agile Led package 需要在 RT-Thread 的包管理器中选择它，具体路径如下：

```
RT-Thread online packages
    peripheral libraries and drivers --->
        [*] agile_led: A agile led package
```

然后让 RT-Thread 的包管理器自动更新，或者使用 `pkgs --update` 命令更新包到 BSP 中。

## 3、使用 Agile Led

- 帮助文档请查看 [doc/doxygen/Agile_Led.chm](./doc/doxygen/Agile_Led.chm)

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

### 3.1、示例

使用示例在 [examples](./examples) 下。

### 3.2、Doxygen 文档生成

- 使用 `Doxywizard` 打开 [Doxyfile](./doc/doxygen/Doxyfile) 运行，生成的文件在 [doxygen/output](./doc/doxygen/output) 下。
- 需要更改 `Graphviz` 路径。
- `HTML` 生成未使用 `chm` 格式的，如果使能需要更改 `hhc.exe` 路径。

## 4、联系方式 & 感谢

- 维护：马龙伟
- 主页：<https://github.com/loogg/agile_led>
- 邮箱：<2544047213@qq.com>
