#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <chrono>
#include <thread>

#include <UL-CCCH-Message.h>

#include "ue_rrc.h"

static void die(const char *s) {
    perror(s);
    exit(1);
}

int main() {
    printf("ue launched\n");
    int flags = 0;
    struct sctp_sndrcvinfo sndrcvinfo;
    
    const int ue_port = 3456;
    struct sockaddr_in ueaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr("127.0.0.1"), // localhost
        .sin_port = htons(ue_port),
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
    auto ret = bind(listen_fd, (struct sockaddr *) &ueaddr, sizeof(ueaddr));
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

    // готовим и отправляем RRCConnectionRequest----------------------------
    uint8_t *buffer;
    ssize_t len;
    RRCConnectionRequest_coder(&buffer, &len);
    
    const int enb_port = 6543;
    struct sockaddr_in enbaddr = {
        .sin_family = AF_INET,
        .sin_port = htons(enb_port),
        .sin_addr.s_addr = inet_addr("127.0.0.1"), // localhost
    };

    int conn_fd;

    conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (conn_fd < 0) {
        printf("Error when opening socket\n");
        exit(1);
    }

    ret = connect(conn_fd, (struct sockaddr *)&enbaddr, sizeof(enbaddr));
    if (ret < 0) {
        printf("Error when connecting socket\n");
        exit(1);
    }

    ret = sctp_sendmsg(conn_fd, buffer, len, NULL, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        printf("Error when sending msg\n");
        exit(1);
    }

    close(conn_fd);

    printf("Sent RRCConnectionRequest packet\n");
    
    free(buffer);
        
    const int buf_size = 1024;
    buffer = new uint8_t[buf_size];
     // ждём RRCConnectionSetup ----------------------------------
     int transaction_id;
    while (true) {
        printf("Waiting for RRCConnectionSetup packet\n");
        
        printf("Waiting for connection\n");
        fflush(stdout);

        conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL);
        if (conn_fd < 0) {
            die("accept()");
        }

        printf("New client connected\n");
        fflush(stdout);
        
        ret = sctp_recvmsg(conn_fd, static_cast<void*>(buffer), buf_size, NULL, 0, &sndrcvinfo, &flags);
        close(conn_fd);
        auto pdu = DL_CCCH_Message_decoder(buffer, ret);
        if (ret > 0 && pdu->message.present == DL_CCCH_MessageType_PR_c1
                && pdu->message.choice.c1.present == DL_CCCH_MessageType__c1_PR_rrcConnectionSetup) {
            transaction_id = pdu->message.choice.c1.choice.rrcConnectionSetup.rrc_TransactionIdentifier;
            printf("Received RRCConnectionSetup packet\n");
            ASN_STRUCT_FREE(asn_DEF_DL_CCCH_Message, pdu);
            break;
        }
        ASN_STRUCT_FREE(asn_DEF_DL_CCCH_Message, pdu);
    }
    delete[] buffer;
    buffer = nullptr;
    
    // готовим и отправляем RRCConnectionSetupComplete--------------------------
    
    RRCConnectionSetupComplete_coder(&buffer, &len, transaction_id);
    
    conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (conn_fd < 0) {
        printf("Error when opening socket\n");
        exit(1);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ret = connect(conn_fd, (struct sockaddr *)&enbaddr, sizeof(enbaddr));
    if (ret < 0) {
        printf("Error when connecting socket\n");
        exit(1);
    }
    
    ret = sctp_sendmsg(conn_fd, buffer, len, NULL, 0, 0, 0, 0, 0, 0);
    if (ret < 0) {
        printf("Error when sending msg\n");
        exit(1);
    }

    close(conn_fd);
    
    printf("ue: sent RRCConnectionSetupComplete, we are done");
    
    return 0;
}
