/*
	stdio.h emulation
*/

#if ! defined _STDIO_H_
#define _STDIO_H_


#ifndef NULL
#define NULL	0
#endif

struct FILE_TAG
{
	void * something;
};

#define FILE struct FILE_TAG

FILE * stdin = 0;
FILE * stdout = 0;
FILE * stderr = 0;

extern "C" int fputc ( int character, FILE * stream );
extern "C" int fputs ( const char * str, FILE * stream );
extern "C" int snprintf ( char * s, int n, const char * format, ... );
extern "C" char * fgets ( char * str, int num, FILE * stream );
extern "C" int sscanf ( const char * s, const char * format, ...);
extern "C" int fprintf ( FILE * stream, const char * format, ... );
extern "C" int printf( const char * format, ... );

#endif