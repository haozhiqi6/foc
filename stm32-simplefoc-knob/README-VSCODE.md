# VS Code + GCC workflow

This project is configured from the Keil STM32 target for STM32F103C8 / Cortex-M3:
Flash 64 KiB at 0x08000000 and RAM 20 KiB at 0x20000000.

1. Install the GNU Arm Embedded Toolchain and add its bin directory (the one containing arm-none-eabi-gcc.exe) to PATH. Alternatively set ARM_GCC_BIN.
2. Open this directory in VS Code and install C/C++, Cortex-Debug, and OpenOCD extensions.
3. Run task "GCC: build motor". Outputs are in build/bin/: motor.elf, motor.hex, motor.bin, and motor.map.
4. Connect ST-Link and run "OpenOCD: probe only" first. Only after it succeeds, run "OpenOCD: flash motor.hex".

The original Keil project and ARMASM startup files are preserved. GCC uses startup/startup_stm32f103x8.S and STM32F103C8.ld.
