#include "PMIP.h"
/*
int main(int argc, char *argv[])
{
 //PMIP start accept service
	
	unsigned char* ueMACValue;
	uint32_t ueIPValue;
	uint8_t causeValue;
	functest(causeValue);
    return 1;
}
*/
//memproxy call PMIP in memproxy
int pmipSessionTerminated(unsigned char* ueMACValue, uint32_t ueIPValue,
        uint8_t causeValue) {
    int sockfd = 0;
    struct sockaddr_in servaddr;

    icd msg = { .code = 1, .length = htons(sizeof(icd)), .messageIDType =
            (uint8_t) 11, .messageIDLength = sizeof(msg.messageIDValue),
            .messageTypeType = 12, .messageTypeLength =
                    sizeof(msg.messageTypeValue), .messageTypeValue = 31,
            .ueDetailsType = 19, .ueDetailsLength = sizeof(msg.ueDetailsValue),
            .ueDetailsValue.ueMACType = 13, .ueDetailsValue.ueMACLength =
                    sizeof(msg.ueDetailsValue.ueMACValue),
            .ueDetailsValue.ueIPType = 14, .ueDetailsValue.ueIPLength =
                    sizeof(msg.ueDetailsValue.ueIPValue), .causeType = 15,
            .causeLength = sizeof(msg.causeValue), .causeValue = 43 };

    msg.causeValue = causeValue;

    msg.messageIDValue = htonl(time(NULL ));
    memcpy(msg.ueDetailsValue.ueMACValue, ueMACValue,
            sizeof(msg.ueDetailsValue.ueMACValue));
    msg.ueDetailsValue.ueIPValue = htonl(ueIPValue);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9876);//make it connect to 9876
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) == -1) {
        perror("inet_pton");
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
    }
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    return 1;
}

//PMIP call greyhound to delete UE (need PMIP module to verify this function, already test in memproxy via using memproxy to call greyhound)
int pmipSessionToTerminate(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue) {

    int sockfd = 0;
    struct sockaddr_in servaddr;

    icdttg msgttg = { .code = 3, .length = htons(sizeof(icdttg)), .messageIDType =
            (uint8_t) 11, .messageIDLength = sizeof(msgttg.messageIDValue),
            .messageTypeType = 12, .messageTypeLength =
                    sizeof(msgttg.messageTypeValue), .messageTypeValue = 31,
            .ueMACType = 13, .ueMACLength =
                    sizeof(msgttg.ueMACValue),
            .ueIPType = 14, .ueIPLength =
                    sizeof(msgttg.ueIPValue), .causeType = 15,
            .causeLength = sizeof(msgttg.causeValue), .causeValue = 41 };

    msgttg.causeValue = causeValue;

    msgttg.messageIDValue = htonl(time(NULL ));
    memcpy(msgttg.ueMACValue, ueMACValue,
            sizeof(msgttg.ueMACValue));
    msgttg.ueIPValue = htonl(ueIPValue);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6812);//make it connect to 6812 greyhound
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) == -1) {
        perror("inet_pton");
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {//udp
        perror("socket");
    }
    sendto(sockfd, &msgttg, sizeof(msgttg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    return 1;
}


