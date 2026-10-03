#include "can_user.h"

CAN_RxHeaderTypeDef rx_header;

CAN_TxHeaderTypeDef tx_header = {
    .StdId = 0x200,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = 8,
    .TransmitGlobalTime = DISABLE
};

uint32_t can_tx_mailbox;

CAN_FilterTypeDef can_filter_config;
