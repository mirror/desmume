#ifndef _WINDRIVER_H_
#define _WINDRIVER_H_

#define WIN32_LEAN_AND_MEAN
#include "../common.h"
#include "CWindow.h"

#ifdef EXPERIMENTAL_WIFI
#include <pcap.h>
#include <remote-ext.h> //uh?

//because the pcap headers are written poorly, we need to declare these as cdecl
//this may cause the code to fail to compile on non-windows platforms;
//we may have to use a macro to call these functions which chooses whether to call them
//through the namespace
namespace PCAP {
	extern "C" __declspec(dllexport) int __cdecl pcap_findalldevs_ex(char *source, struct pcap_rmtauth *auth, pcap_if_t **alldevs, char *errbuf);
	extern "C" __declspec(dllexport) int __cdecl pcap_sendpacket(pcap_t *, const u_char *, int);
	extern "C" __declspec(dllexport) void __cdecl pcap_close(pcap_t *);
	extern "C" __declspec(dllexport) pcap_t* __cdecl pcap_open(const char *source, int snaplen, int flags, int read_timeout, struct pcap_rmtauth *auth, char *errbuf);
	extern "C" __declspec(dllexport) void	__cdecl pcap_freealldevs(pcap_if_t *);
}

#endif

extern WINCLASS	*MainWindow;

class Lock {
public:
	Lock();
	~Lock();
};

#endif