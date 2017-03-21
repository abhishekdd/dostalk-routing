#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "link_api.h"
#include "net_api.h"

#define MSGLENMAX 70
#define INPUTLIMIT (MSGLENMAX + 18)
#define PKT_INT 0x60

void prepareHeader(unsigned char *pktBuffer, unsigned char *destMAC, 
                    unsigned char *srcMAC, unsigned char *type, 
                    unsigned char *destIIP, unsigned char *srcIIP);

void readDestinationIIP(unsigned char *iip);
void changeHeaderDestMAC(unsigned char *pktBuffer, unsigned char *destMAC);
void changeHeaderDestIIP(unsigned char *pktBuffer, unsigned char *destIIP);
unsigned char *setDestination(ARPTable table);

unsigned char pktType[2] = {0x00, 0x00};
unsigned char myIIP[2], routerIIP[2];
unsigned char pktBuffer[INPUTLIMIT + 1], *pktReceived, myMAC[6];
int inputIdx;

void interrupt handleResponse(US bp, US di, US si, US ds, US es,
                US dx, US cx, US bx, US ax, US iip, US cs, US flags)
{
    if (ax == 0) {
        pktReceived = malloc(sizeof(unsigned char) * cx);
        es = FP_SEG(pktReceived);
        di = FP_OFF(pktReceived);
    }

    if (ax == 1) {
        if (memcmp(pktReceived, myMAC, 6) == 0) {
            int msgLen, spaceToPrint, i;
/*            cprintf("\r>>> Received: ");
            for (i = 0; i < 18; i++)
                cprintf("%02X:", pktReceived[i]);

            cprintf("\r\n>%s<", &pktReceived[18]);
*/
            cprintf("\r%02X:%02X| >>>>  ", pktReceived[16], pktReceived[17]);
            msgLen = cprintf("%s", &pktReceived[18]);
            
            spaceToPrint = MSGLENMAX - msgLen;
            i = 0;
            while (i++ <= spaceToPrint) 
                putch(' ');

            // After printing received message restore input progress
            cprintf("\r%02X:%02X| <<<<  ", pktBuffer[14], pktBuffer[15]);
            i = 18;
            while (i < inputIdx) 
                putch(pktBuffer[i++]);
        }

        free(pktReceived);
    }
}

int main(int argc, char *argv[])
{
    unsigned char ch, *tempstr;
    int handle, exit_flag, restart_flag, i;
    struct nic_info ni;
    ARPTable iiptable;

    if (argc < 3) {
        fprintf(stderr, "\nUsage: %s <your_IIP> <router_IIP>\n", argv[0]);
        fprintf(stderr, "\nExample: %s 1:2d 1:1\n", argv[0]);
        exit(1);
    }

    tempstr = strtok(argv[1], ":");
    myIIP[0] = strtol(tempstr, NULL, 16);
    tempstr = strtok(NULL, ":");
    myIIP[1] = strtol(tempstr, NULL, 16);
    printf("\nYour IIP: %02X:%02X\n", myIIP[0], myIIP[1]);
    
    tempstr = strtok(argv[2], ":");
    routerIIP[0] = strtol(tempstr, NULL, 16);
    tempstr = strtok(NULL, ":");
    routerIIP[1] = strtol(tempstr, NULL, 16);
    printf("\nRouter IIP: %02X:%02X\n", routerIIP[0], routerIIP[1]);

    if (myIIP[0] != routerIIP[0]) {
        fprintf(stderr, "\nRouter is unreachable.\n");
        exit(1);
    }

    initTable(&iiptable, 5);
    readTable(&iiptable, "iiplist");
/*    addEntry(&iiptable, "\x00\x00", "\x00\x00\x00\x00\x00\x00");*/
/*    printTable(iiptable);*/

    printf("------------------------- DOS talk --------------------------\n");
    driver_info(PKT_INT, &ni);
    printf("\nNIC | Name: \"%s\"; Version: %d; Functionality: %d.",
        ni.nic_name, ni.nic_version, ni.nic_functionality);
    printf("\nNIC | Class: %d; Type: %d; Number: %d.", 
        ni.nic_class, ni.nic_type, ni.nic_number);

    get_my_address(PKT_INT, myMAC);
    printf("\nYour MAC address: ");
    for (i = 0; i < 6; i++)
        printf("%02X:", myMAC[i]);

    prepareHeader(pktBuffer, "\x00\x00\x00\x00\x00\x00", myMAC, pktType, 
                             "\x00\x00", myIIP);
    setDestination(iiptable);

    handle = access_type(PKT_INT, ni, handleResponse, 0, pktType);
/*    get_rcv_mode(PKT_INT, handle);*/
    printf("-------------------------------------------------------------\n");

    /* Provide interactive prompt for input */
    while (1) {
        cprintf("\r\n%02X:%02X| <<<<  ", pktBuffer[14], pktBuffer[15]);
        /* Read input as well as actions */
        exit_flag = restart_flag =  0;
        inputIdx = 18;
        while (1) {
            if (inputIdx == INPUTLIMIT)
                break;

            ch = getch();
            if (ch >= 32 && ch <= 126) {
                /* Printable character */
                putch(ch);
                pktBuffer[inputIdx++] = ch;
            } else if (ch == 27 || ch == 3) {
                /* Escape (27) or Ctrl-C (3) */
                exit_flag = 1;
                break;
            } else if (ch == '\r') {
                /* Return */
                break;
            } else if (ch == '\b' && inputIdx > 18) {
                /* Backspace pressed and no underflow in input buffer */
                inputIdx--;     /* Take logical cursor back */
                putch('\b');    /* Take display cursor back */
                putch(' ');     /* Overwrite character with space */
                putch('\b');	/* Take display cursor back */
            } else if (ch == 12) {
                /* Ctrl-L (12) - Clear screen */
                pktBuffer[inputIdx] = '\0';
                clrscr();
                cprintf("\r\n<<< ");
                cprintf("%s", &pktBuffer[18]);
            } else if (ch == 4) {
                /* Ctrl-D (4) - Change destination IIP */
                setDestination(iiptable);
                restart_flag = 1;
                break;
            }
        }

        if (exit_flag)
            break;

        if (restart_flag)
            continue;

        pktBuffer[inputIdx] = '\0';    
        send_pkt(PKT_INT, pktBuffer, inputIdx + 1);
    }
    
    release_type(PKT_INT, handle);
    return 0;
}

void prepareHeader(unsigned char *pktBuffer, unsigned char *destMAC, 
                    unsigned char *srcMAC, unsigned char *type, 
                    unsigned char *destIIP, unsigned char *srcIIP)
{
    memcpy(pktBuffer, destMAC, 6);
    memcpy(&pktBuffer[6], srcMAC, 6);
    memcpy(&pktBuffer[12], type, 2);
    memcpy(&pktBuffer[14], destIIP, 2);
    memcpy(&pktBuffer[16], srcIIP, 2);
}

void readDestinationIIP(unsigned char *iip)
{
    cprintf("\r\nDestination IIP (format - B1:7E): ");
    cscanf(" %2x:%2x", iip, iip+1);
    getch();
    cprintf("\r\nType 'Ctrl+D' to change destination IIP address.");
}

void changeHeaderDestMAC(unsigned char *pktBuffer, unsigned char *destMAC)
{
    memcpy(pktBuffer, destMAC, 6);
}

void changeHeaderDestIIP(unsigned char *pktBuffer, unsigned char *destIIP)
{
    memcpy(&pktBuffer[14], destIIP, 2);
}

unsigned char *setDestination(ARPTable table)
{
    unsigned char *chptr, destIIP[2];

    while (1) {
        readDestinationIIP(destIIP);
        cprintf("\r\nInput: %02X:%02X ", destIIP[0], destIIP[1]);
        changeHeaderDestIIP(pktBuffer, destIIP);

        if (destIIP[0] == myIIP[0]) {
            if (destIIP[1] == myIIP[1]) {
                cprintf("\r\nSource and Destination IIPs cannot be same.");
                continue;
            }
            
            chptr = lookupEntry(table, destIIP);
            if (chptr != NULL) {
                changeHeaderDestMAC(pktBuffer, chptr);
                return destIIP;
            } else {
                cprintf("\r\nDestination host IIP is not in ARP table.");
                continue;
            }   
        } else {
            chptr = lookupEntry(table, routerIIP);

            if (chptr != NULL) {
                changeHeaderDestMAC(pktBuffer, chptr);
                return destIIP;
            } else {
                cprintf("\r\nRouter IIP is not in ARP table.");
                continue;
            }
        }
    }
}
