#pragma once
// Encrypting_a_File.cpp : Defines the entry point for the console 
// application.
//

// Link with the Advapi32.lib file.
#pragma comment (lib, "advapi32")

#define KEYLENGTH  0x00800000
#define ENCRYPT_ALGORITHM CALG_RC4 
#define ENCRYPT_BLOCK_SIZE 8 

#define PUBLIC_PASSWORD		L"invalid password"

namespace RSA
{
	static wchar_t* gPublic_password = PUBLIC_PASSWORD;

	static HCRYPTPROV hCryptProv = NULL;
	static HCRYPTKEY hKey = NULL;
	static HCRYPTKEY hXchgKey = NULL;
	static HCRYPTHASH hHash = NULL;

	static PBYTE pbKeyBlob = NULL;
	static DWORD dwKeyBlobLen;

	static PBYTE pbBuffer = NULL;
	static DWORD dwBlockLen;
	static DWORD dwBufferLen;
	static DWORD dwCount;

	// for server
	// initialize session key
	static bool Init()
	{
		// Get the handle to the default provider. 
		// RSA 방식의 암호화 provider 생성한다
		if ( !CryptAcquireContext(&hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, 0 ) )
		{
			printf( "[RSA] CryptAcquireContext error : %d\n", GetLastError() );
			ExceptionHandling();
			return false;
		}

		// Encrypt the file with a random session key, and write the key to a file. 

		//-----------------------------------------------------------
		// Create a random session key. 
		if ( !CryptGenKey( hCryptProv, ENCRYPT_ALGORITHM, KEYLENGTH | CRYPT_EXPORTABLE, &hKey ) )
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
		if ( CryptExportKey( hKey, hXchgKey, SIMPLEBLOB, 0, NULL, &dwKeyBlobLen ) )
		{
			printf( "[RSA] CryptExportKey error : %d\n", GetLastError() );
			ExceptionHandling();
			return false;
		}

		if ( pbKeyBlob = (BYTE *)malloc( dwKeyBlobLen ) )
		{
			printf( "[RSA] out of memory\n" );
			ExceptionHandling();
			return false;
		}

		//-----------------------------------------------------------
		// Encrypt and export the session key into a simple key BLOB. 
		if ( CryptExportKey( hKey, hXchgKey, SIMPLEBLOB, 0, pbKeyBlob, &dwKeyBlobLen ) )
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

		//-----------------------------------------------------------
		// Write the size of the key BLOB to the destination file. 
		if ( !WriteFile( hDestinationFile, &dwKeyBlobLen, sizeof( DWORD ), &dwCount, NULL ) )
		{
			if ( !( CryptDestroyKey( hXchgKey ) ) )
			{
				printf( "[RSA] WriteFile error : %d\n", GetLastError() );
				ExceptionHandling();
				return false;
			}
		}

		//-----------------------------------------------------------
		// Write the key BLOB to the destination file. 
		if ( !WriteFile(
			hDestinationFile,
			pbKeyBlob,
			dwKeyBlobLen,
			&dwCount,
			NULL ) )
		{
			MyHandleError(
				TEXT( "Error writing header.\n" ),
				GetLastError() );
			goto Exit_MyEncryptFile;
		}
		else
		{
			_tprintf(
				TEXT( "The key BLOB has been written to the " )
				TEXT( "file. \n" ) );
		}

		// Free memory.
		free( pbKeyBlob );
	}

	// for server
	static bool Encrypt( PBYTE data, DWORD length )
	{

		return true;
	}


	// for client
	static bool Decrypt( PBYTE data, DWORD length )
	{
		//-----------------------------------------------------------
		// Decrypt the file with the saved session key. 

		DWORD dwKeyBlobLen;
		PBYTE pbKeyBlob = NULL;

		// Read the key BLOB length from the source file. 
		if ( !ReadFile(
			hSourceFile,
			&dwKeyBlobLen,
			sizeof( DWORD ),
			&dwCount,
			NULL ) )
		{
			MyHandleError(
				TEXT( "Error reading key BLOB length!\n" ),
				GetLastError() );
			goto Exit_MyDecryptFile;
		}

		// Allocate a buffer for the key BLOB.
		if ( !( pbKeyBlob = (PBYTE)malloc( dwKeyBlobLen ) ) )
		{
			MyHandleError(
				TEXT( "Memory allocation error.\n" ),
				E_OUTOFMEMORY );
		}

		//-----------------------------------------------------------
		// Read the key BLOB from the source file. 
		if ( !ReadFile(
			hSourceFile,
			pbKeyBlob,
			dwKeyBlobLen,
			&dwCount,
			NULL ) )
		{
			MyHandleError(
				TEXT( "Error reading key BLOB length!\n" ),
				GetLastError() );
			goto Exit_MyDecryptFile;
		}

		//-----------------------------------------------------------
		// Import the key BLOB into the CSP. 
		if ( !CryptImportKey(
			hCryptProv,
			pbKeyBlob,
			dwKeyBlobLen,
			0,
			0,
			&hKey ) )
		{
			MyHandleError(
				TEXT( "Error during CryptImportKey!/n" ),
				GetLastError() );
			goto Exit_MyDecryptFile;
		}

		if ( pbKeyBlob )
		{
			free( pbKeyBlob );
		}

		//---------------------------------------------------------------
		// The decryption key is now available, either having been 
		// imported from a BLOB read in from the source file or having 
		// been created by using the password. This point in the program 
		// is not reached if the decryption key is not available.

		//---------------------------------------------------------------
		// Determine the number of bytes to decrypt at a time. 
		// This must be a multiple of ENCRYPT_BLOCK_SIZE. 

		dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE;
		dwBufferLen = dwBlockLen;

		//---------------------------------------------------------------
		// Allocate memory for the file read buffer. 
		if ( !( pbBuffer = (PBYTE)malloc( dwBufferLen ) ) )
		{
			MyHandleError( TEXT( "Out of memory!\n" ), E_OUTOFMEMORY );
			goto Exit_MyDecryptFile;
		}

		//---------------------------------------------------------------
		// Decrypt the source file, and write to the destination file. 
		bool fEOF = false;
		do
		{
			//-----------------------------------------------------------
			// Read up to dwBlockLen bytes from the source file. 
			if ( !ReadFile(
				hSourceFile,
				pbBuffer,
				dwBlockLen,
				&dwCount,
				NULL ) )
			{
				MyHandleError(
					TEXT( "Error reading from source file!\n" ),
					GetLastError() );
				goto Exit_MyDecryptFile;
			}

			if ( dwCount <= dwBlockLen )
			{
				fEOF = TRUE;
			}

			//-----------------------------------------------------------
			// Decrypt the block of data. 
			if ( !CryptDecrypt(
				hKey,
				0,
				fEOF,
				0,
				pbBuffer,
				&dwCount ) )
			{
				MyHandleError(
					TEXT( "Error during CryptDecrypt!\n" ),
					GetLastError() );
				goto Exit_MyDecryptFile;
			}

			//-----------------------------------------------------------
			// Write the decrypted data to the destination file. 
			if ( !WriteFile(
				hDestinationFile,
				pbBuffer,
				dwCount,
				&dwCount,
				NULL ) )
			{
				MyHandleError(
					TEXT( "Error writing ciphertext.\n" ),
					GetLastError() );
				goto Exit_MyDecryptFile;
			}

			//-----------------------------------------------------------
			// End the do loop when the last block of the source file 
			// has been read, encrypted, and written to the destination 
			// file.
		} while ( !fEOF );

		return true;
	}

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
			{
				MyHandleError(
					TEXT( "Error during CryptDestroyHash.\n" ),
					GetLastError() );
			}

			hHash = NULL;
		}

		//---------------------------------------------------------------
		// Release the session key. 
		if ( hKey )
		{
			if ( !( CryptDestroyKey( hKey ) ) )
			{
				MyHandleError(
					TEXT( "Error during CryptDestroyKey!\n" ),
					GetLastError() );
			}
		}

		//---------------------------------------------------------------
		// Release the provider handle. 
		if ( hCryptProv )
		{
			if ( !( CryptReleaseContext( hCryptProv, 0 ) ) )
			{
				MyHandleError(
					TEXT( "Error during CryptReleaseContext!\n" ),
					GetLastError() );
			}
		}

		return fReturn;
	} // End Encryptfile.
	}
}


















































bool MyEncryptFile(
	LPTSTR szSource,
	LPTSTR szDestination,
	LPTSTR szPassword );

void MyHandleError(
	LPTSTR psz,
	int nErrorNumber );

int _tmain( int argc, _TCHAR* argv[] )
{
	if ( argc < 3 )
	{
		_tprintf( TEXT( "Usage: <example.exe> <source file> " )
			TEXT( "<destination file> | <password>\n" ) );
		_tprintf( TEXT( "<password> is optional.\n" ) );
		_tprintf( TEXT( "Press any key to exit." ) );
		_gettch();
		return 1;
	}

	LPTSTR pszSource = argv[1];
	LPTSTR pszDestination = argv[2];
	LPTSTR pszPassword = NULL;

	if ( argc >= 4 )
	{
		pszPassword = argv[3];
	}

	//---------------------------------------------------------------
	// Call EncryptFile to do the actual encryption.
	if ( MyEncryptFile( pszSource, pszDestination, pszPassword ) )
	{
		_tprintf(
			TEXT( "Encryption of the file %s was successful. \n" ),
			pszSource );
		_tprintf(
			TEXT( "The encrypted data is in file %s.\n" ),
			pszDestination );
	}
	else
	{
		MyHandleError(
			TEXT( "Error encrypting file!\n" ),
			GetLastError() );
	}

	return 0;
}

//-------------------------------------------------------------------
// Code for the function MyEncryptFile called by main.
//-------------------------------------------------------------------
// Parameters passed are:
//  pszSource, the name of the input, a plaintext file.
//  pszDestination, the name of the output, an encrypted file to be 
//   created.
//  pszPassword, either NULL if a password is not to be used or the 
//   string that is the password.
bool MyEncryptFile(
	LPTSTR pszSourceFile,
	LPTSTR pszDestinationFile,
	LPTSTR pszPassword )
{
	//---------------------------------------------------------------
	// Declare and initialize local variables.
	bool fReturn = false;
	HANDLE hSourceFile = INVALID_HANDLE_VALUE;
	HANDLE hDestinationFile = INVALID_HANDLE_VALUE;

	HCRYPTPROV hCryptProv = NULL;
	HCRYPTKEY hKey = NULL;
	HCRYPTKEY hXchgKey = NULL;
	HCRYPTHASH hHash = NULL;

	PBYTE pbKeyBlob = NULL;
	DWORD dwKeyBlobLen;

	PBYTE pbBuffer = NULL;
	DWORD dwBlockLen;
	DWORD dwBufferLen;
	DWORD dwCount;

	//---------------------------------------------------------------
	// Open the source file. 
	hSourceFile = CreateFile(
		pszSourceFile,
		FILE_READ_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	if ( INVALID_HANDLE_VALUE != hSourceFile )
	{
		_tprintf(
			TEXT( "The source plaintext file, %s, is open. \n" ),
			pszSourceFile );
	}
	else
	{
		MyHandleError(
			TEXT( "Error opening source plaintext file!\n" ),
			GetLastError() );
		goto Exit_MyEncryptFile;
	}

	//---------------------------------------------------------------
	// Open the destination file. 
	hDestinationFile = CreateFile(
		pszDestinationFile,
		FILE_WRITE_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	if ( INVALID_HANDLE_VALUE != hDestinationFile )
	{
		_tprintf(
			TEXT( "The destination file, %s, is open. \n" ),
			pszDestinationFile );
	}
	else
	{
		MyHandleError(
			TEXT( "Error opening destination file!\n" ),
			GetLastError() );
		goto Exit_MyEncryptFile;
	}

	



	//---------------------------------------------------------------
	// Create the session key.
	//-----------------------------------------------------------
	// The file will be encrypted with a session key derived 
	// from a password.
	// The session key will be recreated when the file is 
	// decrypted only if the password used to create the key is 
	// available. 
	// 주어진 비밀번호에 의해서 생성되는 세션키를 만든다
	// 이 세션키로 암호화 된 데이터는 해당 비밀번호로 풀어야 제대로 복호화 가능

	//-----------------------------------------------------------
	// Create a hash object. 
	if ( CryptCreateHash(
		hCryptProv,
		CALG_MD5,
		0,
		0,
		&hHash ) )
	{
		_tprintf( TEXT( "A hash object has been created. \n" ) );
	}
	else
	{
		MyHandleError(
			TEXT( "Error during CryptCreateHash!\n" ),
			GetLastError() );
		goto Exit_MyEncryptFile;
	}

	//-----------------------------------------------------------
	// Hash the password. 
	if ( CryptHashData(
		hHash,
		(BYTE *)gPublic_password,
		lstrlen( gPublic_password ),
		0 ) )
	{
		_tprintf(
			TEXT( "The password has been added to the hash. \n" ) );
	}
	else
	{
		MyHandleError(
			TEXT( "Error during CryptHashData. \n" ),
			GetLastError() );
		goto Exit_MyEncryptFile;
	}

	//-----------------------------------------------------------
	// Derive a session key from the hash object. 
	if ( CryptDeriveKey(
		hCryptProv,
		ENCRYPT_ALGORITHM,
		hHash,
		KEYLENGTH,
		&hKey ) )
	{
		_tprintf(
			TEXT( "An encryption key is derived from the " )
			TEXT( "password hash. \n" ) );
	}
	else
	{
		MyHandleError(
			TEXT( "Error during CryptDeriveKey!\n" ),
			GetLastError() );
		goto Exit_MyEncryptFile;
	}

	//---------------------------------------------------------------
	// The session key is now ready. If it is not a key derived from 
	// a  password, the session key encrypted with the private key 
	// has been written to the destination file.

	//---------------------------------------------------------------
	// Determine the number of bytes to encrypt at a time. 
	// This must be a multiple of ENCRYPT_BLOCK_SIZE.
	// ENCRYPT_BLOCK_SIZE is set by a #define statement.
	dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE;

	//---------------------------------------------------------------
	// Determine the block size. If a block cipher is used, 
	// it must have room for an extra block. 
	if ( ENCRYPT_BLOCK_SIZE > 1 )
	{
		dwBufferLen = dwBlockLen + ENCRYPT_BLOCK_SIZE;
	}
	else
	{
		dwBufferLen = dwBlockLen;
	}

	//---------------------------------------------------------------
	// Allocate memory. 
	if ( pbBuffer = (BYTE *)malloc( dwBufferLen ) )
	{
		_tprintf(
			TEXT( "Memory has been allocated for the buffer. \n" ) );
	}
	else
	{
		MyHandleError( TEXT( "Out of memory. \n" ), E_OUTOFMEMORY );
		goto Exit_MyEncryptFile;
	}

	//---------------------------------------------------------------
	// In a do loop, encrypt the source file, 
	// and write to the source file. 
	bool fEOF = FALSE;
	do
	{
		//-----------------------------------------------------------
		// Read up to dwBlockLen bytes from the source file. 
		if ( !ReadFile(
			hSourceFile,
			pbBuffer,
			dwBlockLen,
			&dwCount,
			NULL ) )
		{
			MyHandleError(
				TEXT( "Error reading plaintext!\n" ),
				GetLastError() );
			goto Exit_MyEncryptFile;
		}

		if ( dwCount < dwBlockLen )
		{
			fEOF = TRUE;
		}

		//-----------------------------------------------------------
		// Encrypt data. 
		if ( !CryptEncrypt(
			hKey,
			NULL,
			fEOF,
			0,
			pbBuffer,
			&dwCount,
			dwBufferLen ) )
		{
			MyHandleError(
				TEXT( "Error during CryptEncrypt. \n" ),
				GetLastError() );
			goto Exit_MyEncryptFile;
		}

		//-----------------------------------------------------------
		// Write the encrypted data to the destination file. 
		if ( !WriteFile(
			hDestinationFile,
			pbBuffer,
			dwCount,
			&dwCount,
			NULL ) )
		{
			MyHandleError(
				TEXT( "Error writing ciphertext.\n" ),
				GetLastError() );
			goto Exit_MyEncryptFile;
		}

		//-----------------------------------------------------------
		// End the do loop when the last block of the source file 
		// has been read, encrypted, and written to the destination 
		// file.
	} while ( !fEOF );

	fReturn = true;




Exit_MyEncryptFile:
	//---------------------------------------------------------------
	// Close files.
	if ( hSourceFile )
	{
		CloseHandle( hSourceFile );
	}

	if ( hDestinationFile )
	{
		CloseHandle( hDestinationFile );
	}

	


//-------------------------------------------------------------------
//  This example uses the function MyHandleError, a simple error
//  handling function, to print an error message to the  
//  standard error (stderr) file and exit the program. 
//  For most applications, replace this function with one 
//  that does more extensive error reporting.

void MyHandleError( LPTSTR psz, int nErrorNumber )
{
	_ftprintf( stderr, TEXT( "An error occurred in the program. \n" ) );
	_ftprintf( stderr, TEXT( "%s\n" ), psz );
	_ftprintf( stderr, TEXT( "Error number %x.\n" ), nErrorNumber );
}
