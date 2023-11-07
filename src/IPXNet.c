/* =============================================================================  						IPX PACKET DRIVER  ============================================================================= */  #define __MAC_VERSION__#include <Lion.h>#include "LionDoom.h"#include "IPXNet.h" #include "DebugSwitches.h" #include "ipxcalls.h"#include "ipxerror.h"#include "sap.h"#include "rip.h"#include "bindery.h"#include <OSUtils.h>long			remotetime;long			gLocalTime;//#include "IPXNet.PROTO.H" #define     	kSocketId  0x869b    	// 0x869c is the official DOOM socketpacket_t        *gPackets;       // [NUMPACKETS] nodeadr_t		remoteadr;			// set by each GetPacketint         	remotenetid;  unsigned short	gIPXSocket;QHdr			gReceiveQueue;unsigned char	gLocalIPXAdr[10];nodeadr_t     	nodeadr[MAXNETNODES+1];	// first is local, last is broadcastipxnerd_t		gIPXNerdAddr;			// Information for talking across routersBoolean			gIPXNerd = false;	// Completion Routinespascal void	send_ESR(IPX_ECB *ecb);ESRUPP				send_ESR_UPP=NULL;extern unsigned short OpenSocket(unsigned short socketNumber);extern void ListenForPacket(IPX_ECB *ecb);extern void GetLocalAddress (void); //===========================================================================    //____________________________________________________________________________//// OpenSocket// // Get a dynamic socket.//____________________________________________________________________________unsigned short OpenSocket(unsigned short socketNumber) { 	OSErr	status;		status = IpxOpenSocket(&socketNumber, WANT_BROADCASTS);	if (status)	{		I_Error("IpxOpenSocket failed, Error 0x%hx", status);	}	return (socketNumber);}   //_________________________________________________________________//// ListenForPacket//// Set up packet to recieve.//_________________________________________________________________void ListenForPacket(IPX_ECB *ecb) { 	OSErr	status;		status = IpxReceive(gIPXSocket, ecb);	if (status)	{		I_Error("IpxReceive failed, Error 0x%hx", status);	}}  //______________________________________________________________________//// GetLocalAddress////______________________________________________________________________void GetLocalAddress (void) { 	OSErr	status;	/*	 * Get my IPX network address.	 */	status = IpxGetInternetworkAddress(gLocalIPXAdr);	if (status)	{		I_Error("IpxGetInternetworkAddress failed!  Err=0x%hx",status);	} }  /* ==================== = = IPXTerminateNet = ==================== */  void IPXTerminateNet (void) { 	OSErr	status; 	// Close the socket	status = IpxCloseSocket(gIPXSocket); 	// Free all memory (ECB) 	 	 	if (gPackets) 	{ 		DisposePtr((Ptr)gPackets); 	} 	}   /* ==================== = = IPXInitializeNet = ==================== */  void IPXInitializeNet (void) { 	unsigned short tempSocket;	int     		i,j,k; 	OSErr			status;	// ???????????? ��JDM WARNING Check for Gestalt (but should be done before this)  	// Allocate memory for packets 	gPackets = (packet_t *)NewPtrClear (NUMPACKETS*sizeof(packet_t)); 	if (gPackets == NULL)	{		I_Error("Not enough available memory to continue");	}	// Initialize receive queue	gReceiveQueue.qHead = (QElemPtr)0;	gReceiveQueue.qTail = (QElemPtr)0;		gReceiveQueue.qFlags = 0;		// Initialize completion routines	send_ESR_UPP = NewESRProc(send_ESR);	if (send_ESR_UPP == NULL)	{		I_Error("Unable to allocate a UPP for send_ESR");		return;	}	// Initialize IPX	status = IpxInitialize();	if (status)	{		I_Error("IpxInitialize failed!  Err=0x%hx", status);		return;	}	// Allocate a socket for sending and receiving 		// If zero, use default, otherwise use requested value (fudged appropriately)	if (gIPXSocket == 0)	{		tempSocket = kSocketId;	}	else if (gIPXSocket <= 0x7fff)	{		tempSocket = gIPXSocket;	}	else  if (gIPXSocket < 64000)	{		tempSocket = gIPXSocket - 1;	}	else  if (gIPXSocket == 64000)	{		tempSocket = 64255;	}			gIPXSocket = OpenSocket(tempSocket); //      printf ("socketnum: 0x%x\n",socketnum);  	GetLocalAddress();   	// Initialize the receiving ECBs  	for (i = kNumSends ; i<NUMPACKETS ; i++) 	{ 		gPackets[i].ecb.ESRAddress = NULL;		gPackets[i].ecb.MacOSQueue = &gReceiveQueue; 				// Receive queue		gPackets[i].ecb.fragCount = 1; 								// Number of fragments		gPackets[i].ecb.fragList[0].fragAddress = &gPackets[i].ipx;	// Ptr to header		gPackets[i].ecb.fragList[0].fragSize = sizeof(IPX_HEADER) + sizeof(netstruc_t);		ListenForPacket (&gPackets[i].ecb); 	}  	for (i = 0 ; i < kNumSends ; i++) 	{ 		// Initialize a sending ECB 		gPackets[i].ecb.flags = NO_CHECKSUM_FLAG; 					// Dont create checksum		gPackets[i].ecb.ESRAddress = send_ESR_UPP;		gPackets[i].ecb.MacOSQueue = NULL;		gPackets[i].ecb.UserWorkspace[0] = 0;		gPackets[i].ecb.fragCount = 1; 		gPackets[i].ecb.fragList[0].fragAddress = &gPackets[i].ipx; 		gPackets[i].ecb.fragList[0].fragSize = sizeof(IPX_HEADER) + sizeof(netstruc_t); 			// Initialize sending header		gPackets[i].ipx.packetType = 4;				// Destination network will not change, so we can		// store it in the packet		gPackets[i].ipx.destNet = 0;		if (gIPXNerd)		{			gPackets[i].ipx.destNet = gIPXNerdAddr.net;		}		else		{			for (j=0 ; j<4 ; j++)			{				gPackets[i].ipx.destNet = (gPackets[i].ipx.destNet << 1) | gLocalIPXAdr[j];			}		}		gPackets[i].ipx.destSocket = gIPXSocket;			}		// Copy local node to index 0 in address array	for (j=4, k = 0; j<10 ; j++, k++)	{		nodeadr[0].node[k] = gLocalIPXAdr[j];	}		// Broadcast node at MAXNETNODES	for (j=0 ; j<6 ; j++)	{		nodeadr[MAXNETNODES].node[j] = 0xff;	}}   /* ============== = = SendPacket = ============== */  void SendPacket (int destination) { 	int             j,i;	Boolean			found = false; 	OSErr			status; 	 	gLocalTime++; 		while(!found)	{		for (i = 0; i<kNumSends	&& !found; i++)		{			if (gPackets[i].ecb.UserWorkspace[0] == 0)				found = true;		} 	} 	 	i -= 1;#if __IPX_DEBUG__	fprintf(debugfile,"\t\tGot a free packet");#endif	// set the time	gPackets[i].msg.time = LONG(gLocalTime); 	// set the address	for (j=0 ; j<6 ; j++)	{	gPackets[i].ipx.destNode[j] = gPackets[i].ecb.immediateAddress[j] =		nodeadr[destination].node[j];	}	// Set the size (time (4) + size of data,  + 4 (to be compatible w/ PC)	gPackets[i].ecb.fragList[0].fragSize = 38 + doomcom->datalength;		//�� JDM Accelerate	// put the data into an ipx packet 	BlockMove (&doomcom->data, &gPackets[i].msg.data, doomcom->datalength);   	gPackets[i].ecb.UserWorkspace[0] = 1; 	#if __IPX_DEBUG__	fprintf(debugfile,"\t\tSending packet");#endif	status = IpxSend(gIPXSocket, &gPackets[i].ecb);//printf ("done\n"); 	if (status) 	{		I_Error ("SendPacket %d : 0x%x", i, status);	}#if __IPX_DEBUG__	fprintf(debugfile,"\t\tPacket sent");#endif}   /* ============== = = GetPacket = = Returns false if no packet is waiting = ============== */  Boolean GetPacket (long *size) { 	int           		i;  	long				packettic; 	packet_t			*packet; 	OSErr				status; 		doomcom->remotenode = -1;	 	#if __IPX_DEBUG__	fprintf(debugfile,"\t\tChecking for packet");#endif	if (gReceiveQueue.qHead)	{		packet = (packet_t *)gReceiveQueue.qHead;		status = Dequeue((QElemPtr)gReceiveQueue.qHead, &gReceiveQueue);		if (status != noErr) 		{			I_Error("Packet Dequeue error");		}	}	else	{#if __IPX_DEBUG__		fprintf(debugfile,"\t\tNo packet available");#endif		return(false);	}   	// Get time and flip bytes	packettic = LONG(packet->msg.time);	#if __IPX_DEBUG__	fprintf(debugfile,"\t\tGot packet of time %d ",packettic);#endif	// Check if this is a setup broadcast for another game	if (packettic == -1 && gLocalTime != -1)	{#if __IPX_DEBUG__	fprintf(debugfile,"\t\tGot broadcast for another game");#endif#if __SYSBEEPS__		SysBeep(3);#endif		return (false);	}	 	if (packet->ecb.status != IPX_SUCCESSFUL) 	{		I_Error ("GetPacket: ecb.status = 0x%x",packet->ecb.status); 	 } 		//	// ------- GOT A GOOD PACKET -------	//		// Find out who it is from	memcpy (&remoteadr, packet->ipx.sourceNode, sizeof(remoteadr));	for (i=0 ; i<doomcom->numnodes ; i++)	{		if (!memcmp(&remoteadr, &nodeadr[i], sizeof(remoteadr)))			break;	}		remotetime = packettic;	#if __IPX_DEBUG__	fprintf(debugfile,"\t\tGot broadcast for another game");#endif	if (i < doomcom->numnodes)	{		doomcom->remotenode = i;	}	else	{		if (gLocalTime != -1)		{	// this really shouldn't happen#if __IPX_DEBUG__			fprintf(debugfile,"\t\tError Could not match address");#endif			ListenForPacket ((IPX_ECB *) packet);			return (false);		}	}	// Get Size of data	doomcom->datalength = packet->ecb.dataLen - 38;	*size = doomcom->datalength;	if (doomcom->datalength > sizeof(doomdata_t))	{#if __IPX_DEBUG__		fprintf(debugfile,"\t\tError Got a bad sized packet");#endif		ListenForPacket ((IPX_ECB *) packet);		return (false);	}		// Copy out the data	BlockMove ( &packet->msg.data, &doomcom->data, doomcom->datalength);  	// repost the ECB  	ListenForPacket ((IPX_ECB *) packet); 	return (true); }// Completion routine for sendpascal void send_ESR(IPX_ECB *ecb){	ecb->UserWorkspace[0] = 0;}