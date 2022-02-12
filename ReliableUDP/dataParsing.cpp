/*
* FILE : dataParsing.cpp
* PROJECT : Assignment 1 : ReliableUDP
* PROGRAMMER : Deep Patel and Amritpal Singh
* FIRST VERSION : 2022-02-09
* DESCRIPTION : This file contains one method that will parse each and every packet
* sent by the client to the server.
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PacketSize 256



/*
* Function : extractPacketData()
* Parameter : unsigned char* packet, char* fileName, char* transferStatus, char* data, char* gotFileSize
* return : none
* Description: This functions will parse each and every packet sent by the client to the server.
*/
void extractPacketData(unsigned char* packet, char* fileName, char* transferStatus, char* data, char* gotFileSize)
{
	int packetIndex = 0;
	int fileNameIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		if (packet[packetIndex] == '~')
		{
			break;
		}
		fileName[fileNameIndex] = packet[packetIndex];
		fileNameIndex++;
	}
	fileName[fileNameIndex] = '\0';
	packetIndex++;
	
	int fileSizeIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		if (packet[packetIndex] == '~')
		{
			break;
		}
		gotFileSize[fileSizeIndex] = packet[packetIndex];
		fileSizeIndex++;
	}
	gotFileSize[fileSizeIndex] = '\0';
	packetIndex++;


	int transferStatusIndex = 0;
	for (; packetIndex < PacketSize; packetIndex++)
	{
		if (packet[packetIndex] == '~')
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