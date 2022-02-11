/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include "Net.h"
#include "functionPrototypes.h"
using namespace std;
using namespace std::chrono;


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

	char fileName[121] = "\0";
	char task[3];
	high_resolution_clock::time_point t1;
	high_resolution_clock::time_point t2;

	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	if (argc != 1 && argc != 4)
	{
		printf("Error: Not Enough Command Line Arguments Passed\n");
		printf("Usage: Please Run it as following\m"
			"Run As Server: ReliableUDP.exe\n"
			"Run As Client: ReliableUDP.exe ServerIPAddress FileName Task[-r(request) or -s(send)}]\n");
	}
	else {
		if (argc == 1)
		{
			mode = Server;
		}
		else if(argc == 4)
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
					else if (strcmp(task, "-s") == 0)
					{
						printf("Sending file...\n");
					}
					else
					{
						printf("Error: Not Enough Command Line Arguments Passed\n");
						printf("Usage: Please Run it as following\m"
							"Run As Server: ReliableUDP.exe\n"
							"Run As Client: ReliableUDP.exe ServerIPAddress FileName Task[-r(request) or -s(send)}]\n");
					}
				}
				address = Address(a, b, c, d, ServerPort);
			}
		}

		// initialize
		if (!InitializeSockets())
		{
			printf("Failed to initialize sockets\n");
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

		int sentPakets = 1;

		//char hash[7] = "";
		char* inputFileData = NULL;
		long transferredLength = 0;
		long fileSize = 0;

		bool isFinishedTransfer = false;


		//We are reading the file here for sending
		if (strcmp(task, "-s") == 0)
		{
			FILE* ifp;
			ifp = fopen(fileName, "rb");

			//Find length of file
			fseek(ifp, 0, SEEK_END);
			fileSize = ftell(ifp);
			fseek(ifp, 0, SEEK_SET);

			//read in the data from your file
			
			inputFileData = (char*)malloc(fileSize + 1);
			while (!feof(ifp))
			{
				int howManyRead = 0;

				// as long as we are reading at least one character (indicated by the return value
				// from fread), keep going
				if ((howManyRead = fread(inputFileData, sizeof(char), fileSize+1, ifp)) != 0)
				{
					inputFileData[howManyRead] = '\0';
				}
			}
			if (fclose(ifp) != 0)
			{
					printf("Error closing input file\n");
			}
		}

		//Upto here we have read the whole in inputFileData Variable and also determined the whole file Size.....







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

			

			sendAccumulator += DeltaTime;

			/*std::clock_t c_start = std::clock();
			auto t_start = std::chrono::high_resolution_clock::now();
			long timeTaken = 0.00;*/
			t1 = high_resolution_clock::now();


			while (sendAccumulator > 1.0f / sendRate)
			{

				//sending data here in packets

				if (isFinishedTransfer)
				{
					break;
				}

				unsigned char packet[PacketSize] = "\0";
				memset(packet, 0, sizeof(packet));
				char status[15] = "Processing";
				char body[PacketSize];
				memset(body, 0, sizeof(body));


				if (inputFileData &&  fileSize > transferredLength)
				{
					//we will add headers in the packet now....
					strcat((char*)packet, fileName);
					strcat((char*)packet, "#");
					strcat((char*)packet, status);
					strcat((char*)packet, "#");

					//adding file contents here in body
					int bodySize = PacketSize - strlen((char*)packet) - 1;
					strncpy(body, &inputFileData[transferredLength], bodySize);
					

					strncat((char*)packet, body, bodySize);

					int packetLenthWithBody = strlen((char*)packet);
					packet[PacketSize - 1] = '\0';
					connection.SendPacket(packet, sizeof(packet));
					sendAccumulator -= 1.0f / sendRate;

					//update the transferred length
					transferredLength += bodySize;
				}
				else if (inputFileData && fileSize <= transferredLength)
				{
					strcpy(status, "complete");
					strcat((char*)packet, fileName);
					strcat((char*)packet, "#");
					strcat((char*)packet, status);
					strcat((char*)packet, "#");

					connection.SendPacket(packet, sizeof(packet));
					sendAccumulator -= 1.0f / sendRate;

					isFinishedTransfer = true;
					break;
				}
			}


			string fileData;

			while (true)
			{
				char receivedData[PacketSize];
				char status[15] = "Processing";

				unsigned char packet[PacketSize];
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

				char recFileName[121] = "";
				strcpy(status, "");
				strcpy(receivedData, "");
				extractPacketData(packet, recFileName, status, receivedData);

				if (strcmp(status, "complete") == 0)
				{


					t2 = high_resolution_clock::now();

					double dif = duration_cast<nanoseconds>(t2 - t1).count();
					printf("Elasped time is %lf nanoseconds.\n", dif);

					ofstream ofp;
					ofp.open("rev.txt", std::ios::binary | std::ios::out);
					ofp.write(fileData.c_str(), fileData.length());
					ofp.close();
					exit(1);
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
					string dataCopy(receivedData);
					fileData += dataCopy;
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
