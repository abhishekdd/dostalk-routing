#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "net_api.h"
#include "link_api.h"

#define NIC1_INT 0x61
#define NIC2_INT 0x62

unsigned char pktType[2] = {0x00, 0x00};
unsigned char *pktReceived, nic1MAC[6], nic2MAC[6];
unsigned char nic1IIP[2] = {0x01, 0x01};
unsigned char nic2IIP[2] = {0x02, 0x01};
ARPTable iiptable1, iiptable2;

void interrupt handleResponse(US bp, US di, US si, US ds, US es,
                US dx, US cx, US bx, US ax, US ip, US cs, US flags)
{
    if (ax == 0) {
        pktReceived = malloc(sizeof(unsigned char) * cx);
        es = FP_SEG(pktReceived);
        di = FP_OFF(pktReceived);
    }

    if (ax == 1) {
        int i;
    	unsigned char *chptr;

        if (memcmp(pktReceived, nic1MAC, 6) != 0 && 
        	memcmp(pktReceived, nic2MAC, 6) != 0) {
            free(pktReceived);
        	return;
        }
        
        cprintf("\r\nReceived: ");
    	for (i = 0; i < 18; i++)
    		cprintf("%02X:", pktReceived[i]);
/*        cprintf("\r\n>%s<", &pktReceived[18]);*/

        if (pktReceived[14] == pktReceived  [16]) {
        	cprintf("\r\nMalformed packet; src and dest in same network.");
        	cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");
            free(pktReceived);
        	return;
        }

        if (pktReceived[14] == nic1IIP[0]) {
        	if (pktReceived[15] == nic1IIP[1]) {
	        	cprintf("\r\nMalformed packet. Destination is router NIC 1.");
	        	cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");
                free(pktReceived);
	        	return;
        	}

        	chptr = lookupEntry(iiptable1, &pktReceived[14]);
        	if (chptr != NULL) {
                memcpy(&pktReceived[0], chptr, 6);
                memcpy(&pktReceived[6], nic1MAC, 6);
                send_pkt(NIC1_INT, pktReceived, cx);
                cprintf("\r\nPacket forwarded.\r\nStop? (y/N) ");
            } else {
                cprintf("\r\nDestination IIP not in ARPtable 1.");               
                cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");
            }
        } else if (pktReceived[14] == nic2IIP[0]) {
            if (pktReceived[15] == nic2IIP[1]) {
                cprintf("\r\nMalformed packet. Destination is router NIC 2.");
                cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");
                free(pktReceived);
                return;
            }
            
            chptr = lookupEntry(iiptable2, &pktReceived[14]);
            if (chptr != NULL) {
                memcpy(&pktReceived[0], chptr, 6);
				memcpy(&pktReceived[6], nic2MAC, 6);
				send_pkt(NIC2_INT, pktReceived, cx);
                cprintf("\r\nPacket forwarded.\r\nStop? (y/N) ");
			} else {
        		cprintf("\r\nDestination IIP not in ARPtable 2.");
	        	cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");
        	}
        } else {
        	cprintf("\r\nUnreachable network.");
            cprintf("\r\nPacket dropped.\r\nStop? (y/N) ");            	
        }

        free(pktReceived);
    }
}

int main()
{
    int handle1, handle2, i;
	struct nic_info ni1, ni2;
	char ch;

    printf("\n------------------------- Router --------------------------\n");
    initTable(&iiptable1, 5);
    initTable(&iiptable2, 5);
    /*addEntry(&iptable, "\x00\x00", "\x00\x00\x00\x00\x00\x00");*/
    readTable(&iiptable1, "iiplist1");
    readTable(&iiptable2, "iiplist2");
/*    printf("\nARPTable of NIC 1: \n");
    printTable(iptable1);
    printf("\nARPTable of NIC 2: \n");
    printTable(iptable2);
*/

    driver_info(NIC1_INT, &ni1);
    driver_info(NIC2_INT, &ni2);
    printf("\nNIC1 | Name: \"%s\"; Version: %d; Functionality: %d.",
        ni1.nic_name, ni1.nic_version, ni1.nic_functionality);
    printf("\nNIC1 | Class: %d; Type: %d; Number: %d.", 
        ni1.nic_class, ni1.nic_type, ni1.nic_number);

    printf("\nNIC2 | Name: \"%s\"; Version: %d; Functionality: %d.",
        ni2.nic_name, ni2.nic_version, ni2.nic_functionality);
    printf("\nNIC2 | Class: %d; Type: %d; Number: %d.", 
        ni2.nic_class, ni2.nic_type, ni2.nic_number);

    get_my_address(NIC1_INT, nic1MAC);
    printf("\nNIC 1 MAC address: ");
    for (i = 0; i < 6; i++)
        printf("%02X:", nic1MAC[i]);

    get_my_address(NIC2_INT, nic2MAC);
    printf("\nNIC 2 MAC address: ");
    for (i = 0; i < 6; i++)
        printf("%02X:", nic2MAC[i]);
    
    handle1 = access_type(NIC1_INT, ni1, handleResponse, 0, pktType);
    handle2 = access_type(NIC2_INT, ni2, handleResponse, 0, pktType);
/*    get_rcv_mode(NIC1_INT, handle1);
    get_rcv_mode(NIC2_INT, handle2);
*/
    printf("\n-----------------------------------------------------------\n");

    while (1) {
        cprintf("\r\nStop? (y/N) ");
        ch = getche();

        if (ch == 'y' || ch == 'Y')
            break;
    }
    
    release_type(NIC1_INT, handle1);
    release_type(NIC2_INT, handle2);

    freeTable(&iiptable1);
    freeTable(&iiptable2);
    
    return 0;
}