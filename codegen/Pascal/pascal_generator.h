#pragma once
#include "../../syntax_parser/namespace.h"
class PascalGenerator :
	public namespace_t
{

    void GenerateExpression(expression_t * exp);
    void GenerateStatement(statement_t * code);
    void GenerateCall(call_t * call);
    void GenerateItem(expression_node_t * n, int parent_priority = 1000);
    void GenerateTypeName(type_t * type);
    void GenerateArgumentDefinition(variable_base_t * arg);
    void GenerateFunctionOverload(function_overload_t * overload, bool proto);
    void GenerateType(type_t * type, bool inlined_only);
    void GenerateFunction(function_parser * f, bool proto);
    void GenerateStaticData(static_data_t * data, bool last, bool selfformat = false);

    void GenerateTypes();
    void GenerateSpaceVariables();
    void GeneratePrototypes();
    void GenerateSpaceFunctions();
    void GenerateSpaceCode();

    // Output
    static void MoveIndentRight();
    static void MoveIndentLeft();
    static void Write(const char * s, ...);
    static void IndentWrite(const char *s, ...);
    static void PrintOpenBlock();
    static void PrintCloseBlock();

public:
	void GenerateCode(namespace_t * space);
    void GenerateSpace();

	PascalGenerator();
	virtual ~PascalGenerator();
};

