#pragma once

#include "lexem.h"

#include <string>
#include <list>
#include <map>

typedef enum {
	same_types,	// types exactly match
	cast_type,	// type can be casted to l-value type
	no_cast		// type cannot be casted to l-value
} compare_t;

class type_t
{
protected:
	static const int SystemBitSize = 32;
public:
	typedef enum
	{
		void_type		= 0x0000,
		unsigned_type	= 0x0001,
		signed_type		= 0x0002,
		float_type		= 0x0004,
		compound_type	= 0x0008,
		enumerated_type = 0x0010,
		pointer_type	= 0x0100,
		constant_type	= 0x0200,
		dimension_type	= 0x0400,
		template_type	= 0x0800,
		typedef_type	= 0x2000,
		auto_type		= 0x4000,
		funct_ptr_type  = 0x1100
	} property_t;

	int				bitsize;
	property_t		prop;
	std::string		name;

	virtual lexem_type_t GetType() { return lt_empty; }
//	bool isConstant() { return constant; }

	type_t(std::string name, property_t  prop, int bitsize)
	{
		this->name = name;
		this->prop = prop;
		this->bitsize = bitsize;
	}
};

struct linkage_t
{
	enum storage_class_t {
		sc_default,
		sc_extern,
		sc_static,
		sc_register,
		sc_mutable,
		sc_virtual,
		sc_abstract,
		sc_argument // Not sure, maybe default better for this
	} storage_class = sc_default;
    bool inlined;
	std::string	 linkage_def;

    linkage_t()
    {
        inlined = false;
    }
};

typedef enum 
{
    vs_global,
    vs_static,
    vs_argument,
    vs_local
} variable_segment_t;

class pointer_t : public type_t
{
	virtual lexem_type_t GetType() { return lt_pointer; }

public:
	type_t		*	parent_type;
	pointer_t(type_t * type) : type_t("*", pointer_type, SystemBitSize)
	{
		parent_type = type;
	}
};

class address_t : public type_t
{
	virtual lexem_type_t GetType() { return lt_addrres; }
public:
	type_t		*	parent_type;
	address_t(type_t * type) : type_t("&", pointer_type, SystemBitSize)
	{
		parent_type = type;
	}
};

class array_t : public type_t
{
public:
	type_t		*	child_type;
	int				items_count;
	array_t(type_t * type, int dimsize) : type_t("[_]", dimension_type, dimsize * type->bitsize)
	{
		items_count = dimsize;
		child_type = type;
	}
	array_t(type_t * type) : type_t("[]", dimension_type, 32)
	{
		// 32-bits with zero count is pounter to array.
		items_count = 0;
		child_type = type;
	}
};

class const_t : public type_t
{
public:
	type_t		*	parent_type;

	const_t(type_t * type) : type_t("const", constant_type, type->bitsize)
	{
		parent_type = type;
	}
	const_t() : type_t("const", constant_type, 0)
	{
		parent_type = nullptr;
	}
};

class structure_t : public type_t
{
public:
	lexem_type_t			kind;
	class namespace_t	*	space;
	structure_t(lexem_type_t kind, std::string name)
		: type_t(name, compound_type, 0)
	{
		this->space = nullptr;
		this->kind = kind;
	}
};

class template_t : public type_t
{
public:
	class namespace_t	* space;
	template_t(std::string name)
		: type_t(name, template_type, 0)
	{
		this->space = nullptr;
	}
};

class typedef_t : public type_t
{
public:
	type_t * type;
	typedef_t(std::string name, type_t * type)
		: type_t(name, typedef_type, 0)
	{
		this->type = type;
		this->bitsize = type->bitsize;
	}
};

class enumeration_t :public type_t
{
public:
	std::map<std::string, int>	enumeration;
	enumeration_t(std::string name)
		: type_t(name, enumerated_type, SystemBitSize)
	{
	}
};

typedef struct static_data
{
	lexem_type_t	type;
	union {
		int									s_int;;
		unsigned							u_int;
		float								float_const;
		unsigned char						u_char;
		signed char							s_char;
		char							*	p_char;
		std::list<struct static_data*>	*	nested;
	};

    static_data(int v) : type(lt_integer) { s_int = v; }
	static_data(char * v) : type(lt_string) { p_char = v; }
	static_data(const char * v) : type(lt_string) { p_char = (char*)v; }

    static_data()
    {
        type = lt_empty;
        s_int = 0;
    }

    static_data(lexem_type_t t) : type(t)
	{
		p_char = nullptr;
		nested = new std::list<struct static_data*>();
	}
} static_data_t;

compare_t  CompareTypes(const type_t * ltype, const type_t * rtype, bool arg_mode, bool zero_rval);


