#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>

#include <DL-CCCH-Message.h>
#include <UL-CCCH-Message.h>

#include "enb_rrc.h"

static void die(const char *s) {
    perror(s);
    exit(1);
}

int main() {
    printf("eNB launched\n");
    int flags=0;
    struct sctp_sndrcvinfo sndrcvinfo;
    const int enb_port = 6543;
    struct sockaddr_in enbaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr("127.0.0.1"), // localhost
        .sin_port = htons(enb_port),
    };
    struct sctp_initmsg initmsg = {
        .sinit_num_ostreams = 5,
        .sinit_max_instreams = 5,
        .sinit_max_attempts = 4,
    };
    auto listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listen_fd < 0) {
        die("socket");
    }
    auto ret = bind(listen_fd, (struct sockaddr *) &enbaddr, sizeof(enbaddr));
    if (ret < 0) {
        die("bind");
    }
    ret = setsockopt(listen_fd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
    if (ret < 0) {
        die("setsockopt");
    }
    ret = listen(listen_fd, initmsg.sinit_max_instreams);
    if (ret < 0) {
        die("listen");
    }
    int conn_fd;
    
   // ждём RRCConnectionRequest -------------------------------------- 
    while (true) { 
        uint8_t buffer[1024];
        printf("eNB: waiting for RRCConnectionRequest\n");
        printf("Waiting for connection\n");
        fflush(stdout);

        conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL);
        if (conn_fd < 0) {
            die("accept()");
        }

        printf("New client connected\n");
        fflush(stdout);

        auto in = sctp_recvmsg(conn_fd, static_cast<void*>(buffer), sizeof(buffer), nullptr, nullptr, &sndrcvinfo, &flags);
        auto pdu = UL_CCCH_Message_decoder(buffer, in);
        close(conn_fd);
        if (in > 0 && pdu->message.present == UL_CCCH_MessageType_PR_c1
                && pdu->message.choice.c1.present == UL_CCCH_MessageType__c1_PR_rrcConnectionRequest) {
            printf("eNB: received RRCConnectionRequest packet:\n");
            ASN_STRUCT_FREE(asn_DEF_UL_CCCH_Message, pdu);
            break;
        }
        ASN_STRUCT_FREE(asn_DEF_UL_CCCH_Message, pdu);
    }
    
    // готовим и отправляем RRCConnectionSetup ------------------------------------
    
    uint8_t *buffer;
    ssize_t len;
    RRCConnectionSetup_coder(&buffer, &len);
    
    const int ue_port = 3456;
    struct sockaddr_in ueaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr("127.0.0.1"), // localhost
        .sin_port = htons(ue_port),
    };
 
    conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (conn_fd < 0) {
        printf("Error when opening socket\n");
        exit(1);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    ret = connect(conn_fd, (struct sockaddr *)&ueaddr, sizeof(ueaddr));
    if (ret < 0) {
        printf("Error when connecting socket\n");
        exit(1);
    }
    
    ret = sctp_sendmsg(conn_fd, buffer, len, NULL, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        printf("eNB: error when sending RRCConnectionSetup packet\n");
        exit(1);
    }
    
    printf("eNB: sent RRCConnectionSetup\n");
    
    free(buffer);
    
    close(conn_fd);
    
    // ждём RRCConnectionSetupComplete------------------------------------
    while (true) {
        uint8_t buffer[1024];
        printf("eNB: waiting for RRCConnectionSetupComplete\n");
        printf("Waiting for connection\n");
        fflush(stdout);

        conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL);
        if (conn_fd < 0) {
            die("accept()");
        }

        printf("New client connected\n");
        fflush(stdout);

        auto in = sctp_recvmsg(conn_fd, static_cast<void*>(buffer), sizeof(buffer), nullptr, nullptr, &sndrcvinfo, &flags);
        auto pdu = UL_DCCH_Message_decoder(buffer, in);
        close(conn_fd);
        if (in > 0 && pdu->message.present == UL_DCCH_MessageType_PR_c1
                && pdu->message.choice.c1.present == UL_DCCH_MessageType__c1_PR_rrcConnectionSetupComplete) {
            printf("eNB: received RRCConnectionSetupComplete packet, we are done\n");
            ASN_STRUCT_FREE(asn_DEF_UL_DCCH_Message, pdu);
            break;
        }
        else {
            ASN_STRUCT_FREE(asn_DEF_UL_DCCH_Message, pdu);
        }
    }
    
    return 0;
}
