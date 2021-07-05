/**
* Voice UDP Server
* The server is designed to receive audio from one client. Mixed audio playback from multiple clients is not covered here!
* by Kirill Deryabin 
* 05.07.2021
* github.com/catalyst2001
*/
#include "../networkdefs.h"
#include "../audiodefs.h"

socket_t server_socket;

int recv_buffer_size = 65535; //network receive buffer size
char *p_recv_buffer = NULL;

int main()
{
	printf("NET_Init: Initializing network...\n");
	if (NET_Init(&server_socket, "192.168.1.2", 8080) != NET_OK) {
		printf("NET_Init: Fatal error. Programm will be closed!\n");
		return -1;
	}
	
	p_recv_buffer = (char *)malloc(recv_buffer_size);
	if (!p_recv_buffer) {
		printf("Cannot allocate receive buffer!\n");
		return -2;
	}

	WAVEFORMATEX wave_format;
	memset(&wave_format, 0, sizeof(WAVEFORMATEX));
	wave_format.cbSize = sizeof(WAVEFORMATEX);    // the size of the structure
	wave_format.wFormatTag = WAVE_FORMAT_PCM;     // used pulse code modulation audio (PCM)
	wave_format.nChannels = 1;                    // 2 for stereo, or 1 for mono
	wave_format.nSamplesPerSec = 44100;           // or whatever the sample rate is (8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000)
	wave_format.wBitsPerSample = 16;              // or maybe 8 if you have an 8-bit audio file.
	wave_format.nBlockAlign = wave_format.nChannels * wave_format.wBitsPerSample / 8;
	wave_format.nAvgBytesPerSec = wave_format.nBlockAlign * wave_format.nSamplesPerSec;

	//Open out audio device
	HWAVEOUT h_wave_out;
	MMRESULT waveOutResult = MMSYSERR_NOERROR;
	if ((waveOutResult = waveOutOpen(&h_wave_out, WAVE_MAPPER, &wave_format, 0, 0, CALLBACK_NULL)) != MMSYSERR_NOERROR) {
		char error[512];
		waveOutGetErrorTextA(waveOutResult, error, sizeof(error));
		printf("Error in waveOutOpen: '%s'\n", error);
		return -2;
	}

	//Allocate audio buffers
	int i;
	WAVEHDR wave_hdr[VOICEDATA_BUFFERS];
	for (i = 0; i < sizeof(wave_hdr) / sizeof(WAVEHDR); i++) {
		memset(&wave_hdr[i], NULL, sizeof(WAVEHDR));
		wave_hdr[i].dwBufferLength = VOICEDATA_BUFFER_SIZE;
		wave_hdr[i].lpData = (char *)malloc(wave_hdr[i].dwBufferLength);
		waveOutPrepareHeader(h_wave_out, &wave_hdr[i], sizeof(WAVEHDR)); //prepare audio block
	}

	//We got to receiving messages from a client! All right : )
	printf("Voice UDP server started\n"); 

	// PLEASE READ THIS!
	// While audio data is being written to the second buffer, the first buffer is already being played
	// Thus, we always have data in advance for playback. This approach solves intermittent audio transmission.
	int currentReciveBuffer = 1; //We start writing data to the first buffer
	int currentPlayingBuffer = 0; //And we play from a zero buffer

	int fromlen = sizeof(sockaddr);
	while (true) {
		sockaddr from;
		int n_bytes = recvfrom(server_socket, p_recv_buffer, recv_buffer_size, 0, &from, &fromlen); //receive bytes
		if (n_bytes > 0) {
			memcpy(wave_hdr[currentReciveBuffer].lpData, p_recv_buffer, wave_hdr[currentReciveBuffer].dwBufferLength); //copy audio data from receive buffer
			waveOutWrite(h_wave_out, &wave_hdr[currentPlayingBuffer], sizeof(WAVEHDR)); //send data to out audio device
			printf("client %d:  %d bytes recieved   playing buffer: %d   preparing buffer: %d\n", from.sa_family, n_bytes, currentPlayingBuffer, currentReciveBuffer);
			
			//increment counter variables until we reach the end
			//how we got to the end, start over
			currentReciveBuffer = (currentReciveBuffer + 1) % VOICEDATA_BUFFERS;
			currentPlayingBuffer = (currentPlayingBuffer + 1) % VOICEDATA_BUFFERS;
		}
	}

	//Free audio buffers
	for (i = 0; i < sizeof(wave_hdr) / sizeof(WAVEHDR); i++) {
		waveOutUnprepareHeader(h_wave_out, &wave_hdr[i], sizeof(WAVEHDR));
		free(wave_hdr[i].lpData);
	}
	waveOutClose(h_wave_out); //close the output device.
	NET_Shutdown(server_socket);
	return 0;
}