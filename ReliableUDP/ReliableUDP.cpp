/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "functionPrototypes.h"


#pragma warning(disable: 4996)



//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------

int main(int argc, char* argv[])
{

	char fileName[21];
	char task[3];
	
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	if (argc != 2 && argc != 4)
	{
		printf("Error: Not enough commands passed\n");
		printf("Usage: Run As Server:- ReliableUDP.exe 0 ---- Run As Client:- ReliableUDP.exe ServerIP FileName Task{-r(request) or -s(send)}]\n");
	}
	else {
		if (argc == 2)
		{
			mode = Server;
		}
		else if(argc >= 2)
		{
			int a, b, c, d;
			#pragma warning(suppress : 4996)
			if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
			{
				mode = Client;
				if (mode == Client)
				{
					strcpy(fileName, argv[2]);

					strcpy(task, argv[3]);
					if (strcmp(task,"-r") == 0)
					{
						printf("Requesting file...\n");
					}
					else
					{
						printf("Sending file...\n");
					}
				}
				address = Address(a, b, c, d, ServerPort);
			}
		}

		// initialize

		if (!InitializeSockets())
		{
			printf("failed to initialize sockets\n");
			return 1;
		}

		ReliableConnection connection(ProtocolId, TimeOut);

		const int port = mode == Server ? ServerPort : ClientPort;

		if (!connection.Start(port))
		{
			printf("could not start connection on port %d\n", port);
			return 1;
		}

		if (mode == Client)
			connection.Connect(address);
		else
		connection.Listen();

		bool connected = false;
		float sendAccumulator = 0.0f;
		float statsAccumulator = 0.0f;

		FlowControl flowControl;

		int packets_Sent = 1;

		char hash[7] = "";
		char* inputFileData = NULL; //read file data stored here
		char* strFileContent = NULL;
		long transferredLength = 0;

		bool isFinishedTransfer = false;


		//Reading file for sending
		if (strcmp(task, "-s") == 0)
		{
			// --------------------------------------------------------------------------------
			FILE* ifp;
			ifp = fopen(fileName, "rb");

			//Find length of file
			fseek(ifp, 0, SEEK_END);
			long fileSize = ftell(ifp);
			fseek(ifp, 0, SEEK_SET);

			//read in the data from your file
			inputFileData = (char*)malloc(fileSize + 1);
			fread(inputFileData, sizeof(char), fileSize, ifp);

			
			// --------------------------------------------------------------------------------
		}







		while (true)
		{
			// update flow control

			if (connection.IsConnected())
				flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

			const float sendRate = flowControl.GetSendRate();

			// detect changes in connection state

			if (mode == Server && connected && !connection.IsConnected())
			{
				flowControl.Reset();
				printf("reset flow control\n");
				connected = false;
			}

			if (!connected && connection.IsConnected())
			{
				printf("client connected to server\n");
				connected = true;
			}

			if (!connected && connection.ConnectFailed())
			{
				printf("connection failed\n");
				break;
			}

			// send and receive packets
			/*
			* Ther server will first check what type of request it is.
			* If the request is for the sending a file, it will get the file name
			* or any relative/absolute path if client knows.
			* It will then search for that file using an algorithm that we will create.
			* It will be like a loop. After the file is found, it will send the file with ReliableUDP.
			*/

			sendAccumulator += DeltaTime;


			while (sendAccumulator > 1.0f / sendRate)
			{
				if (isFinishedTransfer)
				{
					break;
				}

				unsigned char packet[PacketSize] = "\0";
				memset(packet, 0, sizeof(packet));
				char status[25] = "Processing";
				char data[PacketSize];


				if (inputFileData &&  strlen(inputFileData) > transferredLength)
				{
					int currPacketSize = PacketSize - 35 - strlen(fileName) - strlen(status);
					memcpy(data, &inputFileData[transferredLength], currPacketSize);
					transferredLength += currPacketSize;
					AddHeader(fileName, status, data);
					memcpy(packet, data, sizeof(data));
					connection.SendPacket(packet, sizeof(packet));
					sendAccumulator -= 1.0f / sendRate;
				}
				else
				{
					strcpy(status, "Done");

					AddHeader(fileName, status, data);
					memcpy(packet, data, sizeof(data));
					connection.SendPacket(packet, sizeof(packet));


					memset(packet, 0, sizeof(packet));
					connection.SendPacket(packet, sizeof(packet));
					sendAccumulator -= 1.0f / sendRate;

					isFinishedTransfer = true;
					break;
				}
			}


			char recData[PacketSize];
			string fileContent;
			char status[25] = "Processing";

			while (true)
			{
				unsigned char packet[256];
				memset(packet, 0, sizeof(packet));
				int bytes_read = connection.ReceivePacket(packet, sizeof(packet));

				if (bytes_read == 0)
				{
					break;
				}
				else
				{
					printf("%s\n", packet);
				}

				char _fileName[150] = "";
				strcpy(status, "");
				strcpy(recData, "");
				ExtractHeader(_fileName, status, (char*)packet, recData);

				if (strcmp(status, "Done") == 0)
				{
					ofstream oFile;

					oFile.open("rev.txt", std::ios::binary | std::ios::out);
					oFile.write(fileContent.c_str(), fileContent.length());
					oFile.close();
					//hash = CalculateMd5Hash("rev.txt");

					/*if (strcmp(hash.c_str(), data) == 0)
					{
						printf("File transfer successfully\n");

					}
					else
					{
						printf("File transfer failed\n");
					}*/
				}
				else if (strcmp(status, "Processing") == 0)
				{
					string data2(recData);
					fileContent += data2;
				}
			}

			// show packets that were acked this frame

#ifdef SHOW_ACKS
			unsigned int* acks = NULL;
			int ack_count = 0;
			connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
			if (ack_count > 0)
			{
				printf("acks: %d", acks[0]);
				for (int i = 1; i < ack_count; ++i)
					printf(",%d", acks[i]);
				printf("\n");
			}
#endif

			// update connection

			connection.Update(DeltaTime);

			// show connection stats

			statsAccumulator += DeltaTime;

			while (statsAccumulator >= 0.25f && connection.IsConnected())
			{
				float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

				unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
				unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
				unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

				float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
				float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

				printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
					rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
					sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
					sent_bandwidth, acked_bandwidth);

				statsAccumulator -= 0.25f;
			}

			net::wait(DeltaTime);
		}

		ShutdownSockets();
	}
	return 0;
}
