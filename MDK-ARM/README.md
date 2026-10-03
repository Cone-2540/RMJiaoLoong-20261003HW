# Keil build

Open `261003Class.uvprojx` and build target `261003Class` with Arm Compiler 6.

The FreeRTOS V10.3.1 Cortex-M4F port is
`../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F`.
Its GNU-style inline assembly is supported by ArmClang. Both `port.c` and the
compiler/assembler include paths use this directory. The port files were copied
unchanged from STM32Cube FW_F4 V1.28.3.

After regenerating the MDK project with STM32CubeMX, check that Arm Compiler 6
is selected and restore the `portable/GCC/ARM_CM4F` source and include paths
if CubeMX selects `portable/RVDS/ARM_CM4F` (the Arm Compiler 5 port).
Keep `port.c` and `portmacro.h` in the GCC directory.

Verified with Keil uVision 5.38 / Arm Compiler 6.19 using a full rebuild:
0 errors, 0 warnings. Outputs are `261003Class/261003Class.axf` and
`261003Class/261003Class.hex`. The rebuild log is `keil-rebuild.log`.
