#ifndef CAN_USER_H
#define CAN_USER_H

#include "can.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UserCode/can_user.h */
extern CAN_RxHeaderTypeDef rx_header;
extern CAN_TxHeaderTypeDef tx_header;
extern uint32_t can_tx_mailbox;
extern CAN_FilterTypeDef can_filter_config;

#ifdef __cplusplus
}
#endif

#endif
