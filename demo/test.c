#include "PMIP.h"
int main(int argc, char* argv[]) {
//memcmake();
/*
int g = getPMIPv6LogLevel();
printf("the log level is %d\n",g);
char* ctrlid = getControlBladeId();
printf("the id we obtain is %s\n",ctrlid);
char* ctrlid2 = getControlBladeId();
printf("the id we obtain is %s\n",ctrlid2);
printf("try to revok %s  x\n",ctrlid);
revokeLicense(ctrlid, 0);
printf("try to assign %s  x\n",ctrlid);
printf("ready?");
assignLicense(ctrlid, 100);
assignLicense(ctrlid, 1);
assignLicense(ctrlid, 1);
assignLicense(ctrlid, 1);
assignLicense(ctrlid, 1);
assignLicense(ctrlid2, 1);
assignLicense(ctrlid, 1);
printf("try to revok\n");
revokeLicense(ctrlid, 59);
printf("try to getActiveLicense\n");
int alllic = getActiveLicense();
printf("The license is %d \n",alllic);
int alllic2 = getActiveLicense();
printf("The license is %d \n",alllic2);
*/
//memc_cleanup();
/*
	printf("Begin test updateNodeLicense to memcached \n");
memcmake();
*/
/*
	char* testbladeid = "mybladeid1";
	updateNodeLicense(testbladeid,111);
	char* testbladeid2 = "mybladeid2";
	updateNodeLicense(testbladeid2,112);
	char* testbladeid3 = "mybladeid3";
	updateNodeLicense(testbladeid3,113);
	char* testbladeid4 = "mybladeid4";
	updateNodeLicense(testbladeid4,114);
	
	int alllic = getActiveLicense();
	
	printf("the result %d \n",alllic);
	
	printf("End test updateNodeLicense to memcached \n");
	printf("Test getActiveLicense");
	int testget = getActiveLicense();
	printf("end of getActiveLicense %d\n",testget);
	printf("Test resetNodeLicense");
	testget = resetNodeLicense();
	printf("end of resetNodeLicense %d\n",testget);
*/
//memc_cleanup();
//getClusterIPList
	ClusterIPList* wpx ;
	int counterx = 0;
	getClusterIPList(&wpx,&counterx);
	ClusterIPList wpxa = *wpx ;	
	printf("Test get IP =  %s\n",  wpxa.clusterip);
//PMIP start accept service
//	int ueLicenseMode=0;
//	int pmipUECount=0; 
//	int tunnel_pmipv6=0;

	char* dBladeId="dbladeid";
	char* lmaIp="lmaIp";
	char* mvnoId="mvnoId";
	long pbuPkts=123;
	long pbaPkts=2;
	long pbuLifetime0Pkts=3;
	long pbaLifetime0Pkts=4;
	long briPkts=5;
	long braPkts=6;
	long totalPkts=7;
	long recordCreationTime=8;
	long recordUpdateTime=9;

	printf("-test writepmipv6LMASignalingStats #1\n");

	writepmipv6LMASignalingStats(dBladeId,lmaIp, mvnoId,pbuPkts,pbaPkts,pbuLifetime0Pkts,pbaLifetime0Pkts,briPkts,braPkts,totalPkts,recordCreationTime,recordUpdateTime);
	printf("-test writepmipv6LMASignalingStats #1 done\n");

	printf("MassTest writepmipv6LMASignalingStats \n");
	int g = 0;
	for(g =0 ; g < 2 ; g ++)
	{
        char clientMac[24];
        bzero(clientMac, sizeof(clientMac));
        snprintf(clientMac, sizeof(clientMac),"%d", g);
        printf("test run %d",g);
        dBladeId = clientMac;
		writepmipv6LMASignalingStats(dBladeId,lmaIp, mvnoId,pbuPkts,pbaPkts,pbuLifetime0Pkts,pbaLifetime0Pkts,briPkts,braPkts,totalPkts,recordCreationTime,recordUpdateTime);
	}
	printf("MassTest writepmipv6LMASignalingStats Done\n");

	char* LMASIGNAL_dBladeId="dbladeid";
	char* LMASIGNAL_lmaIp="lmaIp";
	char* LMASIGNAL_mvnoId="mvnoId";
	long LMASIGNAL_pbuPkts=123;
	long LMASIGNAL_pbaPkts=222;
	long LMASIGNAL_pbuLifetime0Pkts=333;
	long LMASIGNAL_pbaLifetime0Pkts=444;
	long LMASIGNAL_briPkts=5555;
	long LMASIGNAL_braPkts=6666;
	long LMASIGNAL_totalPkts=77777;
	long LMASIGNAL_recordCreationTime=1375860635;
	long LMASIGNAL_recordUpdateTime=1375860635;

	printf("-test writepmipv6LMASignalingStats #2\n");

	writepmipv6LMASignalingStats(LMASIGNAL_dBladeId,LMASIGNAL_lmaIp, 
	LMASIGNAL_mvnoId,LMASIGNAL_pbuPkts,LMASIGNAL_pbaPkts,LMASIGNAL_pbuLifetime0Pkts,LMASIGNAL_pbaLifetime0Pkts,
	LMASIGNAL_briPkts,LMASIGNAL_braPkts,LMASIGNAL_totalPkts,LMASIGNAL_recordCreationTime,LMASIGNAL_recordUpdateTime);
	printf("-test writepmipv6LMASignalingStats #2 done\n");
	
	char* dBladeId2="aa";
	char* mvnoId2="";
	char* primaryLMAIp="";
	char* secondaryLMAIp="";
	char* activeLMAIp="";
	long primaryLMADuration=0;
	long secondaryLMADuration=0;
	long numOfFailOverPrimaryToSecondary=0;
	long numOfFailOverSecondaryToPrimary=0;
	long lastFailoverTime=0;
	long recordCreationTime1=0;
	long recordUpdateTime1=0;

	writepmipv6LMAConnectivityStats(dBladeId2, mvnoId2 ,primaryLMAIp,secondaryLMAIp,activeLMAIp,primaryLMADuration,secondaryLMADuration,numOfFailOverPrimaryToSecondary,numOfFailOverSecondaryToPrimary,lastFailoverTime,recordCreationTime1,recordUpdateTime1);

	char* dpMac="ff:11:22:33:44:55";
	char* ueMac="00:11:22:33:44:55";
	char* ueIpAddress="12.2.2.2";
	char* apMac="22:33:44:55:66";
	char* magControlIp="2.3.4.5";
	char* magDataIp="2.3.4.5";
	char* lmaIp3="99.99.99.99";
	char* mvnoid = "testmvnoid";
	long sessionStartTime=12345678901;
	int cause=3;
/*
	WritetoPmipv6UeSessionInfo(dpMac, ueMac, ueIpAddress , apMac, magControlIp, magDataIp, lmaIp3,
	sessionStartTime, cause);
*/

	printf("MassTest WritetoPmipv6UeSessionInfo \n");
	for(g =0 ; g < 10 ; g ++)
	{
        char clientMac[24];
        bzero(clientMac, sizeof(clientMac));
        snprintf(clientMac, sizeof(clientMac),"%d", g);
        printf("test run %d",g);
        ueMac = clientMac;
		WritetoPmipv6UeSessionInfo(dpMac, ueMac, ueIpAddress , apMac, magControlIp, magDataIp, lmaIp3,
		sessionStartTime, cause,mvnoid);
	}
	printf("MassTest WritetoPmipv6UeSessionInfo Done\n");

/*
	getUELicense(&ueLicenseMode,&pmipUECount,&tunnel_pmipv6);
	printf("test.c get value ueLicenseMode %u pmipUECount %u tunnel_pmipv6 %u",ueLicenseMode,pmipUECount,tunnel_pmipv6);
	printf("=========testing getpmipv6===========\n");
	printf("testing1 begin \n");
	printf("do test dbconnect\n");
	g_type_init();
    GPtrArray *columns = g_ptr_array_new();
    genColumn(columns, "clientMac", "testx", strlen("testx"));
    genColumn(columns, "apMac", "testxx", strlen("testxx"));
    char* columnfamily ="pmipv6LMAConnectivityStats";
    char* key ="xa2d";
	dbconnect(columns,columnfamily, key);
	printf("end of testing db connect\n");
	printf("test getting getAllPMIPv6Profile\n");
	PMIPv6GlobalSetting data = getPMIPv6GlobalSetting();
	int counter  = 0;
	PMIPv6Profile* p1 ;	
	getAllPMIPv6Profile(&p1,&counter);
	printf("Address of p1: %p\n",  p1);

	PMIPv6Profile pd1 =*p1;
	printf("*******getAllPMIPv6Profile---Array1 id %s \n",pd1.id);
	printf("*******getAllPMIPv6Profile--Array1 ip1 %s \n",pd1.ip1);
	printf("getAllPMIPv6Profile--Array1 ip2 %s \n",pd1.ip2);
	printf("getAllPMIPv6Profile--Array1 mn %s \n",pd1.mn);
	printf("getAllPMIPv6Profile--Array1 mac %s \n",pd1.mac);
	printf("getAllPMIPv6Profile--Array1 apn %s \n",pd1.apn);
	printf("getAllPMIPv6Profile--Array1 mvnoid %s \n",pd1.mvnoid);

	PMIPv6Profile pd2 =*(p1+1);
	printf("getAllPMIPv6Profile---Array2 id %s \n",pd2.id);
	printf("getAllPMIPv6Profile--Array2 ip1 %s \n",pd2.ip1);
	printf("getAllPMIPv6Profile--Array2 ip2 %s \n",pd2.ip2);
	printf("getAllPMIPv6Profile--Array2 mn %s \n",pd2.mn);
	printf("getAllPMIPv6Profile--Array2 mac %s \n",pd2.mac);
	printf("getAllPMIPv6Profile--Array2 apn %s \n",pd2.apn);
	printf("getAllPMIPv6Profile--Array1 mvnoid %s \n",pd2.mvnoid);
*/
	PMIPv6ProfileAndWlanMapping* wp1 ;
	int counter2 = 0;
	
	getAllPMIPv6ProfileAndWlanMapping(&wp1,&counter2);
	printf("Address of p1: %p\n",  wp1);

/*

	PMIPv6ProfileAndWlanMapping wpd1 =*wp1;
	printf("getAllPMIPv6Profile---Array1 zone %s \n",wpd1.zone);
	printf("getAllPMIPv6Profile--Array1 wlan %s \n",wpd1.wlan);
	printf("getAllPMIPv6Profile--Array1 id %s \n",wpd1.id);
	printf("getAllPMIPv6Profile--Array1 mvnoid %s \n",wpd1.mvnoid);

	PMIPv6ProfileAndWlanMapping wpd2 =*(wp1+1);
	printf("getAllPMIPv6Profile---Array2 id %s \n",wpd2.zone);
	printf("getAllPMIPv6Profile--Array2 ip1 %s \n",wpd2.wlan);
	printf("getAllPMIPv6Profile--Array2 ip2 %s \n",wpd2.id);
	printf("getAllPMIPv6Profile--Array1 mvnoid %s \n",wpd2.mvnoid);

	printf("PMIP Profile Global Setting: interval %u\n",data.interval);
	printf("PMIP Profile Global Setting: retry %u\n",data.retry);
	printf("PMIP Profile Global Setting: BindingRefreshTime %u\n",data.BindingRefreshTime);
*/	
	printf("testing1 end \n");
	printf("testing2 begin \n");
	unsigned char* ueMACValue;
	unsigned char test[6];
	test[0]=0x30;
	test[1]=0x39;
	test[2]=0x26;
	test[3]=0x20;
	test[4]=0x85;
	test[5]=0x23;
	ueMACValue=test;

	unsigned char* apMACValue;
	unsigned char test2[6];
	test2[0]=0x50;
	test2[1]=0xa7;
	test2[2]=0x33;
	test2[3]=0x1b;
	test2[4]=0xec;
	test2[5]=0x40;
	apMACValue=test2;

	unsigned char buffer[] = {0x0a, 0x0b, 0x0a, 0x0a};
	uint32_t ueIPValue;
	ueIPValue = (uint32_t)buffer[0] << 24 |
	      (uint32_t)buffer[1] << 16 |
	      (uint32_t)buffer[2] << 8  |
	      (uint32_t)buffer[3];
	printf("PMIP to Greyhound\n");
	pmipSessionToTerminate(ueMACValue, ueIPValue, 68);

	//printf("memproxy to UDP PMIP\n");
	int ix = 0 ;
	for(ix = 0 ; ix <100 ; ix++)
	{
		printf("test code");
		pmipSessionToTerminateAP(ueMACValue, ueIPValue, 43,apMACValue);
	}

	printf("testing end \n");	
    for (;;)
        ;

	return 1;

}
//implement 
//PMIPv6WlanRemoved PMIPv6ProfileUpdated PMIPv6WlanUpdated PMIPv6ProfileRemoved PMIPv6GlobalSettingUpdated 
//to receive signal from DBUS
