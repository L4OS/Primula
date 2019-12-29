# Primula syntax parser

SOURCES = \
   syntax.cc config_syntax.cc lexem.cc type_t.cc lexem_tree_root.cc typedef.cc \
   enumeration.cc compound_types.cc namespace.cc function.cc operators.cc \
   expression.cc brace_init.cc ternary.cc template.cc defarglist.cc \
   ../codegen/restore_source_t.cc

TARGET = syntax_parser

all: ${SOURCES}
	g++ ${SOURCES} -o ${TARGET}

clean:
	rm *.o ${TARGET}
