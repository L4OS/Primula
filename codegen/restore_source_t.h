#pragma once
#include "../syntax_parser/namespace.h"
class restore_source_t :
	public namespace_t
{
    void GenerateType(type_t * type, bool inlined_only);
    void GenerateFunctionPrototypes(bool is_methods);
    void GenerateSpaceTypes();
    void GenerateSpaceCode();
    void GenerateSpaceFunctions(bool is_methods);
    void GenerateStatement(statement_t * code);
    void GenerateFunctionOverload(function_overload_t * overload, bool proto);
    void GenerateFunction(function_parser * f, bool is_methos, bool proto);

    void GenerateEmbeddedSpaces();
    void GenerateSpace();

public:
    static bool     opt_HideUnusedPrototypes;

    void GenerateCode(namespace_t * space);

	restore_source_t();
	virtual ~restore_source_t();
};

