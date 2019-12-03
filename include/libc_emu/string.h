#if ! defined _STRING_H_
#define _STRING_H_


#ifdef __cplusplus
extern "C" 
{
#endif

	int memcmp( const void *str1, const void *str2, unsigned int len);
	void * memcpy ( void * dst, const void * src, unsigned int len);
	void * memset ( void * dst, unsigned int c, unsigned int len);
	void * memmove( void * dst, const void * src, unsigned int len);
	void * memchr ( const void * s, int c, int len );
	int strlen( const char *src );
	char * strcpy( char *dst, const char *src );
	char * strcat(char * dest, const char * src);
	char * strncat(char * dest, const char * src, int sz);
	char * strncpy( char *dst, const char *src, unsigned long len );
	char * stpcpy( char *dst, const char *src );
	int strcmp( const char *str1, const char *str2 );
	int strncmp( const char *s1, const char *s2, int len );
	char * strrchr( const char *s1, int c );
	char * strchr( const char *s1, int c );
	char * strstr( const char * str, const char * substr);
	char * strpbrk(const char * s, const char * accept);
	char * strtok_r(char *s, const char *delim, char **last);
	char * strtok( char *s, const char *delim );
	int strspn(const char *s, const char * charset);
	int strcspn(const char * s, const char * charset);
	char * strdup( char * str);

#ifdef __cplusplus
}
#endif


#endif