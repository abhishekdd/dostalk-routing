typedef struct {
	int maxsize;
	int entries;
	int index;
	unsigned char **IIPs;
	unsigned char **MACs;
} ARPTable;

void initTable(ARPTable *table, int size);
void freeTable(ARPTable *table);
void readTable(ARPTable *table, char *arpfilename);
void printTable(ARPTable table);
unsigned char *lookupEntry(ARPTable table, unsigned char *iip);
void addEntry(ARPTable *table, unsigned char *IIP, unsigned char *MAC);
