#include "PMIP.h"
#include <pthread.h>

void demo_PMIPv6GlobalSettingUpdated(int interval, int retry, int BindingRefreshTime)
{
	printf("function 5\n");	
}
void demo_PMIPv6ProfileUpdated(char* profileid, char* primaryip, char* secondaryip, char* mn, char* mac, char* apn, char* mvnoid)
{
	printf("function 3\n");	
}
void demo_PMIPv6ProfileRemoved(char* profile)
{
	printf("function 4\n");
}
void demo_PMIPv6WlanUpdated(char* Zone, char* wlanid, char* pmipprofile, char* mvnoid, char *opRealm, char *apssid, int cnrIntf) {
	printf("function 1\n");
}
void demo_PMIPv6WlanRemoved(char* Zone, char* wlanid)
{
	printf("function 2\n");
}
void demo_PMIPv6LicenseChange(int license)
{
	printf("function %d\n",license);
}
void demo_PMIPv6logChange(int log)
{
	printf("function %d\n",log);
}

void demo_PMIPupdmessage(unsigned char* ueMACValue, uint32_t ueIPValue, uint8_t causeValue)
{
	printf("demo_PMIPupdmessage uemac %s\n",ueMACValue);
}

int main(int argc, char* argv[]) {
     pthread_t thread1, thread2;
     char *message1 = "UDP Service";
     char *message2 = "DBUS Listen Service";
     int  iret1, iret2;

     printf("set up PMIPv6 callbacks\n");
     pmipCallbackSet(demo_PMIPv6GlobalSettingUpdated, 
                     demo_PMIPv6ProfileUpdated,
                     demo_PMIPv6ProfileRemoved,
                     demo_PMIPv6WlanUpdated,
					 demo_PMIPv6LicenseChange,
					 demo_PMIPv6logChange,
                     demo_PMIPv6WlanRemoved);
 
	udpCallbackSet(demo_PMIPupdmessage);
 
 
     printf("build UDP Listener\n");
     iret1 = pthread_create( &thread1, NULL, udplistener, (void*) message1);
     printf("build DBUS Listener\n");
     iret2 = pthread_create( &thread2, NULL, PMIPListenDbus, (void*) message2);
     printf("finish make thread \n");

     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL); 

     return 1;
}
