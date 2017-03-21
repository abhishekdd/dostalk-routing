#include <stdio.h>
#include <dos.h>
#include "link_api.h"

int error_check(union REGS outRegs, char *msg);

void driver_info(int swint, struct nic_info *ni)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 1;
    inRegs.h.al = 255;
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Driver Info") == 0) {
    	ni->nic_name = MK_FP(segRegs.ds, outRegs.x.si);
        ni->nic_version = outRegs.x.bx;
        ni->nic_functionality = outRegs.h.al;
        ni->nic_class = outRegs.h.ch;
        ni->nic_type = outRegs.x.dx;
        ni->nic_number = outRegs.h.cl;
    }
}
    

void get_my_address(int swint, char *address)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 6;
    segRegs.es = FP_SEG(address);
    inRegs.x.di = FP_OFF(address);
    inRegs.x.cx = 6;
    int86x(swint, &inRegs, &outRegs , &segRegs);

    if (error_check(outRegs, "Get Address") == 0) {
/*        printf("Your Ethernet Address: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                address[0], address[1], address[2], 
                address[3], address[4], address[5]);
*/
    }
}

int access_type(int swint, struct nic_info ni, receiver receiverFunc, 
                int typelen, unsigned char *type)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 2;
    inRegs.h.al = ni.nic_class;
    inRegs.x.bx = ni.nic_type;
    inRegs.h.dl = ni.nic_number;

    inRegs.x.cx = typelen;
    segRegs.ds = FP_SEG(type);
    inRegs.x.si = FP_OFF(type);

    segRegs.es = FP_SEG(receiverFunc);
    inRegs.x.di = FP_OFF(receiverFunc);
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Access Type") == 0) {
/*        printf("Handle: %d\n", outRegs.x.ax);*/
    }
    
    return outRegs.x.ax;
}

void release_type(int swint, int handle)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 3;
    inRegs.x.bx = handle;
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Release Type") == 0) {
/*        printf("\nHandle released.\n");*/
    }
}

void send_pkt(int swint, unsigned char *packet, int size)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;
    int i;

    inRegs.h.ah = 4;
    segRegs.ds = FP_SEG(packet);
    inRegs.x.si = FP_OFF(packet);
    inRegs.x.cx = size;
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Send Packet") == 0) {
/*        printf("Packet sent.\n");*/
    }
}

int get_rcv_mode(int swint, int handle)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 21;
    inRegs.x.bx = handle;
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Get Recieve Mode") == 0) {
/*        printf("Recieve Mode: %d\n", outRegs.x.ax);*/
    }

    return outRegs.x.ax;
}

void set_rcv_mode(int swint, int handle, int mode)
{
    union REGS inRegs, outRegs;
    struct SREGS segRegs;

    inRegs.h.ah = 20;               
    inRegs.x.bx = handle;
    inRegs.x.cx = mode;
    int86x(swint, &inRegs, &outRegs, &segRegs);

    if (error_check(outRegs, "Set Recieve Mode") == 0) {
/*        printf("Recieve Mode set to %d.\n", mode);*/
    }
}

int error_check(union REGS outRegs, char *msg)
{
    if (outRegs.x.cflag) {
        fprintf(stderr, "Error: %s; ", msg);
        fprintf(stderr, "Error code: %d.\n", outRegs.h.dh);
        exit(1);
    } 
    
    return 0;
}
