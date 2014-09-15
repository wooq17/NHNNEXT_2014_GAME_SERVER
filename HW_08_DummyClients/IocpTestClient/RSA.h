﻿#pragma once

#define KEYLENGTH  0x00800000
#define ENCRYPT_ALGORITHM CALG_RC4 
#define ENCRYPT_BLOCK_SIZE 8 

class RSA
{
public:
	RSA();
	~RSA();

	void ExceptionHandling();
	bool Init();
	PBYTE GetHeader( __out DWORD& headerLen );
	bool Encrypt( PBYTE data, DWORD length );
	bool Decrypt( PBYTE data, DWORD length, __out PBYTE* dataStartPos );

private:
	HCRYPTPROV hCryptProv;
	HCRYPTKEY hEnKey;
	HCRYPTKEY hDeKey;
	HCRYPTKEY hXchgKey;
	HCRYPTHASH hHash;

	PBYTE pbKeyBlob;
	DWORD dwKeyBlobLen;

	PBYTE pbBuffer;
	DWORD dwBlockLen;
	DWORD dwBufferLen;
	DWORD dwCount;

	PBYTE pbHeader;
	DWORD dwHeaderLen;
};

extern RSA* GRSA;

/*
namespace RSA
{
static HCRYPTPROV hCryptProv = NULL;
static HCRYPTKEY hEnKey = NULL;
static HCRYPTKEY hDeKey = NULL;
static HCRYPTKEY hXchgKey = NULL;
static HCRYPTHASH hHash = NULL;

static PBYTE pbKeyBlob = NULL;
static DWORD dwKeyBlobLen;

static PBYTE pbBuffer = NULL;
static DWORD dwBlockLen;
static DWORD dwBufferLen;
static DWORD dwCount;

static PBYTE pbHeader = NULL;
static DWORD dwHeaderLen = 0;

static void ExceptionHandling()
{
//---------------------------------------------------------------
// Free memory.
if ( pbBuffer )
{
free( pbBuffer );
}


//-----------------------------------------------------------
// Release the hash object.
if ( hHash )
{
if ( !( CryptDestroyHash( hHash ) ) )
printf( "[RSA] CryptDestroyHash error : %d\n", GetLastError() );

hHash = NULL;
}

//---------------------------------------------------------------
// Release the session key.
if ( hEnKey )
{
if ( !( CryptDestroyKey( hEnKey ) ) )
printf( "[RSA] CryptDestroyKey error : %d\n", GetLastError() );
}

if ( hDeKey )
{
if ( !( CryptDestroyKey( hDeKey ) ) )
printf( "[RSA] CryptDestroyKey error : %d\n", GetLastError() );
}

//---------------------------------------------------------------
// Release the provider handle.
if ( hCryptProv )
{
if ( !( CryptReleaseContext( hCryptProv, 0 ) ) )
printf( "[RSA] CryptReleaseContext error : %d\n", GetLastError() );
}
}

// for server
// initialize session key
static bool Init()
{
// Get the handle to the default provider.
if ( !CryptAcquireContext( &hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, 0 ) )
{
printf( "[RSA] CryptAcquireContext error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

//-----------------------------------------------------------
// Create a random session key.
if ( !CryptGenKey( hCryptProv, ENCRYPT_ALGORITHM, KEYLENGTH | CRYPT_EXPORTABLE, &hEnKey ) )
{
printf( "[RSA] CryptGenKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

//-----------------------------------------------------------
// Get the handle to the exchange public key.
if ( !CryptGetUserKey( hCryptProv, AT_KEYEXCHANGE, &hXchgKey ) )
{
if ( NTE_NO_KEY == GetLastError() )
{
// No exchange key exists. Try to create one.
if ( !CryptGenKey( hCryptProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &hXchgKey ) )
{
printf( "[RSA] CryptGenKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}
}
else
{
printf( "[RSA] CryptGetUserKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}
}

//-----------------------------------------------------------
// Determine size of the key BLOB, and allocate memory.
if ( !CryptExportKey( hEnKey, hXchgKey, SIMPLEBLOB, 0, NULL, &dwKeyBlobLen ) )
{
printf( "[RSA] CryptExportKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

if ( !( pbKeyBlob = (BYTE *)malloc( dwKeyBlobLen ) ) )
{
printf( "[RSA] out of memory\n" );
ExceptionHandling();
return false;
}

//-----------------------------------------------------------
// Encrypt and export the session key into a simple key BLOB.
if ( !CryptExportKey( hEnKey, hXchgKey, SIMPLEBLOB, 0, pbKeyBlob, &dwKeyBlobLen ) )
{
printf( "[RSA] CryptExportKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

//-----------------------------------------------------------
// Release the key exchange key handle.
if ( hXchgKey )
{
if ( !( CryptDestroyKey( hXchgKey ) ) )
{
printf( "[RSA] CryptDestroyKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

hXchgKey = 0;
}

// 헤더에 들어 갈 것 미리 만들어 두고 재활용
dwHeaderLen = sizeof(DWORD)+dwKeyBlobLen;
if ( !( pbHeader = (BYTE*)malloc( dwHeaderLen ) ) )
return false;

PBYTE currentPos = pbHeader;

memcpy( currentPos, &dwKeyBlobLen, sizeof( DWORD ) );
currentPos += sizeof( DWORD );

memcpy( currentPos, pbKeyBlob, dwKeyBlobLen );
currentPos += dwKeyBlobLen;
}

static PBYTE GetHeader( __out DWORD& headerLen )
{
headerLen = dwHeaderLen;
return pbHeader;
}

// for server
static bool Encrypt( PBYTE data, DWORD length )
{
DWORD len = length;
// Encrypt data.
if ( !CryptEncrypt( hEnKey, NULL, true, 0, data, &len, length ) )
{
printf( "[RSA] CryptEncrypt error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

return true;
}


// for client
static bool Decrypt( PBYTE data, DWORD length, __out PBYTE* dataStartPos )
{
// 공개키를 이용해서 세션 키 만들고
// 데이터를 복호화 해서 Diffie-Hellman 준비한다

//-----------------------------------------------------------
// Decrypt the file with the saved session key.

DWORD dwKeyBlobLen;
PBYTE pbKeyBlob = NULL;
PBYTE currentPos = data;

memcpy( &dwKeyBlobLen, currentPos, sizeof( DWORD ) );
currentPos += sizeof( DWORD );

// Allocate a buffer for the key BLOB.
if ( !( pbKeyBlob = (PBYTE)malloc( dwKeyBlobLen ) ) )
return false;

memcpy( pbKeyBlob, currentPos, dwKeyBlobLen );
currentPos += dwKeyBlobLen;

//-----------------------------------------------------------
// Import the key BLOB into the CSP.
if ( !CryptImportKey( hCryptProv, pbKeyBlob, dwKeyBlobLen, 0, 0, &hDeKey ) )
{
printf( "[RSA] CryptImportKey error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

if ( pbKeyBlob )
{
free( pbKeyBlob );
}

//-----------------------------------------------------------
// Decrypt the block of data.
*dataStartPos = currentPos;
if ( !CryptDecrypt( hDeKey, 0, true, 0, *dataStartPos, &length ) )
{
printf( "[RSA] CryptDecrypt error : %d\n", GetLastError() );
ExceptionHandling();
return false;
}

return true;
}
}
*/