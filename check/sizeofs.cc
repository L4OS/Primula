extern "C" int printf(const char * format, ...);

int main()
{
    printf("sizeof(char) = %d\n", sizeof(char) );
    printf("sizeof(short) = %d\n", sizeof(short) );
    printf("sizeof(int) = %d\n", sizeof(int) );
    printf("sizeof(long) = %d\n", sizeof(long) );
    printf("sizeof(float) = %d\n", sizeof(float) );
    
    printf("sizeof(void*) = %d\n", sizeof(void*) );
    printf("sizeof(int*) = %d\n", sizeof(int*) );
    return 0;
}
