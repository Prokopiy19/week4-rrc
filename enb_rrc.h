#ifndef RRC_SERVER_H
#define RRC_SERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>

#include <UL-CCCH-Message.h>
#include <UL-DCCH-Message.h>

void RRCConnectionSetup_coder(uint8_t **buffer, ssize_t *len);
UL_CCCH_Message_t* UL_CCCH_Message_decoder(uint8_t *buffer, ssize_t len);
UL_DCCH_Message_t* UL_DCCH_Message_decoder(uint8_t *buffer, ssize_t len);
void tx_send(uint8_t **buffer, ssize_t *len);

#endif