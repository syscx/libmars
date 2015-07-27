#ifndef PMIP_H_
#define PMIP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <sys/time.h>
//copy from mm
#include <stdbool.h>
#include <strings.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <dbus/dbus.h>

#include <glib.h>
#include <glib-object.h>


#ifndef CASSANDRA_HOST
#define CASSANDRA_HOST "127.0.0.1"
#endif // CASSANDRA_HOST
#ifndef CASSANDRA_PORT
#define CASSANDRA_PORT 9160
#endif // CASSANDRA_PORT
#ifndef CASSANDRA_KEYSPACE
#define CASSANDRA_KEYSPACE "wsg"
#endif // CASSANDRA_HOST

#define UE_SESS_CAUSE_INITIATED           0
#define UE_SESS_CAUSE_ROAMING             1
#define UE_SESS_CAUSE_TERMINATED          2
#define UE_SESS_CAUSE_TIMEOUT             3

#define UE_TERM_CAUSE_INACTIVE_TIMEOUT    68
#define UE_TERM_CAUSE_ADMIN_DELETE        53
#define UE_TERM_CAUSE_OTHER_CASE          43
#define UE_TERM_CAUSE_SESSION_TIMEOUT     42
typedef struct _icd {
    uint8_t code;
    uint16_t length;
    uint8_t messageIDType;
    uint8_t messageIDLength;
    uint32_t messageIDValue;
    uint8_t messageTypeType;
    uint8_t messageTypeLength;
    uint8_t messageTypeValue;
    uint8_t ueDetailsType;
    uint8_t ueDetailsLength;
    struct {
        uint8_t ueMACType;
        uint8_t ueMACLength;
        unsigned char ueMACValue[6];
        uint8_t ueIPType;
        uint8_t ueIPLength;
        uint32_t ueIPValue;
    }__attribute__ ((packed)) ueDetailsValue;
    uint8_t causeType;
    uint8_t causeLength;
    uint8_t causeValue;
}__attribute__ ((packed)) icd;

typedef struct _icdttg {
    uint8_t code;
    uint16_t length;
    uint8_t messageIDType;
    uint8_t messageIDLength;
    uint32_t messageIDValue;
    uint8_t messageTypeType;
    uint8_t messageTypeLength;
    uint8_t messageTypeValue;
        uint8_t ueMACType;
        uint8_t ueMACLength;
        unsigned char ueMACValue[6];
        uint8_t ueIPType;
        uint8_t ueIPLength;
        uint32_t ueIPValue;
    uint8_t causeType;
    uint8_t causeLength;
    uint8_t causeValue;
}__attribute__ ((packed)) icdttg;
typedef struct _PMIPv6GlobalSetting {
dbus_uint32_t interval;
dbus_uint32_t retry;
dbus_uint32_t BindingRefreshTime;
}__attribute__ ((packed)) PMIPv6GlobalSetting;

typedef struct _PMIPv6Profile {
char id[64];
char ip1[64];
char ip2[64];
char mn[64];
char mac[64];
char apn[64];
char mvnoid[64];
}__attribute__ ((packed)) PMIPv6Profile;

typedef struct _PMIPv6ProfileAndWlanMapping {
char zone[64];
char wlan[64];
char id[64];
char mvnoid[64];
char opRealm[255];
char apssid[255];
}__attribute__ ((packed)) PMIPv6ProfileAndWlanMapping;

typedef struct _ClusterIPList {
char clusterip[64];
int port;//do not use in pmip, value ignore,reserve for future
}__attribute__ ((packed)) ClusterIPList;

int pmipSessionTerminated(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue);
int pmipSessionToTerminate(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue);
int pmipSessionToTerminateAP(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue,unsigned char* apMACValue);
void* PMIPListenDbus();
void* udplistener();
void udpCallbackSet(void (*funcpmipSessionTerminatedNotify)(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue));
void (*PMIPv6GlobalSettingUpdated)(int intr , int ret, int BindingRefreshTime);
void (*PMIPv6ProfileUpdated)(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid);
void (*PMIPv6ProfileRemoved)(char* profile);
void (*PMIPv6WlanUpdated)(char* zone, char* wlanid, char* pmipprofile, char* mvnoid, char *opRealm, char *apssid, int cnrIntf);
void (*PMIPv6WlanRemoved)(char* zone, char* wlanid);
void pmipCallbackSet(void (*funcPMIPv6GlobalSettingUpdated)(int intr , int ret, int BindingRefreshTime),
                     void (*funcPMIPv6ProfileUpdated)(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid), 
                     void (*funcPMIPv6ProfileRemoved)(char* profile),
                     void (*funcPMIPv6WlanUpdated)(char* zone, char* wlanid, char* pmipprofile, char* mvnoid, char *opRealm, char *apssid, int cnrIntf),
                     void (*funcPMIPv6LicenseChange)(int license),
                     void (*funcPMIPv6LogChange)(int loglevel),
                     void (*funcPMIPv6WlanRemoved)(char* zone, char* wlanid));
void (*PMIPv6ProfileUpdated)(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid);
void (*PMIPv6WlanUpdated)(char* zone, char* wlanid, char* pmipprofile, char* mvnoid, char *opRealm, char *apssid, int cnrIntf);
void (*PMIPv6ProfileRemoved)(char* profile);
void (*PMIPv6GlobalSettingUpdated)(int intr , int ret, int BindingRefreshTime);
PMIPv6GlobalSetting getPMIPv6GlobalSetting();
void getAllPMIPv6Profile(PMIPv6Profile** profilearray,int* arraycount);
void getAllPMIPv6ProfileAndWlanMapping(PMIPv6ProfileAndWlanMapping** profilearray ,int* arraycount);
void getClusterIPList(ClusterIPList** profilearray ,int* arraycount);
void dbconnect(GPtrArray *columns,char* columnfamily ,char* key);
void genColumn(GPtrArray *columns, const char* columnName, const char* columnValue, guint columnValueSize);
int directSend(int socketfd, char *buf, int bufLen, int sendTimewaitInSecond);
int directRecv(int socketfd, char *buf, int bufLen, int recvTimewaitInSecond);
void getUELicense(int *ueLicenseMode ,int *pmipUECount ,int *tunnel_pmipv6);
int updateNodeLicense(char* bladeId, int count);
int revokeLicense(char *contrlid, int num);
int assignLicense(char *contrlid, int num);
char* getControlBladeId();
void reply_to_method_call(DBusMessage* msg, DBusConnection* conn);
DBusMessage* wsgClusterMethodCall(const char* methodName);
void query(char* param);
int getPMIPv6LogLevel();
int match(char *a, char *b);
int getActiveLicense();
int resetNodeLicense();
int setMemKeyVal(char* key, char* val);
void memc_cleanup(void);
void memcmake();
void writepmipv6LMASignalingStats(char* dBladeId,char* lmaIp, char* mvnoId,long pbuPkts,long pbaPkts,long pbuLifetime0Pkts,long pbaLifetime0Pkts,long briPkts,long braPkts,long totalPkts,long recordCreationTime,long recordUpdateTime);
void writepmipv6LMAConnectivityStats(char* dBladeId, char* mvnoId ,char* primaryLMAIp,char* secondaryLMAIp,char* activeLMAIp,long primaryLMADuration,long secondaryLMADuration,long numOfFailOverPrimaryToSecondary,long numOfFailOverSecondaryToPrimary,long lastFailoverTime,long recordCreationTime,long recordUpdateTime);
void WritetoPmipv6UeSessionInfo(char* dpMac,char* ueMac, char* ueIpAddress ,char* apMac,char* magControlIp,char* magDataIp,char* lmaIp,long sessionStartTime,int cause,char* mvnoid);
void glibInit();
#endif /* PMIP_H_ */
