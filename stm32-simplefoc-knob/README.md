# 基于 STM32 的 SimpleFOC 闭环力控旋钮

把 SimpleFOC 磁场定向控制（FOC）算法移植到 STM32F103C8T6（标准外设库，非 Arduino），
驱动无刷电机实现闭环力矩控制，并做出"磁吸档位 + 惯性旋转"的旋钮手感。

## 目录结构

| 路径 | 说明 |
| --- | --- |
| `user/main.c` | 主程序：模式状态机 + 档位/旋转控制律 |
| `user/MyProject.h` | 头文件汇总；编码器选择开关、电机使能引脚宏 |
| `Hardware/` | 底层驱动：`iic`(AS5600)、`spi2`(TLE5012B)、`timer`(TIM2 PWM / TIM4 1ms)、`usart`、`delay` |
| `SimpleFOC/` | 移植的 FOC 库：BLDCMotor、FOCMotor、foc_utils、lowpass_filter、MagneticSensor、pid |
| `lib/` | STM32F10x 标准外设库 |
| `startup/` | 启动文件（Keil 用 ARMASM，GCC 用 `startup_stm32f103x8.S`） |
| `motor.uvprojx` | Keil MDK 工程，**打开这个** |
| `闭环控制原理图.png`、`闭环控制讲解截图.png` | 原理与实测说明图 |

## 硬件

- MCU：STM32F103C8T6，Flash 64 KiB / RAM 20 KiB
- 编码器：AS5600（I2C）或 TLE5012B（SPI），在 `MyProject.h` 里用 `M1_AS5600` / `M1_TLE5012B` 二选一
- 驱动：TIM2 三路 PWM + PB9 使能（高电平使能）
- 串口：115200，输出 `转速rpm,q轴电压` 两列，周期 10 ms
- 供电：12 V，力矩电压上限 9.9 V
- 电机：7 极对无刷电机

## 控制说明

上电流程：使能引脚拉低 → 初始化串口 / I2C / PWM / 1ms 定时器 → 读取磁编码器
（失败则停机死循环）→ FOC 对齐（对齐电压 2.5 V、限幅 6 V）→ 进入主循环。

主循环 1 ms 一拍：读转速（低通滤波）→ 按当前模式算出 q 轴目标电压 → `loopFOC()` 输出；
每 10 ms 打一帧串口数据。

两种模式：

- **档位模式（DETENT）**：每 0.1745 rad（10°）一个档位，带 0.5° 滞回；
  位置环 Kp=6、阻尼 Kd=0.02，限幅 9.8 V。
- **旋转模式（SPIN）**：转速 ≥100 rpm 持续 10 ms 后切入，经过 50 ms 入场斜坡、
  3 s 观察延时，再用 1 s 趋势窗比较转速下降量；若下降小于 5 rpm 则认为该撤力，
  在 20 s 内线性淡出助力。转速 ≤20 rpm 持续 50 ms 后切回档位模式。

## 编译与烧录

- **Keil**：打开 `motor.uvprojx`，直接 Build / Download。
- **VS Code + GCC（可选）**：见 `README-VSCODE.md`。装好 GNU Arm Embedded Toolchain 后
  运行任务 `GCC: build motor`，产物在 `build/bin/`；先用 OpenOCD 探测，再烧录 `motor.hex`。

## 注意事项

- 电机上电前先确认电源限流，避免对齐阶段大电流。
- 本仓库不含 `tools/`（便携 OpenOCD 工具链，1.4 GB）与编译产物，需要时请自行安装工具链。
