#pragma once

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