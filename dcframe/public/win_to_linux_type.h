#ifndef win_to_linux_type_H_
#define  win_to_linux_type_H_
#define INVALID_HANDLE_VALUE   -1
#define _MAX_PATH      260 /* max. length of full pathname */

#define HANDLE         pthread_t
#define MAX_PATH       260
#define TRUE           true
#define FALSE          false
#define __stdcall
#define __declspec(x)
#define __cdecl
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

typedef int                BOOL;
typedef unsigned char      BYTE;
typedef float              FLOAT;
typedef FLOAT              *PFLOAT;
typedef char               CHAR;
typedef unsigned char      UCHAR;
typedef unsigned char      *PUCHAR;
typedef short              SHORT;
typedef unsigned short     USHORT;
typedef unsigned short     *PUSHORT;
typedef unsigned short 	WORD;
typedef long               LONG;
typedef unsigned int       DWORD; //32bit compile dword 4Byte
typedef long long          LONGLONG;
typedef unsigned long long ULONGLONG;
typedef unsigned int       ULONG;   //32bit compile
typedef int                INT;
typedef unsigned int       UINT;
typedef unsigned int       *PUINT;
typedef void               VOID;
typedef unsigned long long uint64;

typedef char               *LPSTR;
typedef const char         *LPCSTR;
typedef wchar_t            WCHAR;
typedef WCHAR              *LPWSTR;
typedef const WCHAR        *LPCWSTR;
typedef DWORD              *LPDWORD;
typedef unsigned long      UINT_PTR;
typedef UINT_PTR           SIZE_T;
typedef LONGLONG           USN;
typedef BYTE               BOOLEAN;
typedef void               *PVOID;


#endif


