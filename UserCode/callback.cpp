#include "main.h"
#include "can.h"
#include "tim.h"

#include "can_user.h"
#include "Motor.hpp"

Motor motor(19.2f);

// Keil Watch: edit this command while the target is running. Unit: A.
volatile float debug_tx_current = 0.0f;
volatile float debug_applied_current = 0.0f;

// Feedback snapshots and CAN diagnostics for the lab exercise.
volatile float debug_angle = 0.0f;          // Output shaft accumulated angle, deg.
volatile float debug_speed_rpm = 0.0f;      // Rotor speed, RPM.
volatile float debug_current_a = 0.0f;      // Actual torque current, A.
volatile float debug_temperature_c = 0.0f;
volatile uint32_t debug_rx_count = 0;
volatile uint32_t debug_tx_queued_count = 0;
volatile uint32_t debug_tx_fail_count = 0;
volatile uint32_t debug_can_error = HAL_CAN_ERROR_NONE;
volatile uint32_t debug_tim6_count = 0;
volatile uint32_t debug_tx_free_mailboxes = 3;
volatile uint32_t debug_can_esr = 0;          // Hardware error status register.

static constexpr uint8_t kMotorId = 4;
static constexpr uint32_t kMotorFeedbackId = 0x200U + kMotorId;
static constexpr float kDebugCurrentLimitA = 1.0f;

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(
        CAN_HandleTypeDef* hcan)
{
    uint8_t data[8];
    if (hcan != &hcan1) return;

    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0,
                                &rx_header, data)
            != HAL_OK) return;

        if (rx_header.IDE == CAN_ID_STD &&
            rx_header.RTR == CAN_RTR_DATA &&
            rx_header.DLC == 8 &&
            rx_header.StdId == kMotorFeedbackId)
        {
            motor.canRxMsgCallback(data);
            debug_angle = motor.angle();
            debug_speed_rpm = motor.speedRpm();
            debug_current_a = motor.currentAmps();
            debug_temperature_c = motor.temperatureC();
            ++debug_rx_count;
        }
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(
        TIM_HandleTypeDef* htim)
{
    if (htim != &htim6) return;
    ++debug_tim6_count;

    float current = debug_tx_current;
    if (current != current) current = 0.0f;
    if (current > kDebugCurrentLimitA) current = kDebugCurrentLimitA;
    if (current < -kDebugCurrentLimitA) current = -kDebugCurrentLimitA;
    debug_applied_current = current;
    motor.setTxCurrent(current, kMotorId);

    debug_tx_free_mailboxes = HAL_CAN_GetTxMailboxesFreeLevel(&hcan1);
    if (debug_tx_free_mailboxes > 0)
    {
        const HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(
            &hcan1,
            &tx_header,
            motor.getTxData(),
            &can_tx_mailbox);
        if (status == HAL_OK)
        {
            ++debug_tx_queued_count;
        }
        else
        {
            ++debug_tx_fail_count;
        }
    }
    debug_can_error = HAL_CAN_GetError(&hcan1);
    debug_can_esr = hcan1.Instance->ESR;
}
