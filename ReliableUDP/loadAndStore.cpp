#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PacketSize 256
#pragma warning(disable: 4996)





char* loadFileData(char* fileName)
{
	int totalSeq = 0;

	FILE* ifp = NULL;
	// open input file
	ifp = fopen(fileName, "rb");

	if (ifp == NULL)
	{
		printf("Can't open input file\n");
	}
	// loop while we are still reading data in from the file
	else {
		fseek(ifp, 0, SEEK_END);
		long fileSize = ftell(ifp);
		totalSeq = (int)(fileSize / PacketSize);

		fseek(ifp, 0, SEEK_SET);
		char* data = (char*)malloc(fileSize + 1);


		while (!feof(ifp))
		{

			int howManyRead = 0;
			// as long as we are reading at least one character (indicated by the return value
			// from fread), keep going
			if ((howManyRead = fread(data, sizeof(char), fileSize, ifp)) != 0)
			{
				return data;
			}
		}
	}
}








#pragma warning (disable:4996)


int AddHeader(char* fileName, char* transferStatus, char* data)
{
	char temp[5000] = " ";
	char delimiter = '#';

	strcat(temp, fileName);
	strncat(temp, &delimiter, 1);
	strcat(temp, transferStatus);
	strncat(temp, &delimiter, 1);
	strcat(temp, data);


	strcpy(data, temp);

	return 0;
}


int ExtractHeader(char* fileName, char* transferStatus, char* input, char* data)
{


	char delimiter = '#';

	char c = 'a';
	int i = 1;
	int j = 1;
	while (c != '\0')
	{
		c = input[i];

		if (j != 3)
		{
			if (c == delimiter)
			{
				j++;
				i++;
				continue;
			}
		}

		if (j == 1)
		{
			strncat(fileName, &c, 1);
		}
		else if (j == 2)
		{
			strncat(transferStatus, &c, 1);
		}
		else if (j == 3)
		{
			strncat(data, &c, 1);
		}


		i++;
	}

	return 0;
}