#define US unsigned short
typedef void interrupt (*receiver)(US bp, US di, US si, US ds, US es,
            US dx, US cx, US bx, US ax, US ip, US cs, US flags);

void driver_info(int swint, struct nic_info *ni);
void get_my_address(int swint, char *address);
int access_type(int swint, struct nic_info ni, receiver r, int typelen, unsigned char *type);
void release_type(int swint, int handle);
void send_pkt(int swint, unsigned char *packet, int size);
int get_rcv_mode(int swint, int handle);
void set_rcv_mode(int swint, int handle, int mode);

struct nic_info {
	char *nic_name;
	int nic_version;
	int nic_functionality;
	int nic_class;
	int nic_type;
	int nic_number;
};
