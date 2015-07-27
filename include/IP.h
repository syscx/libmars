#define MACADDRFMT	"%02X:%02X:%02X:%02X:%02X:%02X"
#define MACADDRCOLONFMT "%02X:%02X:%02X:%02X:%02X:%02X"
#define MACADDRDASHFMT	"%02X-%02X-%02X-%02X-%02X-%02X"
#define MACADDRHEXFMT	"%02X%02X%02X%02X%02X%02X"
#define MACADDRDOTFMT	"%02X%02X.%02X%02X.%02X%02X"   /* Cisco dotted MAC address format */
#define MACADDR(a) \
	((uint8_t *)a)[0], \
	((uint8_t *)a)[1], \
	((uint8_t *)a)[2], \
	((uint8_t *)a)[3], \
	((uint8_t *)a)[4], \
	((uint8_t *)a)[5]

#define IPADDRFMT	"%u.%u.%u.%u"
#define IPADDR(a) \
	((uint8_t *)a)[0], \
	((uint8_t *)a)[1], \
	((uint8_t *)a)[2], \
	((uint8_t *)a)[3]
