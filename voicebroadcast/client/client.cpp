/**
* Voice UDP Client
* The server is designed to receive audio from one client. Mixed audio playback from multiple clients is not covered here!
* by Kirill Deryabin
* 05.07.2021
* github.com/catalyst2001
*/
#define _CRT_SECURE_NO_WARNINGS
#include "../networkdefs.h"
#include "../audiodefs.h"

socket_t client_socket;

int main()
{
	printf("NET_Init: Initializing network...\n");
	if (NET_Init(&client_socket, NULL, 2030) != NET_OK) {
		printf("Critical error. Client will be closed!\n");
		return -1;
	}

	WAVEFORMATEX waveFormat;
	memset(&waveFormat, 0, sizeof(WAVEFORMATEX));
	waveFormat.cbSize = sizeof(WAVEFORMATEX);    // the size of the structure
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;     // used pulse code modulation audio (PCM)
	waveFormat.nChannels = 1;                    // 2 for stereo, or 1 for mono
	waveFormat.nSamplesPerSec = 44100;           // or whatever the sample rate is (8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000)
	waveFormat.wBitsPerSample = 16;              // or maybe 8 if you have an 8-bit audio file.
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

	//Open in audio device
	HWAVEIN h_wave_in;
	MMRESULT waveInResult = MMSYSERR_NOERROR;
	if ((waveInResult = waveInOpen(&h_wave_in, WAVE_MAPPER, &waveFormat, NULL, 0, CALLBACK_NULL)) != MMSYSERR_NOERROR) {
		char buffer[512];
		waveInGetErrorTextA(waveInResult, buffer, sizeof(buffer));
		printf("Error in waveInOpen: '%s'\n", buffer);
		return -1;
	}

	int i = 0;
	WAVEHDR wave_hdr[VOICEDATA_BUFFERS];
	for (i = 0; i < sizeof(wave_hdr) / sizeof(wave_hdr[0]); i++) {
		wave_hdr[i].dwBufferLength = VOICEDATA_BUFFER_SIZE;
		wave_hdr[i].lpData = (char *)malloc(wave_hdr[i].dwBufferLength * waveFormat.nBlockAlign);
		wave_hdr[i].dwFlags = 0;
		waveInPrepareHeader(h_wave_in, &wave_hdr[i], sizeof(WAVEHDR));
		waveInAddBuffer(h_wave_in, &wave_hdr[i], sizeof(WAVEHDR));
	}

	char ip[512];

repeat:
	printf("paste ip address: ");
	scanf("%s", ip);

	sockaddr_in addr;
	addr.sin_family = AF_INET;

	char *p_port = strchr(ip, ':');
	if (!p_port)
		goto repeat;

	*p_port = 0;
	p_port++;
	int iport = atoi(p_port);
	addr.sin_port = htons(iport);
	addr.sin_addr.s_addr = (ip[0] == '0') ? htonl(INADDR_ANY) : inet_addr(ip);

	waveInStart(h_wave_in); //Start filling buffers with sound data

	i = 0;
	while (true) {
		if (wave_hdr[i].dwFlags & WHDR_DONE) {
			sendto(client_socket, wave_hdr[i].lpData, wave_hdr[i].dwBytesRecorded, 0, (sockaddr*)&addr, sizeof(sockaddr_in)); //send buffer data to server
			printf("buffer: %d  send %d bytes\n", i, wave_hdr[i].dwBytesRecorded);
			waveInAddBuffer(h_wave_in, &wave_hdr[i], sizeof(WAVEHDR)); //now we can fill the buffer with sound information again
			i = (i + 1) % VOICEDATA_BUFFERS; //we increase the counter from zero to the number of buffers, and as soon as we reach the end, we reset the counter
		}
	}

	//Free audio buffers
	for (i = 0; i < sizeof(wave_hdr) / sizeof(WAVEHDR); i++) {
		waveInUnprepareHeader(h_wave_in, &wave_hdr[i], sizeof(WAVEHDR));
		free(wave_hdr[i].lpData);
	}
	waveInClose(h_wave_in); //Сlose the input device. We won't use it anymore :)
	NET_Shutdown(client_socket);
	return 0;
}