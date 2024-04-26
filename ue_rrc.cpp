#include "ue_rrc.h"

#include <UL-CCCH-Message.h>
#include <UL-DCCH-Message.h>
#include <DL-CCCH-Message.h>
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

#include <random>

#define PAYLOAD_MAX 100

#define asn1cSeqAdd(VaR, PtR) \
    if (asn_sequence_add(VaR, PtR) != 0) assert(0)

void RRCConnectionRequest_coder(uint8_t **buffer, ssize_t *len) {
    UL_CCCH_Message_t pdu;
    
    uint8_t buf[5], buf2 = 0;
    
    memset(&pdu, 0, sizeof(pdu));
    pdu.message.present = UL_CCCH_MessageType_PR_c1;
    pdu.message.choice.c1.present = UL_CCCH_MessageType__c1_PR_rrcConnectionRequest;
    pdu.message.choice.c1.choice.rrcConnectionRequest.criticalExtensions.present = RRCConnectionRequest__criticalExtensions_PR_rrcConnectionRequest_r8;
    
    auto out = &pdu.message.choice.c1.choice.rrcConnectionRequest.criticalExtensions.choice.rrcConnectionRequest_r8;
    
    out->ue_Identity.present = InitialUE_Identity_PR_randomValue;
    std::random_device rd;
    const auto rv = std::uniform_int_distribution<long>(0L, (1L << 40L) - 1L)(rd);
    out->ue_Identity.choice.randomValue.size = 5;
    out->ue_Identity.choice.randomValue.bits_unused = 0;
    out->ue_Identity.choice.randomValue.buf = buf;
    out->ue_Identity.choice.randomValue.buf[0] = (rv >> 32) & 0xFF;
    out->ue_Identity.choice.randomValue.buf[1] = (rv >> 24) & 0xFF;
    out->ue_Identity.choice.randomValue.buf[2] = (rv >> 16) & 0xFF;
    out->ue_Identity.choice.randomValue.buf[3] = (rv >> 8) & 0xFF;
    out->ue_Identity.choice.randomValue.buf[4] = rv & 0xFF;
    
    out->establishmentCause = EstablishmentCause_mo_Signalling;
    
    out->spare.buf = &buf2;
    out->spare.size=1;
    out->spare.bits_unused = 7;
    
    asn_encode_to_new_buffer_result_t res = {NULL, {0, NULL, NULL}};
    res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_UL_CCCH_Message, &pdu);
    printf("%lu\n", sizeof(res));
    printf("%lu\n", strlen(static_cast<char*>(res.buffer)));
    free(res.buffer);
    memset(&res, 0, sizeof(res));
    res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_UL_CCCH_Message, &pdu);
    *buffer = static_cast<uint8_t*>(res.buffer);
    *len = res.result.encoded;

    if (*buffer == NULL) {
        fprintf(stderr, "Error encoding rrc pdu\n");
        exit(1);
    } else {
        fprintf(stderr, "Encoded pdu\n");
    }

    xer_fprint(stdout, &asn_DEF_UL_CCCH_Message, &pdu);
}

void RRCConnectionSetupComplete_coder(uint8_t **buffer, ssize_t *len, int transaction_id)
{
    UL_DCCH_Message_t pdu;
    
    memset(&pdu, 0, sizeof(pdu));
    pdu.message.present = UL_DCCH_MessageType_PR_c1;
    pdu.message.choice.c1.present = UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete;
    auto out = &pdu.message.choice.c1.choice.rrcConnectionSetupComplete;
    
    out->rrc_TransactionIdentifier = transaction_id;
    out->criticalExtensions.present = RRCConnectionSetupComplete__criticalExtensions_PR_NOTHING; // не успел, поэтому nothing
    
    asn_encode_to_new_buffer_result_t res = {NULL, {0, NULL, NULL}};
    res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_UL_DCCH_Message, &pdu);
    printf("%lu\n", sizeof(res));
    printf("%lu\n", strlen(static_cast<char*>(res.buffer)));
    free(res.buffer);
    memset(&res, 0, sizeof(res));
    res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_UL_DCCH_Message, &pdu);
    *buffer = static_cast<uint8_t*>(res.buffer);
    *len = res.result.encoded;

    if (*buffer == NULL) {
        fprintf(stderr, "Error encoding rrc pdu\n");
        exit(1);
    } else {
        fprintf(stderr, "Encoded pdu\n");
    }

    xer_fprint(stdout, &asn_DEF_UL_DCCH_Message, &pdu);
}

DL_CCCH_Message_t* DL_CCCH_Message_decoder(uint8_t *buffer, ssize_t len) {
    DL_CCCH_Message_t *pdu = nullptr;
    
    asn_decode(nullptr, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_DL_CCCH_Message,
               reinterpret_cast<void**>(&pdu), static_cast<void*>(buffer), len);

    xer_fprint(stdout, &asn_DEF_DL_CCCH_Message, pdu);
    
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