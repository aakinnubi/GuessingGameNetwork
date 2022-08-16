#include <enet/enet.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

ENetHost* NetHost = nullptr;
ENetPeer* Peer = nullptr;
bool IsServer = false;
thread* PacketThread = nullptr;
int secretNum = 0;
bool numGuessed = false;
int playerId = 0;

void BroadcastGuess(int guess);
void BroadcastGuessResponse(int guess, string outputMessage);

enum PacketHeaderTypes
{
	PHT_Invalid = 0,
	PHT_Position,
	PHT_Count,
	PHT_Guess,
	PHT_GuessResponse
};

struct GamePacket
{
	GamePacket() {}
	PacketHeaderTypes Type = PHT_Invalid;
};

struct CountPacket : public GamePacket
{
	CountPacket()
	{
		Type = PHT_Count;
	}

	int playerCount = 0;
};

struct GuessPacket : public GamePacket
{
	GuessPacket()
	{
		Type = PHT_Guess;
	}

	int guess = 0;
};

struct GuessResponsePacket : public GamePacket
{
	GuessResponsePacket()
	{
		Type = PHT_GuessResponse;
	}

	int guess = 0;
	string message = "";
};

struct PositionPacket : public GamePacket
{
	PositionPacket()
	{
		Type = PHT_Position;
	}

	int x = 0;
	int y = 0;
};

//can pass in a peer connection if wanting to limit
bool CreateServer()
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = 1234;
	NetHost = enet_host_create(&address /* the address to bind the server host to */,
		32      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);

	return NetHost != nullptr;
}

bool CreateClient()
{
	NetHost = enet_host_create(NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		0 /* assume any amount of incoming bandwidth */,
		0 /* assume any amount of outgoing bandwidth */);

	return NetHost != nullptr;
}

bool AttemptConnectToServer()
{
	ENetAddress address;
	/* Connect to some.server.net:1234. */
	enet_address_set_host(&address, "127.0.0.1");
	address.port = 1234;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	Peer = enet_host_connect(NetHost, &address, 2, 0);
	return Peer != nullptr;
}

void ServerResponse(int guess)
{
	string message;

	if (guess == secretNum)
	{
		message = "You're right!";
	}
	else if (guess < secretNum)
	{
		message = "Too low! Try again.";
	}
	else if (guess > secretNum)
	{
		message = "Too high! Try again.";
	}

	BroadcastGuessResponse(guess, message);
}

void PlayerResponse()
{
	int guess = -1;
	while (guess == -1)
	{
		cin >> guess;

		BroadcastGuess(guess);
	}
}

void HandleReceivePacket(const ENetEvent& event)
{
	GamePacket* RecGamePacket = (GamePacket*)(event.packet->data);
	if (RecGamePacket)
	{
		cout << "Received Game Packet " << endl;

		switch (RecGamePacket->Type)
		{
		case PHT_Count:
		{
			CountPacket* countGamePacket = (CountPacket*)(event.packet->data);
			if (countGamePacket)
			{
				cout << "There are currently " << countGamePacket->playerCount << " players." << endl;
				//cout << "Your Player ID is: " << countGamePacket->playerId << endl;

				//if(countGamePacket->playerCount >= 2)
				{
					//BroadcastGuess(20);
					cout << "Try to guess the number between 1-100!" << endl;
					PlayerResponse();
				}
			}

			break;
		}
		case PHT_Guess:
		{
			GuessPacket* guessGamePacket = (GuessPacket*)(event.packet->data);
			if (guessGamePacket)
			{
				string message;

				cout << "Player " << playerId << " guessed the number " << guessGamePacket->guess << endl;

				ServerResponse(guessGamePacket->guess);
			}

			break;
		}
		case PHT_GuessResponse:
		{
			GuessResponsePacket* guessResponseGamePacket = (GuessResponsePacket*)(event.packet->data);
			if (guessResponseGamePacket)
			{
				string message;

				if (guessResponseGamePacket->guess == secretNum)
				{
					numGuessed = true;
					cout << "You're right!" << endl;
				}
				else if (guessResponseGamePacket->guess < secretNum)
				{
					cout << "Too low! Try again." << endl;
					PlayerResponse();
				}
				else if (guessResponseGamePacket->guess > secretNum)
				{
					cout << "Too high! Try again." << endl;
					PlayerResponse();
				}
			}

			break;
		}
		default:
		{
			cout << "Uh, oh! You shouldn't be here." << endl;
		}
		}
	}
	else
	{
		cout << "Invalid Packet " << endl;
	}

	/* Clean up the packet now that we're done using it. */
	enet_packet_destroy(event.packet);
	{
		enet_host_flush(NetHost);
	}
}

void BroadcastIsCountPacket(int numClients)
{
	CountPacket* countPacket = new CountPacket();
	countPacket->playerCount = numClients;
	playerId = numClients;
	ENetPacket* packet = enet_packet_create(countPacket,
		sizeof(CountPacket),
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(NetHost->peers, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete countPacket;
}

void BroadcastGuess(int guess)
{
	GuessPacket* guessPacket = new GuessPacket();
	guessPacket->guess = guess;
	ENetPacket* packet = enet_packet_create(guessPacket,
		sizeof(GuessPacket),
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete guessPacket;
}

void BroadcastGuessResponse(int guess, string outputMessage)
{
	GuessResponsePacket* guessResponsePacket = new GuessResponsePacket();
	guessResponsePacket->guess = guess;
	guessResponsePacket->message = outputMessage;
	ENetPacket* packet = enet_packet_create(guessResponsePacket,
		sizeof(GuessResponsePacket),
		ENET_PACKET_FLAG_RELIABLE);

	/* One could also broadcast the packet by         */
	enet_host_broadcast(NetHost, 0, packet);
	//enet_peer_send(event.peer, 0, packet);

	/* One could just use enet_host_service() instead. */
	//enet_host_service();
	enet_host_flush(NetHost);
	delete guessResponsePacket;
}

void ServerProcessPackets()
{
	int numClients = 0;

	while (numGuessed == false)
	{
		ENetEvent event;
		while (enet_host_service(NetHost, &event, 30000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
			{
				numClients += 1;

				cout << "A new client connected from "
					<< event.peer->address.host
					<< ":" << event.peer->address.port
					<< endl;
				/* Store any relevant client information here. */
				event.peer->data = (void*)("Client information");
				BroadcastIsCountPacket(numClients);
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				HandleReceivePacket(event);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				cout << (char*)event.peer->data << "disconnected." << endl;
				/* Reset the peer's client information. */
				event.peer->data = NULL;
				//notify remaining player that the game is done due to player leaving
				break;
			}
			}
		}
	}
}

void ClientProcessPackets()
{
	while (numGuessed == false)
	{
		ENetEvent event;
		/* Wait up to 1000 milliseconds for an event. */
		while (enet_host_service(NetHost, &event, 1000) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
			{
				cout << "Connection succeeded " << endl;
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				HandleReceivePacket(event);
				break;
			}
			}
		}
	}
}

int main(int argc, char** argv)
{
	srand(time(NULL));

	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		cout << "An error occurred while initializing ENet." << endl;
		return EXIT_FAILURE;
	}
	atexit(enet_deinitialize);

	cout << "1) Create Server " << endl;
	cout << "2) Create Client " << endl;
	int UserInput;
	cin >> UserInput;
	if (UserInput == 1)
	{
		//How many players?

		if (!CreateServer())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet server.\n");
			exit(EXIT_FAILURE);
		}

		secretNum = rand() % 100 + 1;
		IsServer = true;
		cout << "The secret number is " << secretNum << endl;
		cout << "waiting for players to join..." << endl;
		PacketThread = new thread(ServerProcessPackets);
	}
	else if (UserInput == 2)
	{
		if (!CreateClient())
		{
			fprintf(stderr,
				"An error occurred while trying to create an ENet client host.\n");
			exit(EXIT_FAILURE);
		}

		ENetEvent event;
		if (!AttemptConnectToServer())
		{
			fprintf(stderr,
				"No available peers for initiating an ENet connection.\n");
			exit(EXIT_FAILURE);
		}

		PacketThread = new thread(ClientProcessPackets);

		//handle possible connection failure
		{
			//enet_peer_reset(Peer);
			//cout << "Connection to 127.0.0.1:1234 failed." << endl;
		}
	}
	else
	{
		cout << "Invalid Input" << endl;
	}


	if (PacketThread)
	{
		PacketThread->join();
	}
	delete PacketThread;
	if (NetHost != nullptr)
	{
		enet_host_destroy(NetHost);
	}

	return EXIT_SUCCESS;
}