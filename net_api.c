#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "net_api.h"

void initTable(ARPTable *table, int size)
{
	int i;

	if (size < 1) {
		fprintf(stderr, "\nARPTable size has to be at least 1.");
		exit(1);
	}

	table->maxsize = size;
	table->entries = table->index = 0;

	table->IIPs = malloc(sizeof(unsigned char *) * size);
	table->MACs = malloc(sizeof(unsigned char *) * size);

	for (i = 0; i < size; i++) {
		table->IIPs[i] = malloc(sizeof(unsigned char) * 2);
		table->MACs[i] = malloc(sizeof(unsigned char) * 6);
	}
}

void freeTable(ARPTable *table)
{
	int i, count;
	count = table->maxsize;

	for (i = 0; i < count; i++) {
		free(table->IIPs[i]);
		free(table->MACs[i]);
	}

	free(table->IIPs);
	free(table->MACs);
}

void readTable(ARPTable *table, char *arpfilename)
{
	int count, i, readCount;
	/* Array 'iip' has been used in a different way to avoid strange
	 * issues in DOS */
	unsigned char iip[3], mac[6];
	FILE *arpfile;

	arpfile = fopen(arpfilename, "r");
	if (arpfile == NULL) {
		fprintf(stderr, "\nInput file '%s' is not accessible.\n", arpfilename);
		freeTable(table);
		exit(1);
	}

	fscanf(arpfile, "%d", &count);
	
	if (count > table->maxsize)
		count = table->maxsize;

	for (i = 0; i < count; i++) {
		readCount = fscanf(arpfile, " %2x:%2x", iip+1, iip+2);

		if (readCount != 2 || readCount == EOF) {
			fprintf(stderr, "\nInput file is not formatted properly.\n");
			freeTable(table);
			exit(1);
		}

		readCount = fscanf(arpfile, " %2x:%2x:%2x:%2x:%2x:%2x", 
			mac, mac+1, mac+2, mac+3, mac+4, mac+5);

		if (readCount != 6 || readCount == EOF) {
			fprintf(stderr, "\nInput file is not formatted properly.\n");
			freeTable(table);
			exit(1);
		}

		addEntry(table, iip+1, mac);
	}

	fclose(arpfile);
}

void printTable(ARPTable table) 
{
	int i;

	for (i = 0; i < table.entries; i++)
		printf("\n#%02d.    %02X:%02X    %02X:%02X:%02X:%02X:%02X:%02X", 
			i+1, table.IIPs[i][0], table.IIPs[i][1], 
			table.MACs[i][0], table.MACs[i][1], table.MACs[i][2], 
			table.MACs[i][3], table.MACs[i][4], table.MACs[i][5]);
}

unsigned char *lookupEntry(ARPTable table, unsigned char *iip)
{
	int i, count;
	count = table.entries;

	for (i = 0; i < count; i++) {
		if (memcmp(iip, table.IIPs[i], 2) == 0)
			break;
	}

	if (i < count)
		return table.MACs[i];
	else 
		return NULL;
}

void addEntry(ARPTable *table, unsigned char *IIP, unsigned char *MAC)
{
	if (table->index == table->maxsize)
		table->index = 0;
	
	memcpy(table->IIPs[table->index], IIP, 2); 
	memcpy(table->MACs[table->index], MAC, 6);

	(table->index)++;

	if (table->entries < table->maxsize)
		(table->entries)++; 
}

