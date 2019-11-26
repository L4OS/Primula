# Primula preprocessor and lexical parser

lexical parser is a programm which preprocess of C files and generates output file in the Primula Raw Lexeme format.

Example of use:
```
	./lexical_parser helloworld.c
```
If no errors were detected then programm will create new file "helloworld.c.lexem". 

Example of generated "helloworld.c.lexem" is shown below:

```
// $ helloworld.c
@1
1017
5 C
605
1 printf
6
1006
601
38
1 format
7
51
8
12
@3
605
1 main
6
8
@4
9
@5
1 printf
6
5 Hello world!\n
8
12
@6
10
```

Each string is a single lexem. Look into ../include/lexem.h to see lexem definitions. 
Also lexical parser can restore source file from Primula Raw Lexem format.
Example of source code restore:
```
	./lexical_parse -REVERSE helloworld.c.lexem
```

This command will restore the source into file "helloworld.c.lexem.cc":

```
extern  "C" int printf (const char *format ,...);
int main ()
{
  printf ( "Hello world!\n" );
}
```

The Primula Raw Lexeme format file is a source for Primula Syntax parser. 

Lines stated with the at symbol define source line number.

Each lexeme defined as a decimal number.

Lexeme number 1 conveys text which can be user defined name, such as variable or function name, name of user defined type and so on.

Lexem number 5 conveys string constants.

