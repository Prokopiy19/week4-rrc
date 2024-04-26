#include "enb_rrc.h"

#include <DL-CCCH-Message.h>
#include <UL-CCCH-Message.h>
#include <UL-DCCH-Message.h>
#include <RRCConnectionSetup.h>
#include <RRCConnectionRequest.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PAYLOAD_MAX 100

#define asn1cSeqAdd(VaR, PtR) \
    if (asn_sequence_add(VaR, PtR) != 0) assert(0)



void RRCConnectionSetup_coder(uint8_t **buffer, ssize_t *len) {
    DL_CCCH_Message_t pdu;
    
    memset(&pdu, 0, sizeof(pdu));
    pdu.message.present = DL_CCCH_MessageType_PR_c1;
    pdu.message.choice.c1.present = DL_CCCH_MessageType__c1_PR_rrcConnectionSetup;
    
    RRCConnectionSetup_t *out = &pdu.message.choice.c1.choice.rrcConnectionSetup;
    const int transaction_id = 3;
    out->rrc_TransactionIdentifier = transaction_id;
    out->criticalExtensions.present = RRCConnectionSetup__criticalExtensions_PR_c1;
    out->criticalExtensions.choice.c1.present = RRCConnectionSetup__criticalExtensions__c1_PR_rrcConnectionSetup_r8;
    // out->criticalExtensions.choice.c1.rrcConnectionSetup_r8.radioResourceConfigDedicated
    
    asn_encode_to_new_buffer_result_t res = {NULL, {0, NULL, NULL}};
    res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_DL_CCCH_Message, &pdu);
    printf("%lu\n", sizeof(res));
    printf("%lu\n", strlen(static_cast<char*>(res.buffer)));
    free(res.buffer);
    memset(&res, 0, sizeof(res));
    res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_DL_CCCH_Message, &pdu);
    *buffer = static_cast<uint8_t*>(res.buffer);
    *len = res.result.encoded;

    if (*buffer == NULL) {
        fprintf(stderr, "Error enconing rrc pdu\n");
        exit(1);
    } else {
        fprintf(stderr, "Encoded pdu\n");
    }

    xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, &pdu);
}

UL_CCCH_Message_t* UL_CCCH_Message_decoder(uint8_t *buffer, ssize_t len) {
    UL_CCCH_Message_t *pdu = nullptr;
    
    asn_decode(nullptr, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_UL_CCCH_Message,
               reinterpret_cast<void**>(&pdu), static_cast<void*>(buffer), len);

    xer_fprint(stdout, &asn_DEF_UL_CCCH_Message, pdu);
    
    return pdu;
}

UL_DCCH_Message_t* UL_DCCH_Message_decoder(uint8_t *buffer, ssize_t len) {
    UL_DCCH_Message_t *pdu = nullptr;
    
    asn_decode(nullptr, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_UL_DCCH_Message,
               reinterpret_cast<void**>(&pdu), static_cast<void*>(buffer), len);

    xer_fprint(stdout, &asn_DEF_UL_DCCH_Message, pdu);
    
    return pdu;
}

void tx_send(uint8_t **buffer, ssize_t *len) {
    struct sockaddr_in servaddr = {
        .sin_family = AF_INET,
        .sin_port = htons(36412),
        .sin_addr.s_addr = inet_addr("10.18.10.221"),
    };

    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) {
        printf("Error when opening socket\n");
        exit(1);
    }

    int ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        printf("Error when connecting socket\n");
        exit(1);
    }

    ret = sctp_sendmsg(sockfd, *buffer, *len, NULL, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        printf("Error when sending msg\n");
        exit(1);
    }

    printf("Sent packet\n");

    close(sockfd);
}