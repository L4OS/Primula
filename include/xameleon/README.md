# libc headers emulation

Real libc headers are platform specific. So we keep here description of some subset of libc functions. At least some of them, which used by the Primula itself.

To make this folder the default include directory for Primula preprocessor and lexical analyzer, define an environment variable C_INCLUDE_PATH with absolute path to this folder.

These includes are based on includes of [The Xameleon Project](http://l4os.ru/).
