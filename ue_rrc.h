#ifndef RRC_SERVER_H
#define RRC_SERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>

#include <DL-CCCH-Message.h>

void RRCConnectionRequest_coder(uint8_t **buffer, ssize_t *len);
void RRCConnectionSetupComplete_coder(uint8_t **buffer, ssize_t *len, int transaction_id);
DL_CCCH_Message_t* DL_CCCH_Message_decoder(uint8_t *buffer, ssize_t len);
void tx_send(uint8_t **buffer, ssize_t *len);

#endif