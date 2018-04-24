#include "SDL.h"
#include "SDL_net.h"

#include <iostream>
#include <string>
#include <time.h>
#include "maths.h"

#define DEBUG 0

using namespace std;

struct WindowDims
{
	int x = 100;
	int y = 100;
	int w = 1000;
	int h = 700;
	int midW = (int)((w / 2) + 0.5f);
	int midH = (int)((h / 2) + 0.5f);
};

struct Keys
{
	bool w;
	bool s;
	bool a;
	bool d;
};

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

void drawRect(SDL_Renderer* renderer, Vec2 pos, Vec2 size, Colour colour)
{
	SDL_Rect rect = { pos.x, pos.y, size.x, size.y };
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, 255);
	SDL_RenderFillRect(renderer, &rect);
}

#if DEBUG

Uint32 start = 0;
char* name = "";

void timer(char* text)
{
	if (start == 0)
	{
		start = SDL_GetTicks();
	}
	else
	{
		cout << name << ": " << SDL_GetTicks() - start << endl;
		start = SDL_GetTicks();
	}
	name = text;
}

#else

void timer(char* text){}

#endif

int main(int argc, char* args[])
{
	timer("Start");

	SDL_Init(SDL_INIT_EVERYTHING);
	SDLNet_Init();

	//ask for ip
	cout << "Enter Server IP: ";
	char ipName[30];
	cin >> ipName;

	//init game things
	Keys key = {};
	bool gameRunning = true;
	srand(time(0));

	Box box = {};
	box.pos = { 100.0f, 100.0f };
	box.size = { randInt(20, 80), randInt(20, 80)};
	box.colour = { (unsigned char)(rand() % 255 - 10.0f),(unsigned char)(rand() % 255 - 10.0f),(unsigned char)(rand() % 255 - 10.0f) };
	Box otherBoxList[50] = {};

	//connect to server
	cout << "connecting to server..." << endl;

	int serverPort = 51114;
	int packetSize = 2048;

	UDPpacket* sendPacket = SDLNet_AllocPacket(packetSize);
	UDPpacket* recvPacket = SDLNet_AllocPacket(packetSize);

	UDPsocket clientSocket = SDLNet_UDP_Open(0);

	IPaddress serverIP = {};
	SDLNet_ResolveHost(&serverIP, ipName, serverPort); //set serverIP

	sendPacket->address.host = serverIP.host;
	sendPacket->address.port = serverIP.port;
	sendPacket->len = 1;
	*(sendPacket->data) = (char)0;

	while (true)
	{
		SDLNet_UDP_Send(clientSocket, -1, sendPacket);
		SDL_Delay(50);
		if (SDLNet_UDP_Recv(clientSocket, recvPacket))
		{
			box.id = (char)*(recvPacket->data);
			break;
		}
	}

	cout << "connected" << endl;

	//init screen
	WindowDims windowDims;
	SDL_Window* window = SDL_CreateWindow("Title", windowDims.x, windowDims.y, windowDims.w, windowDims.h, 0); //look at flags for fullscreen
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC); //uses vsync, look at flags to change
	SDL_RenderSetLogicalSize(renderer, windowDims.w, windowDims.h);

	//main game loop
	while (gameRunning)
	{
		timer("Events");

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				gameRunning = false;
			else if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
				{
					gameRunning = false;
					break;
				}
				case SDLK_w:
				{
					key.w = true;
					break;
				}
				case SDLK_s:
				{
					key.s = true;
					break;
				}
				case SDLK_a:
				{
					key.a = true;
					break;
				}
				case SDLK_d:
				{
					key.d = true;
					break;
				}
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
				{
					key.w = false;
					break;
				}
				case SDLK_s:
				{
					key.s = false;
					break;
				}
				case SDLK_a:
				{
					key.a = false;
					break;
				}
				case SDLK_d:
				{
					key.d = false;
					break;
				}
				}
			}
		}

		timer("Update Box");

		//update box
		box.accel = {};
		if (!(key.w && key.s))
		{
			if (key.w)
			{
				box.accel.y = -4.0f;
			}
			if (key.s)
			{
				box.accel.y = 4.0f;
			}
		}
		if (!(key.a && key.d))
		{
			if (key.a)
			{
				box.accel.x = -4.0f;
			}
			if (key.d)
			{
				box.accel.x = 4.0f;
			}
		}

		box.vel += box.accel;
		box.pos += box.vel;
		box.vel *= 0.8f;

		if (box.pos.x < 0.0f)
		{
			box.pos.x = 0.0f;
		}
		else if (box.pos.x + box.size.x > windowDims.w)
		{
			box.pos.x = windowDims.w - box.size.x;
		}
		if (box.pos.y < 0.0f)
		{
			box.pos.y = 0.0f;
		}
		else if (box.pos.y + box.size.y > windowDims.h)
		{
			box.pos.y = windowDims.h - box.size.y;
		}

		timer("Send Box info");

		//send box info
		sendPacket->len = sizeof(Box);
		memcpy(sendPacket->data, &box, sendPacket->len);

		SDLNet_UDP_Send(clientSocket, -1, sendPacket);

		timer("Update other boxes");

		//update other boxes
		Uint32 startTime = SDL_GetTicks();
		while (!SDLNet_UDP_Recv(clientSocket, recvPacket)) 
		{
			if ((SDL_GetTicks() - startTime) > ((SDL_GetPerformanceFrequency() / 1000) * 2))
			{
				break;
			}
		};

		memcpy(otherBoxList, recvPacket->data, recvPacket->len);

		timer("Render");

		//render
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		drawRect(renderer, box.pos, box.size, box.colour);

		for (int i = 0; i < 50; i++)
		{
			Box &otherBox = otherBoxList[i];
			if (otherBox.id != 0)
			{
				drawRect(renderer, otherBox.pos, otherBox.size, otherBox.colour);
			}
		}

		SDL_RenderPresent(renderer);
	}

	SDLNet_Quit();
	SDL_Quit();
	return 0;

}