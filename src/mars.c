#include "PMIP.h"
#include "IP.h"
#include "thriftmars.h"
#include <ctype.h>

#include <stdio.h>

#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "libmemcached/memcached.h"
#define MEMCACHED_DEFAULT_PORT 11211

static memcached_st *memc = NULL;
static memcached_server_st *servers = NULL;

void (*pmipSessionTerminatedNotify)(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue) = NULL;

void (*PMIPv6GlobalSettingUpdated)(int intr , int ret, int BindingRefreshTime) = NULL;
void (*PMIPv6ProfileUpdated)(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid) = NULL;
void (*PMIPv6ProfileRemoved)(char* profile) = NULL;
void (*PMIPv6WlanUpdated)(char* zone, char* wlanid, char* pmipprofile, char* mvnoid, char *opRealm, char *apssid, int cnrIntf) = NULL;
void (*PMIPv6LicenseChange)(int license) = NULL;
void (*PMIPv6LogChange)(int loglevel) = NULL;
void (*PMIPv6WlanRemoved)(char* zone, char* wlanid) = NULL;
char *strnupper(char *dest, const char *src, size_t n)
{
	char *d;

	bzero(dest, n);

	for (d = dest; *src && (n > 0); n--) {
		*d++ = toupper(*src++);
	}
	return dest;
}
void longtochar(char* buf,long pbuPkts)
{
	buf[0] = (int)((pbuPkts >> 56) & 0xFF) ;
	buf[1] = (int)((pbuPkts >> 48) & 0xFF) ;
	buf[2] = (int)((pbuPkts >> 40) & 0XFF);
	buf[3] = (int)((pbuPkts >> 32) & 0XFF);
	buf[4] = (int)((pbuPkts >> 24) & 0xFF) ;
	buf[5] = (int)((pbuPkts >> 16) & 0xFF) ;
	buf[6] = (int)((pbuPkts >> 8) & 0XFF);
	buf[7] = (int)((pbuPkts & 0XFF));
}
void inttochar(char* buf,int pbuPkts)
{
	buf[0] = (int)((pbuPkts >> 24) & 0xFF) ;
	buf[1] = (int)((pbuPkts >> 16) & 0xFF) ;
	buf[2] = (int)((pbuPkts >> 8) & 0XFF);
	buf[3] = (int)((pbuPkts & 0XFF));
}
void udpCallbackSet(void (*funcpmipSessionTerminatedNotify)(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue))
{
    pmipSessionTerminatedNotify = funcpmipSessionTerminatedNotify;
}

void pmipCallbackSet(void (*funcPMIPv6GlobalSettingUpdated)(int intr , int ret, int BindingRefreshTime),
                     void (*funcPMIPv6ProfileUpdated)(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid), 
                     void (*funcPMIPv6ProfileRemoved)(char* profile),
                     void (*funcPMIPv6WlanUpdated)(char* zone, char* wlanid, char* pmipprofile, char* mvnoid, char* opRealm, char *apssid, int cnrIntf),
                     void (*funcPMIPv6LicenseChange)(int license),
                     void (*funcPMIPv6LogChange)(int loglevel),
                     void (*funcPMIPv6WlanRemoved)(char* zone, char* wlanid))
{
    PMIPv6GlobalSettingUpdated = funcPMIPv6GlobalSettingUpdated;
    PMIPv6ProfileUpdated = funcPMIPv6ProfileUpdated;
    PMIPv6ProfileRemoved = funcPMIPv6ProfileRemoved;
    PMIPv6WlanUpdated = funcPMIPv6WlanUpdated;
	PMIPv6LicenseChange = funcPMIPv6LicenseChange;
	PMIPv6LogChange = funcPMIPv6LogChange;
    PMIPv6WlanRemoved = funcPMIPv6WlanRemoved;
}

int directSend(int socketfd, char *buf, int bufLen, int sendTimewaitInSecond) {
    int n = 0, count = 0, totalCount = 0;
    int epollfd = 0;
    struct epoll_event event;
    struct epoll_event *epollevents = NULL;

    if ((epollfd = epoll_create1(0)) == -1) {
        //printf("%s\n", strerror(errno));
        return -1;
    }
    bzero(&event, sizeof(struct epoll_event));
    event.events = EPOLLOUT;
    event.data.fd = socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        //printf( "%s\n", strerror(errno));
        if (epollfd > 0) {
            close(epollfd);
        }
        return -1;
    }
    if (!(epollevents = calloc(1, sizeof(struct epoll_event)))) {
        //printf( "Out of memory!\n");
        if (epollfd > 0) {
            close(epollfd);
        }
        return -1;
    }

    for (; totalCount < bufLen; count = 0) {
        if ((n = epoll_wait(epollfd, epollevents, 1,
                sendTimewaitInSecond * 1000)) > 0) {
            if ((epollevents[0].events & EPOLLERR)
                    || (epollevents[0].events & EPOLLHUP)) {
                if (epollfd > 0) {
                    close(epollfd);
                }
                if (epollevents) {
                    free(epollevents);
                }
                return -1;
            }
            if (epollevents[0].events & EPOLLOUT) {
                count = send(socketfd, buf + totalCount, bufLen - totalCount,
                        0);
                if (count == -1) {
                    //printf( "%s\n", strerror(errno));
                    if (epollfd > 0) {
                        close(epollfd);
                    }
                    if (epollevents) {
                        free(epollevents);
                    }
                    return -1;
                } else if (count == 0) {
                    //printf( "%s\n", strerror(errno));
                    if (epollfd > 0) {
                        close(epollfd);
                    }
                    if (epollevents) {
                        free(epollevents);
                    }
                    return -1;
                } else {
                    if (count > 0) {
                        totalCount += count;
                    }
                }
            }
        } else if (n == 0) {
            //printf( "Time out! %s\n", strerror(errno));
            if (epollfd > 0) {
                close(epollfd);
            }
            if (epollevents) {
                free(epollevents);
            }
            return -1;
        } else {
            //printf( "%s\n", strerror(errno));
            if (epollfd > 0) {
                close(epollfd);
            }
            if (epollevents) {
                free(epollevents);
            }
            return -1;
        }
    }
    if (epollfd > 0) {
        close(epollfd);
    }
    if (epollevents) {
        free(epollevents);
    }
    return 1;
}

int directRecv(int socketfd, char *buf, int bufLen, int recvTimewaitInSecond) {
    int n = 0, count = 0, totalCount = 0;
    int epollfd = 0;
    struct epoll_event event;
    struct epoll_event *epollevents = NULL;

    if ((epollfd = epoll_create1(0)) == -1) {
        //printf( "%s\n", strerror(errno));
        return -1;
    }
    bzero(&event, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = socketfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
        //printf( "%s\n", strerror(errno));
        if (epollfd > 0) {
            close(epollfd);
        }
        return -1;
    }
    if (!(epollevents = calloc(1, sizeof(struct epoll_event)))) {
        //printf( "Out of memory!\n");
        if (epollfd > 0) {
            close(epollfd);
        }
        return -1;
    }

    for (; totalCount < bufLen; count = 0) {
        if ((n = epoll_wait(epollfd, epollevents, 1,
                recvTimewaitInSecond * 1000)) > 0) {
            if ((epollevents[0].events & EPOLLERR)
                    || (epollevents[0].events & EPOLLHUP)) {
                if (epollfd > 0) {
                    close(epollfd);
                }
                if (epollevents) {
                    free(epollevents);
                }
                return -1;
            }

            if (epollevents[0].events & EPOLLIN) {
                count = recv(socketfd, buf + totalCount, bufLen - totalCount,
                        0);
                if (count == -1) {
                    if (errno != EAGAIN) {
                        //printf( "%s\n", strerror(errno));
                        if (epollfd > 0) {
                            close(epollfd);
                        }
                        if (epollevents) {
                            free(epollevents);
                        }
                        return -1;
                    } else {
                        continue;
                    }
                } else if (count == 0) {
                    //printf( "Connection closed by peer! %s\n",  strerror(errno));
                    if (epollfd > 0) {
                        close(epollfd);
                    }
                    if (epollevents) {
                        free(epollevents);
                    }
                    return -1;
                } else {
                    if (count > 0) {
                        totalCount += count;
                    }
                }
            }
        } else if (n == 0) {
            //printf( "Time out! %s\n", strerror(errno));
            if (epollfd > 0) {
                close(epollfd);
            }
            if (epollevents) {
                free(epollevents);
            }
            return -1;
        } else {
            //printf( "%s\n", strerror(errno));
            if (epollfd > 0) {
                close(epollfd);
            }
            if (epollevents) {
                free(epollevents);
            }
            return -1;
        }
    }
    if (epollfd > 0) {
        close(epollfd);
    }
    if (epollevents) {
        free(epollevents);
    }
    return 1;
}
void genBean(GPtrArray *columns, const char* columnName, const char* columnValue, guint columnValueSize, gint64 timestamp) {
    Mutation *mutation = g_object_new(TYPE_MUTATION, 0);
    if (mutation) {
        mutation->__isset_column_or_supercolumn = TRUE;
        ColumnOrSuperColumn *columnOrSuperColumn =
                mutation->column_or_supercolumn;
        columnOrSuperColumn->__isset_column = TRUE;
        columnOrSuperColumn->column->name = NULL;
        Column *column = columnOrSuperColumn->column;
        column->name = g_byte_array_new();
        g_byte_array_append(column->name, (const guint8 *) columnName,
                strlen(columnName));
        column->__isset_value = TRUE;
        column->value = g_byte_array_new();
        g_byte_array_append(column->value, (const guint8 *) columnValue,
                columnValueSize);
        column->__isset_timestamp = TRUE;
        column->timestamp = timestamp;
        column->__isset_ttl = TRUE;
        column->ttl = 259200;
        g_ptr_array_add(columns, mutation);
    } else {
		//printf("Failed to create Mutation object!\n");
    }
}

void genColumn(GPtrArray *columns, const char* columnName, const char* columnValue, guint columnValueSize) {
    struct timeval x;
    gettimeofday(&x, NULL );
    gint64 timestamp = (double) x.tv_sec * 1000000 + (double) x.tv_usec;
    Mutation *mutation = g_object_new(TYPE_MUTATION, 0);
    if (mutation) {
        mutation->__isset_column_or_supercolumn = TRUE;
        ColumnOrSuperColumn *columnOrSuperColumn =
                mutation->column_or_supercolumn;
        columnOrSuperColumn->__isset_column = TRUE;
        columnOrSuperColumn->column->name = NULL;
        Column *column = columnOrSuperColumn->column;
        column->name = g_byte_array_new();
        g_byte_array_append(column->name, (const guint8 *) columnName,
                strlen(columnName));
        column->__isset_value = TRUE;
        column->value = g_byte_array_new();
        g_byte_array_append(column->value, (const guint8 *) columnValue,
                columnValueSize);
        column->__isset_timestamp = TRUE;
        column->timestamp = timestamp;
        column->__isset_ttl = TRUE;
        column->ttl = 259200;
        g_ptr_array_add(columns, mutation);
    } else {
		//printf("Failed to create Mutation object!\n");
    }
}
int thriftConnectionReset(thriftConnection *thrift) {
    thriftConnectionTearDown(thrift);
    return thriftConnectionSetup(thrift);
}
void freeDb(GHashTable *mutationMap,GHashTable *mutationMapCF,GError *gError,InvalidRequestException *ire
    ,UnavailableException *ue,TimedOutException *te,thriftConnection thrift)
{
    GHashTableIter iter;
    gpointer freekey = NULL;
    gpointer value = NULL;
    g_hash_table_iter_init(&iter, mutationMap);
    while (g_hash_table_iter_next(&iter, &freekey, &value)) {
		GByteArray *unclientMacKey = (GByteArray *) freekey;
        g_byte_array_unref(unclientMacKey);
        GHashTableIter mutationMapCFIter;
        gpointer mutationMapCFKey = NULL;
        gpointer mutationMapCFValue = NULL;
        GHashTable *mutationMapCF = (GHashTable *) value;
        g_hash_table_iter_init(&mutationMapCFIter, mutationMapCF);
        while (g_hash_table_iter_next(&mutationMapCFIter, &mutationMapCFKey, &mutationMapCFValue)) {
			GPtrArray *columns = mutationMapCFValue;
            g_ptr_array_set_free_func(columns, g_object_unref);
            g_ptr_array_unref(columns);
        }
        g_hash_table_unref(mutationMapCF);
    }
    g_hash_table_unref(mutationMap);
    if (gError) {
		if (ire) {
			//printf("Invalid request exception: %s\n",ire->why);
            g_object_unref(ire);
        } else if (ue) {
			//printf( "Unavailable exception\n");
            g_object_unref(ue);
        } else if (te) {
            //printf("Timed out exception\n");
            g_object_unref(te);
        } else {
			thriftConnectionTearDown(&thrift);
        }
    }
    else
		thriftConnectionTearDown(&thrift);
}
//pmipv6UeSessionInfo
void WritetoPmipv6UeSessionInfo(char* dpMac,char* ueMac, char* ueIpAddress ,char* apMac,char* magControlIp,char* magDataIp,char* lmaIp,
gint64 sessionStartTime,int cause,char* mvnoid)
{
    struct timeval x;
    gettimeofday(&x, NULL );
    gint64 timestamp = (double) x.tv_sec * 1000000 + (double) x.tv_usec;

	char* columnfamily = "pmipv6UeSessionInfo";
	char key[120];
	bzero(key,120);
    snprintf(key, sizeof(key), "%lu_%s", timestamp, ueMac);
    //printf("[%s] trying to write key %s to db\n", __func__, key);
	g_type_init();
    GPtrArray *columns = g_ptr_array_new();   
    
    char macarray[18];
    strnupper(macarray, ueMac, sizeof(macarray));
    genColumn(columns, "ueMac", macarray, 17);
    strnupper(macarray, dpMac, sizeof(macarray));
    genColumn(columns, "dpMac", macarray, 17);
    genColumn(columns, "ueIpAddress", ueIpAddress, strlen(ueIpAddress));

    strnupper(macarray, apMac, sizeof(macarray));
    genColumn(columns, "apMac", macarray, 17);
    genColumn(columns, "mvnoId", mvnoid, strlen(mvnoid));
    genColumn(columns, "magControlIp", magControlIp, strlen(magControlIp));
    genColumn(columns, "magDataIp", magDataIp, strlen(magDataIp));
    genColumn(columns, "lmaIp", lmaIp, strlen(lmaIp));

    
    char buf[128];

    bzero(buf, sizeof(buf));
	longtochar(buf,sessionStartTime*1000);
	if(sessionStartTime==0)
	{
		//use current time to replace
		bzero(buf, sizeof(buf));
		longtochar(buf,timestamp/1000);
	}
    genBean(columns, "sessionStartTime", buf, 8, timestamp);


    bzero(buf, sizeof(buf));
	inttochar(buf,cause);
    genBean(columns, "cause", buf, 4, timestamp);
    
//	long test = 0; 
	//printf("%lu \n",test);
    thriftConnection thrift;
    if (thriftConnectionSetup(&thrift) == -1) {
		//printf("connection fail \n");
        thriftConnectionTearDown(&thrift);
        return;
    }
	//printf("[%s] prepare mutuation map\n", __func__);
    GHashTable *mutationMap = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *mutationMapCF = g_hash_table_new(g_str_hash, g_str_equal);

	//printf("prepare pmipkeytester\n");
    GByteArray *clientMacKey = g_byte_array_new();
    g_byte_array_append(clientMacKey, (const guint8 *) key, strlen(key));

	//printf("prepare \n");

    InvalidRequestException *ire = NULL;
    UnavailableException *ue = NULL;
    TimedOutException *te = NULL;
    GError *gError = NULL;

	//printf("prepare columnfamily\n");
    g_hash_table_insert(mutationMapCF, (gpointer) columnfamily, columns);
    g_hash_table_insert(mutationMap, (gpointer) clientMacKey, mutationMapCF);
	//printf("end columnfamily\n");

    cassandra_client_set_keyspace(thrift.service, CASSANDRA_KEYSPACE, &ire, &gError);
	cassandra_if_batch_mutate(thrift.service, mutationMap, CONSISTENCY_LEVEL_ONE, &ire, &ue, &te, &gError);
//Free it
	freeDb(mutationMap,mutationMapCF,gError,ire,ue,te,thrift);
}

void writepmipv6LMAConnectivityStats(char* dBladeId, char* mvnoId ,char* primaryLMAIp,char* secondaryLMAIp,char* activeLMAIp,
long primaryLMADuration,long secondaryLMADuration,long numOfFailOverPrimaryToSecondary,long numOfFailOverSecondaryToPrimary,long lastFailoverTime
,long recordCreationTime,long recordUpdateTime)
{
	char* columnfamily = "pmipv6LMAConnectivityStats";
	char key[120];
    char macarray[18];
    strnupper(macarray, dBladeId, sizeof(macarray));
	
    snprintf(key, sizeof(key), "%s_%s_%s", macarray, primaryLMAIp, secondaryLMAIp);
    //printf("[pmipv6LMAConnectivityStats]Trying to write key %s to db\n",key);

	g_type_init();
    GPtrArray *columns = g_ptr_array_new();   

    genColumn(columns, "dBladeId", macarray,17);
    genColumn(columns, "mvnoId", mvnoId, strlen(mvnoId));
    genColumn(columns, "primaryLMAIp", primaryLMAIp, strlen(primaryLMAIp));
    genColumn(columns, "secondaryLMAIp", secondaryLMAIp, strlen(secondaryLMAIp));
    genColumn(columns, "activeLMAIp", activeLMAIp, strlen(activeLMAIp));

    
    char buf[128];

    struct timeval x;
    gettimeofday(&x, NULL );
    gint64 timestamp = (double) x.tv_sec * 1000000 + (double) x.tv_usec;

    bzero(buf, sizeof(buf));
	longtochar(buf,primaryLMADuration);
    genBean(columns, "primaryLMADuration", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,secondaryLMADuration);    
    genBean(columns, "secondaryLMADuration", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,numOfFailOverPrimaryToSecondary);
    genBean(columns, "numOfFailOverPrimaryToSecondary", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,numOfFailOverSecondaryToPrimary);
    genBean(columns, "numOfFailOverSecondaryToPrimary", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,lastFailoverTime*1000);
    genBean(columns, "lastFailoverTime", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,recordUpdateTime*1000);
    genBean(columns, "recordUpdateTime", buf, 8, timestamp);

    bzero(buf, sizeof(buf));
	longtochar(buf,recordCreationTime*1000);
    genBean(columns, "recordCreationTime", buf, 8, timestamp);

//	long test = 0; 
	//printf("%lu \n",test);
    thriftConnection thrift;
    if (thriftConnectionSetup(&thrift) == -1) {
		//printf("connection fail \n");
        thriftConnectionTearDown(&thrift);
        return;
    }
	//printf("prepare mutuation map[writepmipv6LMAConnectivityStats]\n");
    GHashTable *mutationMap = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *mutationMapCF = g_hash_table_new(g_str_hash, g_str_equal);

	//printf("prepare pmipkeytester\n");
    GByteArray *clientMacKey = g_byte_array_new();
    g_byte_array_append(clientMacKey, (const guint8 *) key, strlen(key));

    InvalidRequestException *ire = NULL;
    UnavailableException *ue = NULL;
    TimedOutException *te = NULL;
    GError *gError = NULL;

	//printf("prepare columnfamily\n");
    g_hash_table_insert(mutationMapCF, (gpointer) columnfamily, columns);
    g_hash_table_insert(mutationMap, (gpointer) clientMacKey, mutationMapCF);
	//printf("end columnfamily\n");

    cassandra_client_set_keyspace(thrift.service, CASSANDRA_KEYSPACE, &ire, &gError);

	cassandra_if_batch_mutate(thrift.service, mutationMap, CONSISTENCY_LEVEL_ONE, &ire, &ue, &te, &gError);
//Free it
	freeDb(mutationMap,mutationMapCF,gError,ire,ue,te,thrift);
}
//write to pmipv6LMASignalingStats
void writepmipv6LMASignalingStats(char* dBladeId,char* lmaIp, char* mvnoId,
long pbuPkts,long pbaPkts,long pbuLifetime0Pkts,long pbaLifetime0Pkts,long briPkts,long braPkts,long totalPkts,
long recordCreationTime,long recordUpdateTime){
	char* columnfamily = "pmipv6LMASignalingStats";
	char key[120];

    char macarray[18];
    strnupper(macarray, dBladeId, sizeof(macarray));

    snprintf(key, sizeof(key), "%s_%s", macarray,lmaIp);
    //printf("[writepmipv6LMASignalingStats]Trying to write key %s to db\n",key);

	g_type_init();
    GPtrArray *columns = g_ptr_array_new();   
    genColumn(columns, "dBladeId", macarray,17);
    genColumn(columns, "lmaIp", lmaIp, strlen(lmaIp));
    genColumn(columns, "mvnoId", mvnoId, strlen(mvnoId));
    
    char buf[128];

    struct timeval x;
    gettimeofday(&x, NULL );
    gint64 timestamp = (double) x.tv_sec * 1000000 + (double) x.tv_usec;

    bzero(buf, sizeof(buf));
	longtochar(buf,pbuPkts);
    genBean(columns, "pbuPkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,pbaPkts);
    genBean(columns, "pbaPkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,pbuLifetime0Pkts);
    genBean(columns, "pbuLifetime0Pkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,pbaLifetime0Pkts);
    genBean(columns, "pbaLifetime0Pkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,briPkts);
    genBean(columns, "briPkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,braPkts);
    genBean(columns, "braPkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,totalPkts);
    genBean(columns, "totalPkts", buf, 8, timestamp);
    
    bzero(buf, sizeof(buf));
	longtochar(buf,recordCreationTime*1000);
    genBean(columns, "recordCreationTime", buf, 8, timestamp); 
    
    bzero(buf, sizeof(buf));
	longtochar(buf,recordUpdateTime*1000);
    genBean(columns, "recordUpdateTime", buf, 8, timestamp);

//	long test = 0; 
	//printf("%lu \n",test);
    thriftConnection thrift;
    if (thriftConnectionSetup(&thrift) == -1) {
		//printf("connection fail \n");
        thriftConnectionTearDown(&thrift);
        return;
    }
	//printf("prepare mutuation map[writepmipv6LMASignalingStats]\n");
    GHashTable *mutationMap = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *mutationMapCF = g_hash_table_new(g_str_hash, g_str_equal);

	//printf("prepare pmipkeytester\n");
    GByteArray *clientMacKey = g_byte_array_new();
    g_byte_array_append(clientMacKey, (const guint8 *) key, strlen(key));

	//printf("prepare \n");

    InvalidRequestException *ire = NULL;
    UnavailableException *ue = NULL;
    TimedOutException *te = NULL;
    GError *gError = NULL;

	//printf("prepare columnfamily\n");
    g_hash_table_insert(mutationMapCF, (gpointer) columnfamily, columns);
    g_hash_table_insert(mutationMap, (gpointer) clientMacKey, mutationMapCF);
	//printf("end columnfamily\n");

    cassandra_client_set_keyspace(thrift.service, CASSANDRA_KEYSPACE, &ire, &gError);
	cassandra_if_batch_mutate(thrift.service, mutationMap, CONSISTENCY_LEVEL_ONE, &ire, &ue, &te, &gError);
//Free it
	freeDb(mutationMap,mutationMapCF,gError,ire,ue,te,thrift);
}
void dbconnect(GPtrArray *columns,char* columnfamily ,char* key){
	g_type_init();
//	long test = 0; 
	//printf("%lu \n",test);
    thriftConnection thrift;
    if (thriftConnectionSetup(&thrift) == -1) {
		//printf("connection fail \n");
        thriftConnectionTearDown(&thrift);
        return;
    }
	//printf("prepare mutuation map[dbconnect]\n");
    GHashTable *mutationMap = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *mutationMapCF = g_hash_table_new(g_str_hash, g_str_equal);

	//printf("prepare pmipkeytester\n");
    GByteArray *clientMacKey = g_byte_array_new();
    g_byte_array_append(clientMacKey, (const guint8 *) key, strlen(key));

	//printf("prepare TESTERMAC\n");

    InvalidRequestException *ire = NULL;
    UnavailableException *ue = NULL;
    TimedOutException *te = NULL;
    GError *gError = NULL;

	//printf("prepare apClient\n");
    g_hash_table_insert(mutationMapCF, (gpointer) columnfamily, columns);
    g_hash_table_insert(mutationMap, (gpointer) clientMacKey, mutationMapCF);
	//printf("end apClient\n");

    cassandra_client_set_keyspace(thrift.service, CASSANDRA_KEYSPACE, &ire, &gError);
	cassandra_if_batch_mutate(thrift.service, mutationMap, CONSISTENCY_LEVEL_ONE, &ire, &ue, &te, &gError);
//Free it
	freeDb(mutationMap,mutationMapCF,gError,ire,ue,te,thrift);
	

}

int thriftConnectionSetup(thriftConnection *thrift) {
    GError *gError = NULL;
    InvalidRequestException *ire = NULL;

    if (!(thrift->tsocket =
            THRIFT_SOCKET(
                    g_object_new(THRIFT_TYPE_SOCKET, "hostname", CASSANDRA_HOST, "port", CASSANDRA_PORT, 0)))) {
        thriftConnectionTearDown(thrift);
        return -1;
    } else if (!(thrift->transport =
            THRIFT_TRANSPORT(
                    g_object_new(THRIFT_TYPE_FRAMED_TRANSPORT, "transport", thrift->tsocket, 0)))) {
        thriftConnectionTearDown(thrift);
        return -1;
    } else if (!(thrift->protocol =
            THRIFT_PROTOCOL(
                    g_object_new(THRIFT_TYPE_BINARY_PROTOCOL, "transport", thrift->transport, 0)))) {
        thriftConnectionTearDown(thrift);
        return -1;
    } else if (!(thrift->client =
            CASSANDRA_CLIENT(
                    g_object_new(TYPE_CASSANDRA_CLIENT, "input_protocol", thrift->protocol, "output_protocol", thrift->protocol, 0)))) {
        thriftConnectionTearDown(thrift);
        return -1;
    } else if (!(thrift->service = CASSANDRA_IF(thrift->client))) {
        thriftConnectionTearDown(thrift);
        return -1;
    } else {
        if (!thrift_transport_open(thrift->transport, &gError) || gError) {
            g_error_free(gError);
            gError = NULL;
            thriftConnectionTearDown(thrift);
            return -1;
        } else if (!thrift_transport_is_open(thrift->transport)) {
            thriftConnectionTearDown(thrift);
            return -1;
        } else {
            struct linger l;
            l.l_onoff = 1;
            l.l_linger = 0;
            int tcpNoDelay = 1;
            int keepalive = 1;
            struct timeval s = { 1, 0 };
            if (setsockopt(thrift->tsocket->sd, SOL_SOCKET, SO_KEEPALIVE,
                    (int*) &keepalive, sizeof(int))) {
                thriftConnectionTearDown(thrift);
                return -1;
            }
            if (setsockopt(thrift->tsocket->sd, SOL_SOCKET, SO_LINGER, &l,
                    sizeof(l)) == -1) {
                thriftConnectionTearDown(thrift);
                return -1;
            }
            if (setsockopt(thrift->tsocket->sd, IPPROTO_TCP, TCP_NODELAY,
                    (char *) &tcpNoDelay, sizeof(tcpNoDelay)) == -1) {
                thriftConnectionTearDown(thrift);
                return -1;
            }
            if ((setsockopt(thrift->tsocket->sd, SOL_SOCKET, SO_RCVTIMEO, &s,
                    sizeof(s))) == -1) {
                thriftConnectionTearDown(thrift);
                return -1;
            }
            if ((setsockopt(thrift->tsocket->sd, SOL_SOCKET, SO_RCVTIMEO, &s,
                    sizeof(s))) == -1) {
                thriftConnectionTearDown(thrift);
                return -1;
            }
        }
        cassandra_client_set_keyspace(thrift->service, CASSANDRA_KEYSPACE, &ire,
                &gError);
        if (gError) {
            if (ire) {
                g_object_unref(ire);
            }
            g_error_free(gError);
            gError = NULL;
            thriftConnectionTearDown(thrift);
            return -1;
        }
    }
    return 0;
}

void thriftConnectionTearDown(thriftConnection *thrift) {
    GError *gError = NULL;
    if (thrift->transport) {
        if (!thrift_transport_close(thrift->transport, &gError) || gError) {
            //printf( "Failed to close Cassandra connection! '%s'\n",     gError->message);
            g_error_free(gError);
        }
    }
    if (thrift->client) {
        g_object_unref(thrift->client);
        thrift->client = NULL;
    }
    if (thrift->protocol) {
        g_object_unref(thrift->protocol);
        thrift->protocol = NULL;
    }
    if (thrift->transport) {
        g_object_unref(thrift->transport);
        thrift->transport = NULL;
    }
    if (thrift->tsocket) {
        g_object_unref(thrift->tsocket);
        thrift->tsocket = NULL;
    }
    if (thrift->service) {
        thrift->tsocket = NULL;
    }
}

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
        return 0;
    }
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    close(sockfd);
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
        return 0;
    }
    sendto(sockfd, &msgttg, sizeof(msgttg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    close(sockfd);
    return 1;
}
//PMIP call greyhound to delete UE (need PMIP module to verify this function, already test in memproxy via using memproxy to call greyhound)
int pmipSessionToTerminateAP(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue,unsigned char* apMACValue) {
	memcached_return_t memRet = MEMCACHED_SUCCESS;
    uint8_t *ueMACValue8 = (uint8_t *) ueMACValue;

	char *getValue = NULL;
	size_t getValueLength = 0;
	uint32_t getFlags = 0;
	
    char buf[256];
    bzero(buf,sizeof(buf) );
    snprintf(buf, sizeof(buf),"rcks:%02x%02x%02x%02x%02x%02x", ueMACValue8[0],ueMACValue8[1],ueMACValue8[2],ueMACValue8[3],ueMACValue8[4],ueMACValue8[5]);

    //printf("the key is %s\n\n",buf);

	getValue = memcached_get(memc, buf, 17, &getValueLength, &getFlags, &memRet);
	if(getValue==NULL)
	{
		//printf("go 2 branch");
		return 0;
	}
    uint8_t *old_scache = (uint8_t *) getValue;


	if(memcmp(old_scache + 70, apMACValue, 6)==0 &&ntohs(*((uint16_t *) (old_scache + 78))) != 0)
	{	
		//printf("match");
		pmipSessionToTerminate(ueMACValue, ueIPValue, causeValue) ;
		return 1;
	}
	else
	{
		//printf("not match");
		return 0;
	}
    
    return 1;
}

void* udplistener()
{
	int sockfd,n;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t len;
	icd msg;

	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd<0)
		return 0;

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "br1", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
 
	
    if (inet_pton(AF_INET, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), &servaddr.sin_addr) == -1) {
        perror("inet_pton");
    }
	//printf("Listen to memproxy %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return 0;
    }

	servaddr.sin_port=htons(9876);
	bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

	for (;;)
	{
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cliaddr, &len);
		/* Simple message validition check, only by message length */
		if (n < sizeof(icd))
			continue;
		if (pmipSessionTerminatedNotify) {
			(*pmipSessionTerminatedNotify)(msg.ueDetailsValue.ueMACValue, 
                                         msg.ueDetailsValue.ueIPValue, 
                                         msg.causeValue);
		}
	}
	close(sockfd);
}

int get_ipaddr(char *ifname, uint32_t *addr_ptr)
{
	int sock;
	struct ifreq ifr;

	*addr_ptr = 0;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) 
		return -1;

	memset(&ifr, 0x00, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);
	if (!ioctl(sock, SIOCGIFADDR, &ifr)) {
		*addr_ptr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
	}
	close(sock);
	
	return 0;
}

DBusMessage* wsgMethodCall(const char* methodName) {
    DBusMessage* msg;
    DBusMessage* replyMsg = NULL;
    DBusError err;
    dbus_error_init(&err);
	DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	//printf("begin method call xx %s\n",methodName);

    if ((msg = dbus_message_new_method_call("com.ruckuswireless.greyhound", "/", "com.ruckuswireless.greyhound",
            methodName)) == NULL) {
		//printf("Message from method_call:%s is Null.\n", methodName);
        return NULL;
    } else {
        while (replyMsg == NULL) {
            replyMsg = dbus_connection_send_with_reply_and_block(conn, msg,
                    10000, &err);
            dbus_connection_flush(conn);
            if (dbus_error_is_set(&err)) {
				//5004 Config fail
/*
//get config fail should not raised 5004
				int level = LOG_CRIT;
				char mgmt_mac[6];
				uint32_t mgmt_ip;	
				get_macaddr("br0", mgmt_mac, sizeof(mgmt_mac));
				get_ipaddr("br2", &mgmt_ip);
				syslog(level, "@@%4d, \"ctrlBladeMac\"=\""MACADDRCOLONFMT 
				"\", \"SCGMgmtIp\"=\""IPADDRFMT "\", Dbus Error Config fail!",
				5004, MACADDR(mgmt_mac), 
				IPADDR(&mgmt_ip));
*/
                //printf("'%s' '%s'\n", err.name, err.message);
                dbus_error_free(&err);
            } else {
                break;
            }
            sleep(5);
        }
        dbus_message_unref(msg);
        return replyMsg;
    }
}
void reply_to_method_call(DBusMessage* msg, DBusConnection* conn)
{
   DBusMessage* reply;
   DBusMessageIter args;
   bool stat = true;
   dbus_uint32_t level = 21614;
   dbus_uint32_t serial = 0;
   char* param = "";

   // read the arguments
   if (!dbus_message_iter_init(msg, &args))
      fprintf(stderr, "Message has no arguments!\n"); 
   else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
      fprintf(stderr, "Argument is not string!\n"); 
   else 
      dbus_message_iter_get_basic(&args, &param);

   //printf("Method called with %s\n", param);

   // create a reply from the message
   reply = dbus_message_new_method_return(msg);

   // add the arguments to the reply
   dbus_message_iter_init_append(reply, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &stat)) { 
      fprintf(stderr, "Out Of Memory!\n"); 
      exit(1);
   }
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &level)) { 
      fprintf(stderr, "Out Of Memory!\n"); 
      exit(1);
   }

   // send the reply && flush the connection
   if (!dbus_connection_send(conn, reply, &serial)) {
      fprintf(stderr, "Out Of Memory!\n"); 
      exit(1);
   }
   dbus_connection_flush(conn);

   // free the reply
   dbus_message_unref(reply);
}

//This not yet finish, need to put cluster into it
DBusMessage* wsgClusterMethodCall(const char* methodName) {
    DBusMessage* msg = NULL;
    DBusMessage* replyMsg = NULL;
    DBusError err;
    dbus_error_init(&err);
	DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	//printf("begin method call x %s\n",methodName);
    if ((msg = dbus_message_new_method_call("com.ruckuswireless.cluster", "/", "com.ruckuswireless.cluster",
            "getAllBladeState")) == NULL) {
		//printf("Message from method_call:%s is Null.\n", methodName);
        return NULL;
    } else {
        while (replyMsg == NULL) {
			//printf("checking ");			
            replyMsg = dbus_connection_send_with_reply_and_block(conn, msg,
                    10000, &err);
            dbus_connection_flush(conn);
            if (dbus_error_is_set(&err)) {
				//printf("xxxx3 ");
                //printf("'%s' '%s'\n", err.name, err.message);
                dbus_error_free(&err);
            } else {
                break;
            }
            sleep(5);
        }
        dbus_message_unref(msg);
        return replyMsg;
    }
}
//getPMIPv6GlobalSetting
PMIPv6GlobalSetting getPMIPv6GlobalSetting()
{
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;

    replyMsg = wsgMethodCall("getPMIPv6GlobalSetting");

    /* This data is returned and used by the caller. */
    static PMIPv6GlobalSetting pmipgs = { .interval=1,.retry=2 ,.BindingRefreshTime=3};

	//printf("finish call\n");
    if(replyMsg && dbus_message_iter_init(replyMsg, &args)) 
    {
		char* signature = dbus_message_iter_get_signature(&args);		    
        if ( signature && !memcmp(signature, "(uuu)", strlen("(uuu)") + 1)) 
        {
            if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) 
            {
				dbus_message_iter_recurse(&args, &structIter);
				dbus_uint32_t val0;
				dbus_message_iter_get_basic(&structIter, &val0);
				pmipgs.interval=val0;
				//printf("pmipgs.interval = %u\n", val0);
				if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_UINT32)) 
				{
					dbus_uint32_t val;
					dbus_message_iter_get_basic(&structIter, &val);
					pmipgs.retry=val;
					//printf("pmipgs.retry = %u\n", val);
				}
				if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_UINT32)) 
				{
					dbus_uint32_t val2;
					dbus_message_iter_get_basic(&structIter, &val2);
					pmipgs.BindingRefreshTime=val2;
					//printf("pmipgs.BindingRefreshTime = %u\n", val2);
				}
			}
        } 
        else 
        {
            if ( signature && !memcmp(signature, "as", strlen("as") + 1)) 
            {
                //printf( "empty table!\n");
            }
            else 
            {
				//printf("signature doesn't match! '%s' \n", signature);
            }
        }
        if( signature )
			dbus_free(signature);
        dbus_message_unref(replyMsg);
        return pmipgs;//return true
    } 
    else 
    {
        return pmipgs;//return false
    }
}

void getUELicense(int* ueLicenseMode , int* pmipUECount , int* tunnel_pmipv6) {
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;

    replyMsg = wsgMethodCall("getUELicense");

    if (replyMsg && dbus_message_iter_init(replyMsg, &args)) {
		char* sigval = dbus_message_iter_get_signature(&args);
        if ( sigval && !memcmp(sigval, "(ubbbbbbbbusbubbbbbuuuuubu)",
                strlen("(ubbbbbbbbusbubbbbbuuuuubu)") + 1)) {
            if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) {
            	dbus_message_iter_recurse(&args, &structIter);
				char* val;
				dbus_message_iter_get_basic(&structIter, &val);//1
				dbus_message_iter_get_basic(&structIter, &val);//2
				dbus_message_iter_get_basic(&structIter, &val);//3
				dbus_message_iter_get_basic(&structIter, &val);//4
				dbus_message_iter_get_basic(&structIter, &val);//5
				dbus_message_iter_get_basic(&structIter, &val);//6
				dbus_message_iter_get_basic(&structIter, &val);//7
				dbus_message_iter_get_basic(&structIter, &val);//8
				dbus_message_iter_get_basic(&structIter, &val);//9
				dbus_message_iter_get_basic(&structIter, &val);//10
				dbus_message_iter_get_basic(&structIter, &val);//11
				dbus_message_iter_get_basic(&structIter, &val);//12
				dbus_message_iter_get_basic(&structIter, &val);//13
				dbus_message_iter_get_basic(&structIter, &val);//14
				dbus_message_iter_get_basic(&structIter, &val);//15
				dbus_message_iter_get_basic(&structIter, &val);//16
				dbus_message_iter_get_basic(&structIter, &val);//17
				dbus_message_iter_get_basic(&structIter, &val);//18
				dbus_message_iter_get_basic(&structIter, &val);//19
				dbus_message_iter_get_basic(&structIter, &val);//20
				dbus_message_iter_get_basic(&structIter, &val);//21
				dbus_message_iter_get_basic(&structIter, &val);//22*
				
				int ueLicenseMode1=0;
				dbus_message_iter_get_basic(&structIter, &ueLicenseMode1);//23 we need
				//printf("getUELicense ueLicenseMode !!!! %i\n", ueLicenseMode1);
				*ueLicenseMode = ueLicenseMode1;
				int pmipUECount1=0;
				dbus_message_iter_get_basic(&structIter, &pmipUECount1);//24
				//printf("getUELicense pmipUECount %u\n", pmipUECount1);
				*pmipUECount = pmipUECount1;
				int tunnel_pmipv6_1 = 0;
				dbus_message_iter_get_basic(&structIter, &tunnel_pmipv6_1);//25 we need
				//printf("testgetAllPMIPv6Profile1 boolean %i\n", tunnel_pmipv6_1);
				*tunnel_pmipv6 = tunnel_pmipv6_1;
            }
            dbus_free(sigval);
        } else {
            //printf("signature doesn't match! '%s' \n", dbus_message_iter_get_signature(&args));
        }
        dbus_message_unref(replyMsg);
    } else {
    }
}
int getPMIPv6LogLevel()
{
	int loglevel = 0;
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;

    replyMsg = wsgMethodCall("getPMIPv6LogLevel"); 


	//printf("start test getPMIPv6LogLevel call\n");
    if(replyMsg && dbus_message_iter_init(replyMsg, &args)) 
    {
		dbus_message_iter_recurse(&args, &structIter);
		dbus_uint16_t val0;
        dbus_message_iter_get_basic(&structIter, &val0);
        dbus_message_iter_next(&structIter);
        dbus_message_iter_get_basic(&structIter, &loglevel);
        //printf( "val0 %d\n",val0);
		//printf( "loglevel%d\n",loglevel);
        dbus_message_unref(replyMsg);
    }
	if(loglevel == 2)
	{loglevel=3;}
	else if(loglevel==3)
	{loglevel=4;}
	else if(loglevel==4)
	{loglevel=6;}
	else if(loglevel==5)
	{loglevel=7;}
	else
	{loglevel=7;}
    return loglevel;
}
//getAllPMIPv6ProfileAndWlanMapping
void getClusterIPList(ClusterIPList** profilearray ,int* arraycount)
{
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;
    DBusMessageIter arrayIiter;

    DBusMessageIter args1;
    DBusMessageIter arrayIiter1;

    replyMsg = wsgMethodCall("getClusterIPList");

	int counter = 0;
	
	if(replyMsg && dbus_message_iter_init(replyMsg, &args1)) 
    {
        if (!memcmp(dbus_message_iter_get_signature(&args1), "a(si)", strlen("a(si)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args1)) 
            {
                dbus_message_iter_recurse(&args1, &arrayIiter1);
				while (dbus_message_iter_get_arg_type(&arrayIiter1) != DBUS_TYPE_INVALID) 
				{
					counter++;
					//printf("getAllPMIPv6ProfileAndWlanMapping size %i x\n",counter);
					dbus_message_iter_next(&arrayIiter1);
				}
				//printf("continue next array \n");
			}
		}
	}
	*arraycount=counter;

	/* This data is returned and used, and should be freed by the caller. */
	ClusterIPList *clusterlist = calloc(counter, sizeof(ClusterIPList));
	//printf("Allocated Cluster IP List %p, count %d, total size %ld\n",  clusterlist, counter, counter * sizeof(ClusterIPList));
	if (!clusterlist) {
		*profilearray = NULL;
		*arraycount = 0;
		return;
	}

    replyMsg = wsgMethodCall("getClusterIPList"); 


	int currentpt = 0;
	//printf("start test getClusterIPList call\n");
    if(replyMsg && dbus_message_iter_init(replyMsg, &args)) 
    {
        if (!memcmp(dbus_message_iter_get_signature(&args), "a(si)", strlen("a(si)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args)) 
            {
                dbus_message_iter_recurse(&args, &arrayIiter);
				while (dbus_message_iter_get_arg_type(&arrayIiter) != DBUS_TYPE_INVALID) 
				{

					if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&arrayIiter)) 
					{
						dbus_message_iter_recurse(&arrayIiter, &structIter);
						char* val0;
						dbus_message_iter_get_basic(&structIter, &val0);
						//printf("========getClusterIPList String %s==========\n", val0);						
						bzero(clusterlist[currentpt].clusterip,64);
						strcpy(clusterlist[currentpt].clusterip, val0);
						currentpt++;
						dbus_message_iter_next(&arrayIiter);
					}
				}
			}
        } 
        else 
        {
            if (!memcmp(dbus_message_iter_get_signature(&args), "as", strlen("as") + 1)) 
            {
                //printf( "empty table!\n");
            }
            else 
            {
				//printf("signature doesn't match! '%s' \n", dbus_message_iter_get_signature(&args));
            }
        }
        dbus_message_unref(replyMsg);

		*profilearray = clusterlist;

		//printf("Address of a1: %p\n", profilearray);
    } 
    else 
    {
		*profilearray = clusterlist;
		//printf("Address of a1: %p\n", profilearray);
    }
}

void getNodeState(char* noderesult)
{
    DBusMessage* replyMsg = NULL;

    DBusMessageIter args1;
    DBusMessageIter arrayIiter1;
    replyMsg = wsgClusterMethodCall("getBladeID");
    
    char buf[256];
    bzero(buf,sizeof(buf));
	strcpy(buf, "");
    
	if(replyMsg && dbus_message_iter_init(replyMsg, &args1)) 
    {

  	    while (dbus_message_iter_get_arg_type(&args1) != DBUS_TYPE_INVALID) 
		{
			dbus_message_iter_recurse(&args1, &arrayIiter1);
			if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayIiter1))
			{

			   DBusMessageIter dictIter;           
			   dbus_message_iter_recurse(&arrayIiter1, &dictIter);//Initialize iterator for dictentry

			   if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&dictIter))
			   {

				char* val0;
				dbus_message_iter_get_basic(&dictIter, &val0);//Read nodeid
				strcat(buf, val0);
				strcat(buf, ",");
				//printf("ready to save to %s\n",buf);
				dbus_message_iter_next(&dictIter);//Go to next argument of dictentry
				if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&dictIter))
				{
				   dbus_message_iter_get_basic(&dictIter, &val0);//Read state
				}
			   }
			}
			dbus_message_iter_next(&args1);
		}
	}
	strcpy(noderesult, buf);
}
//1.read file 
//2.getControlBladeId
//3.setMemKeyVal PMIPList :  blade1,blade2,blade3,blade4,
char* getControlBladeId()
{
	//key PMIPList
	static char tester[100];
	bzero(tester,100);
    char stringlist[256];
    bzero(stringlist,sizeof(stringlist) );
	getNodeState(stringlist);
	    char buf[256];
    bzero(buf,sizeof(buf) );
    snprintf(buf, sizeof(buf), "%s", stringlist);
	//printf("obtain controlbladeid %s \n",stringlist);	

	memcmake();
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	char* key = "PMIPList";
	long unsigned int keyLen = strlen(key); /* 6 bytes MAC address in binary */
	char *getValue = NULL;
	size_t getValueLength = 0;
	uint32_t getFlags = 0;
    if ((memRet = memcached_set(memc, key, strlen(key), buf, strlen(buf), (time_t) 0,(uint32_t) 0)) != MEMCACHED_SUCCESS) 
    {
    }
	getValue = memcached_get(memc, key, keyLen, &getValueLength, &getFlags, &memRet);
	//printf("the result is %s\n\n",getValue);
	
	//READ FILE
	// /opt/ruckuswireless/wsg/conf/cluster.properties
       FILE * fp;
       char * line = NULL;
       size_t len = 0;
       ssize_t read;

       fp = fopen("/opt/ruckuswireless/wsg/conf/cluster.properties", "r");
       if (fp == NULL)
           exit(EXIT_FAILURE);

       while ((read = getline(&line, &len, fp)) != -1) {
           //printf("Retrieved line of length %zu :\n", read);
           //printf("%s", line);
		if(match(line,"blade.id=")!=-1)
		{
			//printf("found char %s\n",(line+9));
			snprintf(tester,100,"%s",(line+9));
			tester[36]='\0';
			free(line);
			return tester;
         }
       }

       if (line)
           free(line);
//	memc_cleanup();
	return tester;

}
int match(char *a, char *b)
{
   int position = 0;
   char *x, *y;
 
   x = a;
   y = b;
 
   while(*a)
   {
      while(*x==*y)
      {
         x++;
         y++;
         if(*x=='\0'||*y=='\0')
            break;         
      }   
      if(*y=='\0')
         break;
 
      a++;
      position++;
      x = a;
      y = b;
   }
   if(*a)
      return position;
   else   
      return -1;   
}
//getAllPMIPv6ProfileAndWlanMapping
void getAllPMIPv6ProfileAndWlanMapping(PMIPv6ProfileAndWlanMapping** profilearray ,int* arraycount)
{
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;
    DBusMessageIter arrayIiter;

    DBusMessageIter args1;
    DBusMessageIter arrayIiter1;
    
    replyMsg = wsgMethodCall("getAllPMIPv6ProfileAndWlanMapping");

	int counter = 0;
	
	if(replyMsg && dbus_message_iter_init(replyMsg, &args1)) 
    {
		char* signature0 = dbus_message_iter_get_signature(&args1);
        if ( signature0 && !memcmp(signature0, "a(ssss)", strlen("a(ssss)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args1)) 
            {
                dbus_message_iter_recurse(&args1, &arrayIiter1);
				while (dbus_message_iter_get_arg_type(&arrayIiter1) != DBUS_TYPE_INVALID) 
				{
					counter++;
					//printf("getAllPMIPv6ProfileAndWlanMapping size %i x\n",counter);
					dbus_message_iter_next(&arrayIiter1);
				}
				//printf("continue next array \n");
			}
		}
		if ( signature0 )
			dbus_free(signature0);
		
	}
	*arraycount=counter;

	/* This data is returned and used, and should be freed by the caller. */
	PMIPv6ProfileAndWlanMapping *profmap = calloc(counter, sizeof(PMIPv6ProfileAndWlanMapping));
	//printf("Allocated PMIPv6 Profile and WLAN Mapping %p, count %d, total size %ld\n",  profmap, counter, counter * sizeof(PMIPv6ProfileAndWlanMapping));
	if (!profmap) {
		*profilearray = NULL;
		*arraycount = 0;
		return;
	}

    replyMsg = wsgMethodCall("getAllPMIPv6ProfileAndWlanMapping"); 


	int currentpt = 0;
	//printf("start test getAllPMIPv6ProfileAndWlanMapping call\n");
    if(replyMsg && dbus_message_iter_init(replyMsg, &args)) 
    {
		char* signature1=dbus_message_iter_get_signature(&args);
        if ( signature1 && !memcmp(signature1, "a(ssss)", strlen("a(ssss)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args)) 
            {
                dbus_message_iter_recurse(&args, &arrayIiter);
				while (dbus_message_iter_get_arg_type(&arrayIiter) != DBUS_TYPE_INVALID) 
				{

					if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&arrayIiter)) 
					{
						dbus_message_iter_recurse(&arrayIiter, &structIter);
						char* val0;
						dbus_message_iter_get_basic(&structIter, &val0);
						//printf("testgetAllPMIPv6Profile0x %s\n", val0);
						bzero(profmap[currentpt].zone,64);
						strcpy(profmap[currentpt].zone, val0);
						//printf("pass zone obtain %s\n",profmap[currentpt].zone);
						//2
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val;
							dbus_message_iter_get_basic(&structIter, &val);
							//printf("testgetAllPMIPv6Profile1 %s\n", val);
							bzero(profmap[currentpt].wlan,64);
							strcpy(profmap[currentpt].wlan, val);
							//printf("pass wlan obtain %s\n",profmap[currentpt].wlan);
						}
//3
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val2;
							dbus_message_iter_get_basic(&structIter, &val2);
							//printf("testgetAllPMIPv6Profile2 %s\n", val2);
							bzero(profmap[currentpt].id,64);
							strcpy(profmap[currentpt].id, val2);
							//printf("pass id obtain %s\n",profmap[currentpt].id);
						}
//4
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val3;
							dbus_message_iter_get_basic(&structIter, &val3);
							//printf("testgetAllPMIPv6Profile2 %s\n", val3);
							bzero(profmap[currentpt].mvnoid,64);
							strcpy(profmap[currentpt].mvnoid, val3);
							//printf("pass mvnoid obtain %s\n",profmap[currentpt].mvnoid);
						}
						currentpt++;
						dbus_message_iter_next(&arrayIiter);
					}
				}
			}
        } 
        else 
        {
			char* signature2=dbus_message_iter_get_signature(&args);
            if ( signature2 && !memcmp(signature2, "as", strlen("as") + 1)) 
            {
                //printf( "empty table!\n");
            }
            else 
            {
				//printf("signature doesn't match! '%s' \n", signature2);
            }
            if( signature2 )
				dbus_free(signature2);
        }
        dbus_message_unref(replyMsg);

		*profilearray = profmap;

		//printf("Address of a1: %p\n", profilearray);
    } 
    else 
    {
		*profilearray = profmap;
		//printf("Address of a1: %p\n", profilearray);
    }
}
//getPMIPv6Profile
void getAllPMIPv6Profile(PMIPv6Profile** profilearray ,int* arraycount)
{
    DBusMessage* replyMsg = NULL;
    DBusMessageIter args;
    DBusMessageIter structIter;
    DBusMessageIter arrayIiter;

    DBusMessageIter args1;
    DBusMessageIter arrayIiter1;

    replyMsg = wsgMethodCall("getAllPMIPv6Profile");

	int counter = 0;
	
	if(replyMsg && dbus_message_iter_init(replyMsg, &args1)) 
    {
		char* signature=dbus_message_iter_get_signature(&args1);
        if ( signature && !memcmp(signature, "a(sssssss)", strlen("a(sssssss)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args1)) 
            {
                dbus_message_iter_recurse(&args1, &arrayIiter1);
				while (dbus_message_iter_get_arg_type(&arrayIiter1) != DBUS_TYPE_INVALID) 
				{
					counter++;
					//printf("[getAllPMIPv6Profile] Check Array size %i x\n",counter);
					dbus_message_iter_next(&arrayIiter1);
				}
				//printf("(Array)End of arrayIiter1 parsing, Continue next \n");
			}
			//printf("(All type)End of arrayIiter1 parsing, Continue next \n");
		}
		if( signature )
			dbus_free(signature);
		//printf("End of a(sssssss) signature check\n");
	}
	*arraycount=counter;

	/* This data is returned and used, and should be freed by the caller. */
	PMIPv6Profile *prof = calloc(counter, sizeof(PMIPv6Profile));
	//printf("Allocated PMIPv6 Profile %p, count %d, total size %ld\n",  prof, counter, counter * sizeof(PMIPv6ProfileAndWlanMapping));
	if (!prof) {
		*profilearray = NULL;
		*arraycount = 0;
		return;
	}

    replyMsg = wsgMethodCall("getAllPMIPv6Profile"); 


	int currentpt = 0;
	//printf("start test getAllPMIPv6Profile call\n");
    if(replyMsg && dbus_message_iter_init(replyMsg, &args)) 
    {
		char* signature1=dbus_message_iter_get_signature(&args);
        if ( signature1 && !memcmp(signature1, "a(sssssss)", strlen("a(sssssss)") + 1)) 
        {
            if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args)) 
            {
                dbus_message_iter_recurse(&args, &arrayIiter);
				while (dbus_message_iter_get_arg_type(&arrayIiter) != DBUS_TYPE_INVALID) 
				{

					if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&arrayIiter)) 
					{
						dbus_message_iter_recurse(&arrayIiter, &structIter);
						char* val0;
						dbus_message_iter_get_basic(&structIter, &val0);
						//printf("testgetAllPMIPv6Profile0x %s\n", val0);
						bzero(prof[currentpt].id,64);
						strcpy(prof[currentpt].id, val0);
						//printf("pass id obtain %s\n",prof[currentpt].id);
						//2
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val;
							dbus_message_iter_get_basic(&structIter, &val);
							//printf("testgetAllPMIPv6Profile1 %s\n", val);
							bzero(prof[currentpt].ip1,64);
							strcpy(prof[currentpt].ip1, val);
							//printf("pass ip1 obtain %s\n",prof[currentpt].ip1);
						}
//3
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val2;
							dbus_message_iter_get_basic(&structIter, &val2);
							//printf("testgetAllPMIPv6Profile2 %s\n", val2);
							bzero(prof[currentpt].ip2,64);
							strcpy(prof[currentpt].ip2, val2);
							//printf("pass ip2 obtain %s\n",prof[currentpt].ip2);
						}
						//mn
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val3;
							dbus_message_iter_get_basic(&structIter, &val3);
							//printf("testgetAllPMIPv6Profile2[mn] %s\n", val3);
							bzero(prof[currentpt].mn,64);
							strcpy(prof[currentpt].mn, val3);
							//printf("pass mn obtain %s\n",prof[currentpt].mn);
						}
						//mac
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val4;
							dbus_message_iter_get_basic(&structIter, &val4);
							//printf("testgetAllPMIPv6Profile2[mac] %s\n", val4);
							bzero(prof[currentpt].mac,64);
							strcpy(prof[currentpt].mac, val4);
							//printf("pass mac obtain %s\n",prof[currentpt].mac);
						}
						//apn
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val5;
							dbus_message_iter_get_basic(&structIter, &val5);
							//printf("testgetAllPMIPv6Profile2 [apn]%s\n", val5);
							bzero(prof[currentpt].apn,64);
							strcpy(prof[currentpt].apn, val5);
							//printf("pass apn obtain %s\n",prof[currentpt].apn);
						}
						//mvnoid
						if (dbus_message_iter_next(&structIter) && (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING)) 
						{
							char* val6;
							dbus_message_iter_get_basic(&structIter, &val6);
							//printf("testgetAllPMIPv6Profile2 [mvnoid]%s\n", val6);
							bzero(prof[currentpt].mvnoid,64);
							strcpy(prof[currentpt].mvnoid, val6);
							//printf("pass mvnoid obtain %s\n",prof[currentpt].mvnoid);
						}
						currentpt++;
						dbus_message_iter_next(&arrayIiter);
					}
				}
			}
			dbus_free(signature1);
        } 
        else 
        {
			char* signature1 =dbus_message_iter_get_signature(&args); 
            if ( signature1 && !memcmp(signature1, "as", strlen("as") + 1)) 
            {
                //printf( "empty table!\n");
            }
            else 
            {
				//printf("signature doesn't match! '%s' \n", signature1);
            }
            if( signature1 )
				dbus_free(signature1);
        }
        dbus_message_unref(replyMsg);

		*profilearray = prof;

		//printf("Address of a1: %p\n", profilearray);
    } 
    else 
    {
		*profilearray = prof;
		//printf("Address of a1: %p\n", profilearray);
    }
}

void* PMIPListenDbus()
{

    //printf("PMIPListenDbus beginning ...\n");
    DBusMessage* msg;
    DBusMessageIter args;
    DBusError err2;

	//printf( "checkDbus Begin \n");
   DBusConnection* conn;
   int ret;
   dbus_error_init(&err2); 
// connect to the bus
   conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err2);
   if (dbus_error_is_set(&err2)) { 
      //printf( "Connection Error 1 (%s)\n", err2.message); 
      dbus_error_free(&err2); 
   }
   if (NULL == conn) { 
      exit(1); 
   }
   //printf( "finish dbus_bus_get connection setup\n");
// request a name on the bus
   //printf( "Request a name called marslisten to handle dbus stuff(caller receiver)\n");
   ret = dbus_bus_request_name(conn, "com.ruckuswireless.marslisten", 
         DBUS_NAME_FLAG_REPLACE_EXISTING 
         , &err2);
   if (dbus_error_is_set(&err2)) { 
	  //printf("Name Error (%s)\n", err2.message); 
      dbus_error_free(&err2); 
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
      exit(1);
   }
    
   //call function to get greyhound pmip data

    DBusMessageIter structIter;
    char *sigvalue = NULL;
    
	// add a rule for which messages we want to see
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6ProfileUpdated'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6WlanRemoved'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='MWSGConfChanged'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6LogLevelChanged'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6WlanUpdated'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6ProfileRemoved'", &err2); // see signals from the given interface
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='PMIPv6GlobalSettingUpdated'", &err2); // see signals from the given interface   
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.greyhound',member='ClusterIPListChanged'", &err2); // see signals from the given interface   

   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.cluster',member='getBladeID'", &err2); // see signals from the given interface   
   dbus_bus_add_match(conn, "type='signal',interface='com.ruckuswireless.wsg.configurer.dbus.ConfigurerDBusService',member='getalivelist'", &err2); // see signals from the given interface   
//ClusterIPListChanged

   dbus_connection_flush(conn);//PMIPv6ProfileRemoved
   if (dbus_error_is_set(&err2)) { 
      //printf("Match Error (%s)\n", err2.message);
      exit(1); 
   }
   //printf( "Line 88 pass\n");
// loop listening for signals being emmitted
   while (true) {

      // non blocking read of the next available message
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      // loop again if we haven't read a message
      if (NULL == msg) { 
         sleep(1);
         continue;
      }
      //printf( "reading message...awaiting...\n");

      // check if the message is a signal from the correct interface and with the correct name
      if (dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6ProfileUpdated")) {
         // read the parameters
         char* sign1 = NULL;
         if (!dbus_message_iter_init(msg, &args))
         {
            //printf( "Message has no arguments!\n");
         }
         else if ( (sign1 =dbus_message_iter_get_signature(&args)) && !memcmp(sign1,"(sssssss)", strlen("(sssssss)") + 1)) 
         {
			 //args
			if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) 
			{
				char *profileid = NULL;
				char *primaryip = NULL;
				char *secondaryip = NULL;
				char *mn = NULL;
				char *mac = NULL;
				char *apn = NULL;
				char *mvnoid = NULL;
				
				dbus_message_iter_recurse(&args, &structIter);
                dbus_message_iter_get_basic(&structIter, &profileid);
                //printf( "profileid %s\n",profileid);
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &primaryip);
                ////printf( "primaryip %s\n",primaryip);
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &secondaryip);
                //printf( "secondaryip %s\n",secondaryip);
                
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &mn);
                //printf( "mn %s\n",mn);
                dbus_message_iter_next(&structIter);
                
                dbus_message_iter_get_basic(&structIter, &mac);
                //printf( "Mac %s\n",mac);
                dbus_message_iter_next(&structIter);
                
                dbus_message_iter_get_basic(&structIter, &apn);
                //printf( "APN %s\n",apn);
                dbus_message_iter_next(&structIter);                                

                dbus_message_iter_get_basic(&structIter, &mvnoid);
                //printf( "MVNOID %s\n",mvnoid);
                dbus_message_iter_next(&structIter);                                


                if (PMIPv6ProfileUpdated)
                    (*PMIPv6ProfileUpdated)(profileid,primaryip,secondaryip,mn,mac,apn,mvnoid);
            }
			//printf( "(PMIPListenDbus)PMIPv6ProfileUpdated finish !\n"); 
		 }
         else {
            dbus_message_iter_get_basic(&args, &sigvalue);
            //printf( "Got Signal with value %s\n", sigvalue);
         }
         if( sign1 )
			dbus_free(sign1);
	  }
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6WlanRemoved"))
	  {
		char *zone = NULL;
		char *wlanid = NULL;
         if (!dbus_message_iter_init(msg, &args))
         {}
            //printf( "Message has no arguments!\n");
        dbus_message_iter_get_basic(&args, &zone);
        //printf( "s1 %s\n",zone);
        dbus_message_iter_next(&args);
        dbus_message_iter_get_basic(&args, &wlanid);
        //printf( "s2 %s\n",wlanid);
        if (PMIPv6WlanRemoved && zone)
            (*PMIPv6WlanRemoved)(zone,wlanid);
	  }//PMIPv6LogLevelChanged
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6LogLevelChanged"))
	  {
         if (!dbus_message_iter_init(msg, &args))
         {
		 }
		 else
		 {
			//printf( "PMIPv6LogLevelChanged\n");

            if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) 
            {
				dbus_message_iter_recurse(&args, &structIter);
				dbus_uint16_t val0;
                dbus_message_iter_get_basic(&structIter, &val0);
                //printf( "val0 %d\n",val0);
				dbus_uint16_t val1;           
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &val1);
                //printf( "val1 %d\n",val1);
				if(val1 == 2)
				{val1=3;}
				else if(val1==3)
				{val1=4;}
				else if(val1==4)
				{val1=6;}
				else if(val1==5)
				{val1=7;}
				else
				{val1=7;}
				//printf( "change log level to %d\n",val1);
				if(PMIPv6LogChange)
					(*PMIPv6LogChange)(val1);

			}

		 }
	  }
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "MWSGConfChanged"))
	  {
         if (!dbus_message_iter_init(msg, &args))
         {
		 }
		 else
		 {
			char *content = NULL;
            //printf( "MWSGConfChanged\n");
			dbus_message_iter_get_basic(&args, &content);
			
			if (!memcmp(content, "UELicenseChanged", strlen("UELicenseChanged") + 1))
			{
				int ueLicenseMode=0;
				int pmipUECount=0;
				int tunnel_pmipv6=0;
				getUELicense(&ueLicenseMode,&pmipUECount,&tunnel_pmipv6);
				//Do UELicenseChanged 
				if(PMIPv6LicenseChange)
					(*PMIPv6LicenseChange)(pmipUECount);
			}
			
			
			//printf( "s1 %s\n",content);
			dbus_message_iter_next(&args);			 
		 }
	  }
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6WlanUpdated"))
	  {
		  //printf( "PMIPv6WlanUpdated !\n");
         // read the parameters
         char* sign1 = NULL;
         if (!dbus_message_iter_init(msg, &args))
         {
            //printf( "Message has no arguments!\n");
         }
         else if ( (sign1=dbus_message_iter_get_signature(&args)) && !memcmp(sign1,"(ssss)", strlen("(ssss)") + 1)) 
         {
			 //args
			if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) 
			{
				char *zone = NULL;
				char *wlanid = NULL;
				char *pmipprofile = NULL;
				char *mvnoid = NULL;
				dbus_message_iter_recurse(&args, &structIter);
                dbus_message_iter_get_basic(&structIter, &zone);
                //printf( "zone %s\n",zone);
                dbus_message_iter_next(&structIter);
                
                dbus_message_iter_get_basic(&structIter, &wlanid);
                //printf( "wlanid %s\n",wlanid);
                dbus_message_iter_next(&structIter);
                
                dbus_message_iter_get_basic(&structIter, &pmipprofile);
                //printf( "pmipprofile %s\n",pmipprofile);
                dbus_message_iter_next(&structIter);
                
                dbus_message_iter_get_basic(&structIter, &mvnoid);
                //printf( "pmipprofile %s\n",mvnoid);
                dbus_message_iter_next(&structIter);
                if (PMIPv6WlanUpdated)  
                    (*PMIPv6WlanUpdated)(zone,wlanid,pmipprofile,mvnoid, NULL, NULL, 0);
            }
			//printf( "Signature match !\n"); 
		 }
         else {
            dbus_message_iter_get_basic(&args, &sigvalue);
            //printf( "Got Signal with value %s\n", sigvalue);
         }
         if( sign1 )
			dbus_free(sign1);

	  }
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6ProfileRemoved"))
	  {
		char *profile = NULL;
         if (!dbus_message_iter_init(msg, &args))
         {}
            //printf( "Message has no arguments!\n");
		dbus_message_iter_get_basic(&args, &profile);
		if (PMIPv6ProfileRemoved)
                    PMIPv6ProfileRemoved(profile);
		//printf( "s1 %s\n",profile);
		//printf( "PMIPv6ProfileRemoved !\n");
	  }
	  else if(dbus_message_is_signal(msg, "com.ruckuswireless.greyhound", "PMIPv6GlobalSettingUpdated"))
	  {
		 char* sign1 =NULL;
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
         {
            //printf( "Message has no arguments!\n");
         }
         else if ( (sign1=(dbus_message_iter_get_signature(&args))) && !memcmp(sign1,"(uuu)", strlen("(uuu)") + 1)) 
         {
			 //args
			if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&args)) 
			{
				int interval = 0;
				int retry = 0;
				int brtime = 0;

				dbus_message_iter_recurse(&args, &structIter);
                dbus_message_iter_get_basic(&structIter, &interval);
                //printf( "interval %i\n",interval);
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &retry);
                //printf( "retry %i\n",retry);
                dbus_message_iter_next(&structIter);
                dbus_message_iter_get_basic(&structIter, &brtime);
                //printf( "brtime %i\n",brtime);
                if (PMIPv6GlobalSettingUpdated) 
                    PMIPv6GlobalSettingUpdated(interval,retry,brtime);
            }
			//printf( "Signature match !\n"); 
		 }
         else {
            dbus_message_iter_get_basic(&args, &sigvalue);
            //printf( "Got Signal with valuex %s\n", sigvalue);
         }
         if( sign1 )
			dbus_free(sign1);

  		  //printf( "PMIPv6GlobalSettingUpdated !\n");
	  }
	  else{
		//printf( "end filter\n");  
	  }
      // free the message
      dbus_message_unref(msg);
   }   
   //printf( "end of call\n");
   
   //end of call
   dbus_connection_close(conn);		
	
}
void glibInit() {
#if (GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 36)
    g_type_init();
#endif
    // redirect glib message
//    g_log_set_default_handler(gLogHandler, NULL);
    // coredump on glib error
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

    cassandra_if_get_type();
    column_get_type();
    super_column_get_type();
    counter_column_get_type();
    counter_super_column_get_type();
    column_or_super_column_get_type();
    not_found_exception_get_type();
    invalid_request_exception_get_type();
    unavailable_exception_get_type();
    timed_out_exception_get_type();
    authentication_exception_get_type();
    authorization_exception_get_type();
    schema_disagreement_exception_get_type();
    column_parent_get_type();
    column_path_get_type();
    slice_range_get_type();
    slice_predicate_get_type();
    index_expression_get_type();
    index_clause_get_type();
    key_range_get_type();
    key_slice_get_type();
    key_count_get_type();
    deletion_get_type();
    mutation_get_type();
    endpoint_details_get_type();
    token_range_get_type();
    authentication_request_get_type();
    column_def_get_type();
    cf_def_get_type();
    ks_def_get_type();
    cql_row_get_type();
    cql_metadata_get_type();
    cql_result_get_type();
}
void memcmake()
{
	//do memcached stuff
	memc = NULL;
	servers = NULL;
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	char *memcachedServer = NULL;
	unsigned int memcachedPort = 0;
	char *env;

	memcachedServer = "127.0.0.1";
	env = getenv("MEMCACHE_DB_PORT");
	memcachedPort = env ? atoi(env) : MEMCACHED_DEFAULT_PORT;

	memc = memcached_create(NULL);
	servers = NULL;
	servers = memcached_server_list_append(servers, memcachedServer,
	                                       memcachedPort, &memRet);
	memRet = memcached_server_push(memc, servers);
	memRet = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
	memc->flags.buffer_requests = 0;
	memc->flags.no_block = 1;
	memc->flags.support_cas = 0;
	memc->flags.tcp_nodelay = 1;
	memc->flags.tcp_keepalive = 1;
	memc->tcp_keepidle = 1;
	memc->poll_timeout = 60 * 60 * 1000;
	memc->retry_timeout = 2;	
}

void memc_cleanup(void)
{
	if(servers)
	memcached_server_free(servers);
	if(memc)
	memcached_free(memc);
}
int assignLicense(char *contrlid, int num)
{
	
	int all = getActiveLicense();
	int ueLicenseMode=0;
	int pmipUECount=0;
	int tunnel_pmipv6=0;
	getUELicense(&ueLicenseMode,&pmipUECount,&tunnel_pmipv6);
	
	int remain = pmipUECount - all;
	int allcate = remain - num;
	if(allcate <=0 )
	{
		if(remain <=0 ) 
			return 0;
		else
		{
			updateNodeLicense(contrlid, remain);
			return remain;
		}
	}
	else
	{
		updateNodeLicense(contrlid, num);
		return num;
	}
	return allcate;
};
int revokeLicense(char *contrlid, int num)
{
	if(num==0)
	{
		//printf("the id we test is %s \n",contrlid);
		resetNodeLicense(contrlid);
		//printf("end of resetNodeLicense(contrlid);\n");
	}
	else
		updateNodeLicense(contrlid, -num);
	return 0;
};
int updateNodeLicense(char* bladeId, int count)
{
	//printf("updateNodeLicensea %s %d x=====",bladeId,count);
//	memcmake();
	//printf("finish memmak");
//getbr0
	char buf[20];
    char buf1[100];
    bzero(buf1, sizeof(buf1));
	snprintf(buf1, sizeof(buf1), "%s", bladeId);
	buf1[36]='\0';
	//printf("updateNodeLicense:Update [%s], length = %lu\n",buf1,strlen(buf1));
//save count to buf1
//1.get
//2.sum and set
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	char *getValue = NULL;
	size_t getValueLength = 0;
	uint32_t getFlags = 0;

	const long unsigned int valueLen=20;	
    //printf("updateNodeLicense:try to get it\n");
	getValue = memcached_get(memc, buf1, strlen(buf1), &getValueLength, &getFlags, &memRet);
	if(getValue==NULL)
		getValue="0";
    //add count
    //printf("middle");
    int addresult = atoi(getValue)+count;
    //printf("updateNodeLicense:try to add it %i\n",addresult);
    //set it
    //printf("updateNodeLicense:try to set it\n");
    bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "%d", addresult);
    if ((memRet = memcached_set(memc, buf1, strlen(buf1), buf, valueLen, (time_t) 0,(uint32_t) 0)) != MEMCACHED_SUCCESS) 
    {
    }
    //printf("middle2");
    //get it and print
	getValue = memcached_get(memc, buf1, strlen(buf1), &getValueLength, &getFlags, &memRet);
	//printf("updateNodeLicense:Update [%s]result is %s, length = %lu\n",buf1,getValue,strlen(buf1));

	free(getValue);
//	memc_cleanup();
	return 0;
}
int resetNodeLicense(char* ctrlid)
{	
//	memcmake();
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	long unsigned int keyLen = 36; 
	//printf("Testing resetNodeLicense %s  length %lu \n",ctrlid,keyLen);
	char *getValue = NULL;
	size_t getValueLength = 0;
	uint32_t getFlags = 0;
	
	const long unsigned int valueLen=20;	
    //set it
    //printf("resetNodeLicense:try to set it\n");
    char buf[100];
    bzero(buf, sizeof(buf));
	snprintf(buf, sizeof(buf), "%d", 0);
    if ((memRet = memcached_set(memc, ctrlid, keyLen, buf, valueLen, (time_t) 0,(uint32_t) 0)) != MEMCACHED_SUCCESS) 
    {
    }
    //get it and print
	getValue = memcached_get(memc, ctrlid, keyLen, &getValueLength, &getFlags, &memRet);
	//printf("resetNodeLicense:Update result is %s\n",getValue);

	if (memRet != MEMCACHED_SUCCESS) {
		return -1;
	}

	if (!getValue) {
		return -1;
	}

	free(getValue);
//	memc_cleanup();
	//printf("end of clean up reset procedure\n");
	return 0;
}
int setMemKeyVal(char* key, char* val)
{	
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	long unsigned int keyLen = strlen(key); /* 6 bytes MAC address in binary */
    if ((memRet = memcached_set(memc, key, keyLen, val, strlen(val), (time_t) 0,(uint32_t) 0)) != MEMCACHED_SUCCESS) 
    {
    }

	if (memRet != MEMCACHED_SUCCESS) {
		return -1;
	}
	return 0;
}

int getActiveLicense()
{
//	memcmake();

 /*  
  * GetActiveLicense 
  * 1.load List
  * 2.find blade1 blade2 blade3 blade4
  * 3.sum them and return
  * */
	memcached_return_t memRet = MEMCACHED_SUCCESS;
	char* key = "PMIPList";
	long unsigned int keyLen = strlen(key); /* 6 bytes MAC address in binary */
	char *getValue = NULL;
	size_t getValueLength = 0;
	uint32_t getFlags = 0;
	
	getValue = memcached_get(memc, key, keyLen, &getValueLength, &getFlags, &memRet);
	if(getValue==NULL)
		getValue="0";
	
	//printf("***********GetActiveLicense*************");
	//printf("PMIPList=%s\n",getValue);
	const char s[2] = ",";
    char *token;
   
	/* get the first token */
	token = strtok(getValue, s);
   
	/* walk through other tokens */
	int counters = 0;
	
	while( token != NULL ) 
	{
		//printf( "The id is %s\n", token );    
		getValue = memcached_get(memc, token, strlen(token), &getValueLength, &getFlags, &memRet);
		//printf( "The License result key %s length  %lu \n value %s", token ,strlen(token),getValue);    

		if(getValue==NULL)
			getValue="0";
		counters+=atoi(getValue);
		token = strtok(NULL, s);
	}
	
	//printf("the finle result = %i\n",counters);
	
	if (memRet != MEMCACHED_SUCCESS) {
		return -1;
	}

	if (!getValue) {
		return -2;
	}
//	memc_cleanup();
	free(getValue);
	return counters;

}


