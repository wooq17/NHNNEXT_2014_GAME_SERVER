#include "stdafx.h"
#include "Crypt.h"

#pragma comment(lib, "crypt32.lib")

Crypt::Crypt()
{
}


Crypt::~Crypt()
{
}

void Crypt::SetP( DATA_BLOB p )
{
	m_P.cbData = p.cbData;
	m_P.pbData = p.pbData;
}

void Crypt::SetG( DATA_BLOB g )
{
	m_G.cbData = g.cbData;
	m_G.pbData = g.pbData;
}

void Crypt::GenerateBaseKey()
{
	m_P.cbData = DHKEYSIZE / 8;
	m_P.pbData = (BYTE*)( g_rgbPrime );

	m_G.cbData = DHKEYSIZE / 8;
	m_G.pbData = (BYTE*)( g_rgbGenerator );
}

bool Crypt::GeneratePrivateKey()
{
	// Acquire a provider handle for party 1.
	BOOL fReturn = CryptAcquireContext( &m_ProvParty, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	// Create an ephemeral private key for party 1.
	// 임시 비밀 키 생성 
	fReturn = CryptGenKey( m_ProvParty, CALG_DH_EPHEM, DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN, &m_PrivateKey );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	// Set the prime for party 1's private key.
	// P값 설정 
	fReturn = CryptSetKeyParam( m_PrivateKey, KP_P, (PBYTE)&m_P, 0 );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	// Set the generator for party 1's private key.
	// 비밀 키 생성기 장착
	fReturn = CryptSetKeyParam( m_PrivateKey, KP_G, (PBYTE)&m_G, 0 );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	// Generate the secret values for party 1's private key.
	// 비밀 키 생성 
	fReturn = CryptSetKeyParam( m_PrivateKey, KP_X, NULL, 0 );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	return true;
}

PBYTE Crypt::ExportPublicKey( __out DWORD* dataLen )
{
	// Public key value, (G^X) mod P is calculated.


	// Get the size for the key BLOB.
	BOOL fReturn = CryptExportKey(
		m_PrivateKey,
		NULL,
		PUBLICKEYBLOB,
		0,
		NULL,
		dataLen );
	if ( !fReturn )
		ReleaseResources();

	// Allocate the memory for the key BLOB.
	if ( !( m_KeyBlob = (PBYTE)malloc( *dataLen ) ) )
		ReleaseResources();

	// Get the key BLOB.
	fReturn = CryptExportKey(
		m_PrivateKey,
		0,
		PUBLICKEYBLOB,
		0,
		m_KeyBlob,
		dataLen );
	if ( !fReturn )
		ReleaseResources();

	return m_KeyBlob;
}

bool Crypt::GenerateSessionKey( PBYTE keyBlob, DWORD dataLen )
{
	BOOL fReturn = CryptImportKey(
		m_ProvParty,
		keyBlob,
		dataLen,
		m_PrivateKey,
		0,
		&m_SessionKey );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	/************************
	Convert the agreed keys to symmetric keys. They are currently of
	the form CALG_AGREEDKEY_ANY. Convert them to CALG_RC4.
	************************/
	ALG_ID Algid = CALG_RC4;

	// Enable the party 1 public session key for use by setting the 
	// ALGID.
	fReturn = CryptSetKeyParam(
		m_SessionKey,
		KP_ALGID,
		(PBYTE)&Algid,
		0 );
	if ( !fReturn )
	{
		ReleaseResources();
		return false;
	}

	// printf( "session key : %d\n", m_SessionKey );

	return true;
}


void Crypt::ReleaseResources()
{
	if ( m_SessionKey )
	{
		CryptDestroyKey( m_SessionKey );
		m_SessionKey = NULL;
	}

	if ( m_KeyBlob )
	{
		free( m_KeyBlob );
		m_KeyBlob = NULL;
	}

	if ( m_PrivateKey )
	{
		CryptDestroyKey( m_PrivateKey );
		m_PrivateKey = NULL;
	}

	if ( m_ProvParty )
	{
		CryptReleaseContext( m_ProvParty, 0 );
		m_ProvParty = NULL;
	}
}

bool Crypt::Encrypt( PBYTE data, DWORD length )
{
	// Encrypt the data.
	DWORD dwLength = length;
	BOOL fReturn = CryptEncrypt(
		m_SessionKey,
		0,
		TRUE,
		0,
		data,
		&length,
		length );
	if ( !fReturn )
	{
		printf( "Encrypt error : %d\n", GetLastError() );
		ReleaseResources();
		return false;
	}

	return true;
}

bool Crypt::Decrypt( PBYTE data, DWORD length )
{
	DWORD dwLength = length;
	BOOL fReturn = CryptDecrypt(
		m_SessionKey,
		0,
		TRUE,
		0,
		data,
		&dwLength );
	if ( !fReturn )
	{
		printf( "Decrypt error : %d\n", GetLastError() );
		ReleaseResources();
		return false;
	}

	return true;
}