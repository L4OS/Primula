#ifndef _STDLIB_H_
#define _STDLIB_H_

extern "C" int atoi(const char * str);
extern "C" long atol(const char * str);
extern "C" long strtol(const char *p, char **out_p, int base);

	// Memory allocation functions
extern "C" void *calloc(int nmemb, int size);
extern "C" void *malloc(int size);
extern "C" void free(void *ptr);
extern "C" void *realloc(void *ptr, int size);

extern "C" int setenv(const char * name, const char * value, int overwrite);
extern "C" int unsetenv(const char * name);
extern "C" char * getenv(const char * name);
extern "C" int putenv(char * string);

#endif
