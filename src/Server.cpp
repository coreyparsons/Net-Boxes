#include "SDL_net.h"
#include <iostream>
#include <string>

#include "maths.h"

using namespace std;

struct Colour
{
	unsigned char r, g, b;
};

struct Box
{
	char id;
	Vec2 pos;
	Vec2 size;
	Vec2 vel;
	Vec2 accel;
	Colour colour;
};


int main(int argc, char* args[])
{
	SDLNet_Init();

	int serverPort = 51114;
	int packetSize = 2048;

	const int packetListSize = 20;
	UDPpacket* sendPacketList[packetListSize];
	for (int i = 0; i < packetListSize; i++)
	{
		sendPacketList[i] = SDLNet_AllocPacket(packetSize);
	}

	UDPpacket* recvPacket = SDLNet_AllocPacket(packetSize);
	UDPsocket serverSocket = SDLNet_UDP_Open(serverPort);

	Box dataList[packetListSize] = {};

	char idCount = 1;

	while (true)
	{
		if (SDLNet_UDP_Recv(serverSocket, recvPacket) != 0)
		{
			if (((string)((char*)recvPacket->data) == ""))
			{
				for (int i = 0; i < packetListSize; i++)
				{
					UDPpacket* &sendPacket = sendPacketList[i];
					if (sendPacket->address.host == 0)
					{
						sendPacket->address.host = recvPacket->address.host;
						sendPacket->address.port = recvPacket->address.port;

						sendPacket->len = 1;
						*(sendPacket->data) = idCount;

						SDLNet_UDP_Send(serverSocket, -1, sendPacket);

						cout << "connected to client " << (int)idCount << endl;
						idCount++;
						break;
					}
				}
			}
			else
			{
				Box* box = (Box*)(recvPacket->data);
				dataList[box->id - 1] = *box;

				UDPpacket* &sendPacket = sendPacketList[box->id - 1];
				sendPacket->len = sizeof(Box) * packetListSize;
				memcpy(sendPacket->data, dataList, sendPacket->len);

				SDLNet_UDP_Send(serverSocket, -1, sendPacket);
			}
		}
	}

	SDLNet_Quit();
	return 0;
}