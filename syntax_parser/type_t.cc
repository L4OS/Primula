#include "type_t.h"
#include "namespace.h"

compare_t  CompareTypes(const type_t * ltype, const type_t * rtype, bool arg_mode)
{
	bool lconst = false;
	bool rconst = false;

	if (ltype == rtype)
		return same_types;

	while (ltype->prop == type_t::constant_type)
	{
		lconst = true;
		ltype = ((const_t*)ltype)->parent_type;
	}

	while (rtype->prop == type_t::constant_type)
	{
		rconst = true;
		rtype = ((const_t*)rtype)->parent_type;
	}

	if (rtype->prop == type_t::typedef_type)
		rtype = ((typedef_t*)rtype)->type;

	if (ltype->prop == type_t::typedef_type)
		ltype = ((typedef_t*)ltype)->type;

	if (ltype->prop == rtype->prop)
	{
		if (lconst == true && rconst == false)
			return arg_mode ? cast_type : no_cast; // Due to const

		switch (ltype->prop)
		{
		case type_t::pointer_type:
			return CompareTypes(((pointer_t*)ltype)->parent_type, ((pointer_t*)rtype)->parent_type, arg_mode);

		case type_t::compound_type:
		case type_t::enumerated_type:
			return CompareTypes(ltype, rtype, arg_mode);

		default:
			if (ltype->bitsize == rtype->bitsize)
				return same_types;
			if (ltype->bitsize > rtype->bitsize)
				return cast_type;

			throw "TODO Compare same propt types";
			break;
		}
	}
	if (ltype->prop == type_t::signed_type)
	{
		return (rtype->prop == type_t::unsigned_type) ? cast_type : no_cast;
	}
	if (ltype->prop == type_t::unsigned_type)
	{
		return no_cast;
	}
	if (ltype->prop == type_t::float_type)
	{
		if (rtype->prop == type_t::unsigned_type || rtype->prop == type_t::signed_type)
			return cast_type;
		return no_cast;
	}
	throw "TODO Finish type comparasiion";
}

