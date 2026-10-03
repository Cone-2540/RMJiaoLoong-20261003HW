#include "Motor.hpp"
#include "main.h"

Motor::Motor(const float ratio)
    : ratio_(ratio > 0.0f ? ratio : 1.0f)
{
}

void Motor::canRxMsgCallback(const uint8_t rx_data[8])
{
    // 1. 拆 8 字节为字段（字节序 + 符号）
    const uint16_t ecd =
        (static_cast<uint16_t>(rx_data[0]) << 8) |
        static_cast<uint16_t>(rx_data[1]);

    const uint16_t speed_bits =
        (static_cast<uint16_t>(rx_data[2]) << 8) |
        static_cast<uint16_t>(rx_data[3]);

    const uint16_t current_bits =
        (static_cast<uint16_t>(rx_data[4]) << 8) |
        static_cast<uint16_t>(rx_data[5]);

    // 将 16 位补码解释为有符号数
    const int32_t speed_raw =
        (speed_bits & 0x8000U)
        ? static_cast<int32_t>(speed_bits) - 65536
        : static_cast<int32_t>(speed_bits);

    const int32_t current_raw =
        (current_bits & 0x8000U)
        ? static_cast<int32_t>(current_bits) - 65536
        : static_cast<int32_t>(current_bits);

    ecd_angle_ = static_cast<float>(ecd);
    speedRpm_ = static_cast<float>(speed_raw);
    currentA_ = static_cast<float>(current_raw) * 20.0f / 16384.0f;
    tempC_ = static_cast<float>(rx_data[6]);

    // 第一帧用于建立角度基准
    if (!received_)
    {
        last_ecd_ = ecd_angle_;
        received_ = true;
        return;
    }

    // 3. 增量累加 + 过零点环绕修正
    float delta = ecd_angle_ - last_ecd_;

    if (delta > kEncoderRange / 2.0f)
    {
        delta -= kEncoderRange;
    }
    else if (delta < -kEncoderRange / 2.0f)
    {
        delta += kEncoderRange;
    }

    // 2. 角度换算 ×360/8192
    // 4. 折到输出轴（÷ 减速比）
    angle_ += delta * 360.0f / kEncoderRange / ratio_;

    last_ecd_ = ecd_angle_;
}

float Motor::angle() const
{
    return angle_;
}

float Motor::speedRpm() const
{
    return speedRpm_;
}

float Motor::currentAmps() const
{
    return currentA_;
}

float Motor::temperatureC() const
{
    return tempC_;
}

bool Motor::hasFeedback() const
{
    return received_;
}

void Motor::setTxCurrent(float amperes, uint8_t motor_id)
{
    if (motor_id < 1 || motor_id > 4)
    {
        return;
    }

    // NaN 输入按零电流处理
    if (amperes != amperes)
    {
        amperes = 0.0f;
    }

    // 电流限幅
    if (amperes > 20.0f)
    {
        amperes = 20.0f;
    }
    else if (amperes < -20.0f)
    {
        amperes = -20.0f;
    }

    const int16_t current_raw =
        static_cast<int16_t>(amperes * 16384.0f / 20.0f);

    const uint16_t bits = static_cast<uint16_t>(current_raw);
    const uint8_t index = (motor_id - 1) * 2;

    // 防止发送中断在两字节更新之间取走数据
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    tx_data_[index] = static_cast<uint8_t>(bits >> 8);
    tx_data_[index + 1] = static_cast<uint8_t>(bits & 0xFFU);

    __set_PRIMASK(primask);
}

uint8_t* Motor::getTxData()
{
    return tx_data_;
}
