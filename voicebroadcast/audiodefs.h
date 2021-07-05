#pragma once
#include <Windows.h>
#include <mmeapi.h>
#pragma comment(lib, "winmm.lib")

#define VOICEDATA_BUFFER_SIZE 2000	//Voice data buffer size
#define VOICEDATA_BUFFERS 15		//Count voice buffers for exchange