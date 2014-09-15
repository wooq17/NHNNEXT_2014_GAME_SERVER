#pragma once

#include "DDConfig.h"
#include "DDCircularBuffer.h"
#include "DDPacketHeader.h"

#define MAX_LOADSTRING 100

#define IDC_SEND_BUTTON	103

const int MAX_PACKET_SIZE = 1024;
const int BUFFER_SIZE = 1024 * 10;

typedef void( *HandlerFunc )( DDPacketHeader& pktBase );

class DDNetwork : public Singleton<DDNetwork>
{
public:
	//friend class DDApplication;

	DDNetwork();
	~DDNetwork();

	bool Init( );
	bool Connect( const char* serverIP, int port );
	void Disconnect();

	void NagleOff();
		
	bool Read();
	void Write( const char* data, size_t size );
	void Send();

	void RegisterHandler( int pktType, HandlerFunc handler );
	static void DefaultHandler( DDPacketHeader& pktBase );
	void HandleInit();
	
	void GetPacketData( char* data, size_t bytes ) { m_RecvBuffer.Read( data, bytes ); }

	void ProcessPacket();
private:

	SOCKET m_Socket;

	SOCKADDR_IN m_ServerAddr;

	DDCircularBuffer m_RecvBuffer;
	DDCircularBuffer m_SendBuffer;

	// packet handle
	HandlerFunc m_HandlerTable[MAX_PACKET_SIZE];
};