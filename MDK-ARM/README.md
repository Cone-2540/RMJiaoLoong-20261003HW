# Keil build

Open `261003Class.uvprojx` and build target `261003Class` with Arm Compiler 6.

The current application uses a bare-metal main loop and HAL interrupts.
The Keil target includes the sources in `../UserCode`, adds that directory to
the compiler include paths, and selects C++11 for `Motor.cpp` and `callback.cpp`.
The obsolete FreeRTOS `port.c` entry and include path have been removed.

After regenerating the MDK project with STM32CubeMX, restore the UserCode source
entries, the `../UserCode` include path, and C++11 under Options for Target >
C/C++ (AC6). Headers added to the project tree use Text Document File type.

Verified with Keil uVision 5.38 / Arm Compiler 6.19 using a full rebuild on
2026-10-03:
0 errors, 0 warnings. Outputs are `261003Class/261003Class.axf` and
`261003Class/261003Class.hex`. The rebuild log is `keil-rebuild.log`.

## CAN behavior

CAN1 uses PD0 (RX) and PD1 (TX). Initialization configures the FIFO0 filter,
starts CAN1, enables receive notifications, and starts TIM6 interrupts.
The linked callbacks receive standard data frames with ID 0x204 and DLC 8,
and request transmission of standard data frames with ID 0x200 and DLC 8
when a transmit mailbox is free.

With the configured 12 MHz HSE, PLLM=6, PLLN=168, PLLP=2 and PLLQ=7,
SYSCLK is 168 MHz and APB1 is 42 MHz. Flash latency is 5 wait states.
CAN bitrate is 42 MHz / (3 * (1 + 9 + 4)) = 1 Mbit/s, matching C620.
TIM6 frequency is 84 MHz / (84 * 1000) = 1000 Hz (1 ms per frame).
Verify the physical oscillator and the peer bitrate before testing on hardware.

The transmit buffer and `debug_tx_current` start at zero. Each TIM6 callback
limits `debug_tx_current` to +/-1 A for this exercise and calls
`Motor::setTxCurrent(current, kMotorId)` with `kMotorId = 4` before requesting
transmission. Hardware
validation at the initial zero command should capture
ID 0x200 with DLC 8 and payload `00 00 00 00 00 00 00 00`, using a powered CAN
transceiver, common ground, bus termination, and an active peer providing ACK.
Check `HAL_CAN_AddTxMessage` status and `HAL_CAN_GetError` when diagnosing
transmit failures. Compilation and linking validate the software build;
bus-level behavior requires a hardware test.

## Keil current debugging

This implements the exercise on page 52 of `../references/CAN.pdf`: verify
continuous accumulated angle by turning the motor by hand, then apply a small
current. Page 41 requires zero output at startup and suggests a debugger
variable in `callback.cpp`. The C620 manual, page 32, specifies ID 0x200 for
motor IDs 1-4, high byte first, and +/-16384 mapping to +/-20 A.

1. Rebuild, download the AXF to the board, enter Debug, and open Watch 1.
2. Add the globals below and run the target. Keep the motor secured and the
   output shaft clear while testing open-loop torque current.
3. Keep `debug_tx_current` at 0. Confirm `debug_rx_count` increases. Turn the
   shaft by hand and check that `debug_angle` varies continuously across
   encoder wraparound. The first feedback establishes the angle baseline.
4. While running, write 0.2 to `debug_tx_current` (unit: A). Observe the speed,
   current feedback, and accumulated angle. A small current may require a
   modest increase to overcome friction; this exercise limits magnitude to 1 A.
5. Write 0 to return to zero torque current. Observe the shaft settling, then
   test -0.2 for reverse torque. Return the command to 0 when finished.

| Watch expression | Meaning |
| --- | --- |
| `debug_tx_current` | Editable requested torque current in A; default 0 |
| `debug_applied_current` | Software command after the +/-1 A exercise limit |
| `debug_angle` | Accumulated output shaft angle in degrees |
| `debug_speed_rpm` | Measured rotor speed in RPM |
| `debug_current_a` | Measured torque current in A |
| `debug_temperature_c` | Measured motor temperature in degrees C |
| `debug_rx_count` | Valid ID 0x204 feedback frames processed |
| `debug_tx_queued_count` | Frames accepted into a transmit mailbox |
| `debug_tx_fail_count` | Failed calls to HAL_CAN_AddTxMessage |
| `debug_can_error` | HAL software error bitmask; hardware errors require checking ESR |
| `debug_tim6_count` | Timer callback count; increases at approximately 1000/s |
| `debug_tx_free_mailboxes` | Available transmit mailboxes before queueing, 0-3 |
| `debug_can_esr` | Raw CAN hardware error status register |

At +0.2 A, truncation gives a raw command of 163 and payload
`00 00 00 00 00 00 00 A3`. At -0.2 A, the payload is
`00 00 00 00 00 00 FF 5D`. This test targets motor ID 4; the other three
current slots stay zero. `debug_tx_queued_count` measures mailbox acceptance.
Use motor feedback and a CAN capture to verify communication with the ESC.

Constant current commands apply torque, and the unloaded motor can continue
accelerating. Zero current requests zero torque; the shaft can coast. Return
the command to zero while the program is running before halting or ending the
debug session. The Watch command persists in RAM until edited or reset.

For a snapshot with zero RX frames and exactly three queued TX frames, first
check that `debug_tim6_count` increases. A free-mailbox count staying at zero
indicates pending transmissions occupying all three mailboxes. Check the CAN
bitrate against C620's 1 Mbit/s specification. The original 126 MHz SYSCLK
configuration yielded 750 kbit/s and has been corrected to 168 MHz.
Only FIFO0 receive notifications are enabled; HAL_CAN_GetError can stay zero
while hardware bus errors accumulate. Read `debug_can_esr` for hardware status:
bits 6:4 are the last error code (3 = ACK error), bits 23:16 are the transmit
error counter, bits 31:24 are the receive error counter, and bit 2 is bus-off.
