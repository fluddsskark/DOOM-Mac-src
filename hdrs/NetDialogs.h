#ifndef __NETDIALOGS__#define __NETDIALOGS__#define __MAC_VERSION__#include "doomdef.h"#include "MacPCSwitches.h"void GetPlayMode (void);boolean ReadPacket (void);void WritePacket (char *buffer, int len);#define MAXPACKET 512extern char packet[MAXPACKET];extern int  packetlen;#endif __NETDIALOGS__