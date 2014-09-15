#pragma once

struct DDPacketHeader
{
	DDPacketHeader( ) : mSize( 0 ), mType( 0 ) {}
	unsigned short mSize;
	short mType;
};
