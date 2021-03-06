#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <istream>
#include <vector>
#include <string>
#include <detours.h>
#include <hde.h>
#include <hde.cpp>

typedef struct Packet {
	// [eax]
	char *packet;

	// [eax+4]
	int idk; // always 0x7F8?

	// [eax+8]
	int length; // entire packet len, including last null byte
};

static char invisibleOnPacket[] = {
	0x06,
	0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
	0x01, // On
	0x00 // EOF
};

static char invisibleOffPacket[] = {
	0x06,
	0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
	0x00, // Off
	0x00 // EOF
};

DWORD *cPacket = 0;

typedef char (__thiscall *tSendPacket)(DWORD *, __int16, Packet *);
MologieDetours::Detour<tSendPacket>* detour_SendPacket = NULL;

// sendPacket
char __fastcall hook_SendPacket(DWORD *_this, DWORD *edx, __int16 opcode, Packet *data)
{
	if (cPacket != _this) {
		cPacket = _this;
		std::cout << "Changed CPacket address to " << std::hex << _this << std::endl;
	}

	return detour_SendPacket->GetOriginalFunction()(_this, opcode, data);
}

void sendImmortalOn()
{
	struct Packet p;
	p.packet = &invisibleOnPacket[0];
	p.idk = 0x7F8;
	p.length = sizeof(invisibleOnPacket);

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x1461, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void sendImmortalOff()
{
	struct Packet p;
	p.packet = &invisibleOffPacket[0];
	p.idk = 0x7F8;
	p.length = sizeof(invisibleOffPacket);

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x1461, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void sendInvisibleOn()
{
	struct Packet p;
	p.packet = &invisibleOnPacket[0];
	p.idk = 0x7F8;
	p.length = sizeof(invisibleOnPacket);

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x1464, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void sendInvisibleOff()
{
	struct Packet p;
	p.packet = &invisibleOffPacket[0];
	p.idk = 0x7F8;
	p.length = sizeof(invisibleOffPacket);

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x1464, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void sendSuperRun(int speed)
{
	std::cout << "[sendSuperRun] Setting speed to " << speed << std::endl;

	char packet[] = {
		0x06,
		0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
		0x05, // Speed, pos 7
		0x00 // EOF
	};

	packet[7] = (char)speed;

	struct Packet p;
	p.packet = packet;
	p.idk = 0x7F8;
	p.length = sizeof(packet);

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x1458, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void sendSetMusic(std::string filename)
{
	char header[] = {
		0x06, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
	};

	std::vector<char> packet;
	packet.insert(packet.end(), header, header + sizeof(header));
	const char *name = filename.c_str();
	packet.insert(packet.end(), name, name + strlen(name));
	packet.push_back(0x00);
	
	struct Packet p;
	p.packet = (char *)packet.data();
	p.idk = 0x7F8;
	p.length = packet.size();

	bool success = (bool)detour_SendPacket->GetOriginalFunction()(cPacket, 0x14AD, &p);

	std::cout << (success ? "OK!" : "Failed to update state.") << std::endl;
}

void gmLoop()
{
	while (cPacket == 0) {
		Sleep(1000);
	}

	while (true) {
		std::string command;
		std::cout << "> ";
		std::getline(std::cin, command);

		if (command == "invisible") {
			sendInvisibleOn();
		}
		else if (command == "visible") {
			sendInvisibleOff();
		}
		else if (command == "immortal") {
			sendImmortalOn();
		}
		else if (command == "mortal") {
			sendImmortalOff();
		}
		else if (command.length() > 9 && command.substr(0, 9) == "setmusic ") {
			std::vector<std::string> vecParam;

			std::stringstream ss(command);
			std::string temp;
			while (ss >> temp) {
				vecParam.push_back(temp);
			}

			if (vecParam.size() != 2) {
				std::cout << "Usage: setmusic {filename}" << std::endl;
				continue;
			}

			sendSetMusic(vecParam[1]);
		}
		else if (command.length() > 6 && command.substr(0, 6) == "speed ") {
			std::vector<std::string> vecParam;

			std::stringstream ss(command);
			std::string temp;
			while (ss >> temp) {
				vecParam.push_back(temp);
			}

			// Check size
			if (vecParam.size() != 2) {
				std::cout << "Usage: speed {1~5}" << std::endl;
				continue;
			}

			if (vecParam[1].find_first_not_of("12345") != std::string::npos) {
				std::cout << "Invalid speed, will not parse." << std::endl;
				std::cout << "Usage: speed {1~5}" << std::endl;
				continue;
			}

			int val = std::stoi(vecParam[1]);

			if (val < 1 || val > 5) {
				std::cout << "Invalid speed." << std::endl;
				std::cout << "Usage: speed {1~5}" << std::endl;
				continue;
			}

			sendSuperRun(val);
		}
		else {
			std::cout << "Unknown command!" << std::endl;
		}
	}
}

void installHooks()
{
	detour_SendPacket = new MologieDetours::Detour<tSendPacket>((tSendPacket)0x0042D240, (tSendPacket)hook_SendPacket);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		AllocConsole();

		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);

		installHooks();

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)gmLoop, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void __v0() {}