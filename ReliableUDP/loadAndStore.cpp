#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PacketSize 256




void extractPacketData(unsigned char* packet, char* fileName, char* transferStatus, char* data)
{
	char delimiter = '#';
	int packetIndex = 0;
	int fileNameIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		if (packet[packetIndex] == '#')
		{
			break;
		}
		fileName[fileNameIndex] = packet[packetIndex];
		fileNameIndex++;
	}
	fileName[fileNameIndex] = '\0';
	packetIndex++;

	int transferStatusIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		if (packet[packetIndex] == '#')
		{
			break;
		}
		transferStatus[transferStatusIndex] = packet[packetIndex];
		transferStatusIndex++;
	}
	transferStatus[transferStatusIndex] = '\0';
	packetIndex++;

	int dataIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		data[dataIndex] = packet[packetIndex];
		dataIndex++;
	}
}