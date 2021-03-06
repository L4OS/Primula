#include "../include/type_t.h"
#include "namespace.h"

compare_t  CompareTypes(
	const type_t *	ltype,			// Type of l-value
	const type_t *	rtype,			// Type of r-value
	bool			arg_mode,		// Ignore const for l-vavlue
	bool			zero_rvalue)	// Zero can be assigned to pointers too
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
		if (!arg_mode && lconst == true && rconst == false)
			return no_cast; // Due to const

		switch (ltype->prop)
		{
		case type_t::pointer_type:
        {
            if (zero_rvalue)
            {
                return same_types; // Maybe cast_type here for zero pointer?
            }
            type_t * prt = ((pointer_t*)ltype)->parent_type;
            if (prt->prop == type_t::void_type)
                return cast_type;
            return CompareTypes(prt, ((pointer_t*)rtype)->parent_type, true, false);
        }

		case type_t::compound_type:
		case type_t::enumerated_type:
			return CompareTypes(ltype, rtype, arg_mode, false);

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
        return (rtype->prop == type_t::signed_type) ? cast_type : no_cast;
	}
	if (ltype->prop == type_t::float_type || ltype->prop == type_t::enumerated_type)
	{
		if (rtype->prop == type_t::unsigned_type || rtype->prop == type_t::signed_type)
			return cast_type;
		return no_cast;
	}
	if (ltype->prop == type_t::pointer_type)
	{
        type_t * ptr_to = ((pointer_t*)ltype)->parent_type;
        if(zero_rvalue)
		    return cast_type; // Maybe same_types?
        if (rtype->prop == type_t::dimension_type)
        {
            if (ptr_to->prop == type_t::void_type)
                return same_types;
            return CompareTypes(ptr_to, ((array_t*)rtype)->child_type, true, false);
        }
        return no_cast;
	}
    if (ltype->prop == type_t::void_type)
        return no_cast;
	throw "TODO Finish type comparasion";
}

