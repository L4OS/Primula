#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if defined(_MSC_VER)
# include <io.h>
#else
# include <unistd.h>
# define _access access
# define _snprintf snprintf
#endif

#include "defdef.h"

extern int parse_input( flow_control_t	*	flow );
extern int substitute_def_expression(def_pair_t * def, char * str);

int add_include_to_list(const char * filename); // Returns how many times file was included

//static const char *ipath[] = { "C:/Program Files (x86)/Microsoft Visual Studio 8/VC/include", 0 };
static const char *ipath[] = { "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/include" , 0 };
//static const char *ipath[] = { "C:/Temp/GNU/include.deb", "C:/Temp/GNU/include.deb/linux", "C:/Temp/GNU/include.deb/c++/4.3/tr1", 0 };
//static const char *ipath[] = { "C:/Program Files (x86)/Embarcadero/RAD Studio/7.0/include" , 0 };

////////////////////////////////////////////////////////////////////
typedef enum { 
	ei_val, ei_dig, ei_not, ei_ob, ei_cb, ei_def, ei_eq, ei_ne, 
	ei_gr, ei_le, ei_greq, ei_leeq, ei_and, ei_or, ei_add, ei_sub,
} ex_itemtype_t;

///////////////////////////////////////////////////////////////////////////
// Operation priority
typedef enum {
	TOP_PRIO		=100,
	SELECT_PRIO		=100,
	PREFIX_PRIO		=80,
	UNARY_PRIO		=70,
	MULITIVE_PRIO	=60,
	ADDITIVE_PRIO	=40,
	SHIFT_PRIO		=35,
	COMPARE_PRIO	=30,
	EQUAL_PRIO		=20,
	BITAND_PRIO		=18,
	BITXOR_PRIO		=17,
	BITOR_PRIO		=16,
	LOGAND_PRIO		=14,
	LOGOR_PRIO		=12,
	TERNAR_PRIO		=11,
	POSTFIX_PRIO	=9,
	ASSIGN_PRIO		=7,
	NO_PRIO			=0,
} item_prio_t;


typedef struct ex_item {
	struct ex_item	*	prev;
	struct ex_item	*	next;
	ex_itemtype_t		type;
	int					prio;
	char				val[128];
} ex_item_t;

ex_item_t	*	item_queue_head = 0;
ex_item_t	*	item_queue_tail = 0;

static void appen_ex_item(ex_itemtype_t t, item_prio_t prio, char * v)
{
	ex_item_t	*	i = (ex_item_t*) malloc( sizeof(ex_item_t) );
	i->type = t;
	i->next = 0;
	i->prio = prio;
	if(v)
		strncpy(i->val, v, 127);
	else
		i->val[0] = '\0';

	if( item_queue_head ) {
		i->prev = item_queue_tail;
		item_queue_tail->next = i;
		item_queue_tail = i;
	} else {
		i->prev = 0;
		item_queue_tail = item_queue_head = i;
	}
}

static ex_item_t	*	value_queue_tail = 0;
static ex_item_t	*	value_queue_head = 0;
static ex_item_t	*	op_queue_tail = 0;

// Transfer of lexem between queues
static void move_lexem( ex_item_t * i, bool operation)
{
	// Used for fast remove item from list
	ex_item_t * prev=i->prev, * next = i->next;

	if( ! prev ) {
		if( item_queue_head==i ) {
			item_queue_head=next;
			if( next ) {
				next->prev = 0;
				i->next = 0;
			}
		}
		else throw("List is broken");
	}
	else {
		prev->next = next;
		if( next ) next->prev = prev;
		else {
//			if( next != item_queue_tail ) {
//			throw "List is broken";
			item_queue_tail = i->prev;
		}
	}
	// Parse queue with priority
	if( operation ) {
		int status = op_queue_tail ? op_queue_tail->prio - i->prio : -1;
		if( status < 0 || i->type == ei_ob) {
			if(op_queue_tail)
				op_queue_tail->next = i;
			i->prev = op_queue_tail;
			op_queue_tail = i;
		} else {
			while( status >= 0 ) { 
				prev = op_queue_tail;
				if(!prev) throw "Preprocessor internal error 6";
				op_queue_tail = prev->prev;
				prev->next = 0;
				prev->prev = value_queue_tail;
				value_queue_tail = prev;
				if(!value_queue_head) value_queue_head = prev;
				else prev->prev->next = prev;
				status = op_queue_tail ? op_queue_tail->prio - i->prio : -1;
			}
			i->prev = op_queue_tail;
			op_queue_tail = i;
		}
	}
	else {
		if(value_queue_tail)				
			value_queue_tail->next = i;
		else
			value_queue_head = i;
		i->prev = value_queue_tail;
		value_queue_tail = i;
	}
	i->next = 0;
}

static int  get_value(ex_item_t * i) 
{
	int r;
	if( i->type == ei_val ) {
		def_pair_t * pair = check_for_preprocessor_define(i->val);
		if(pair) {
			r = atoi(pair->payload);
		}
		else {
			r = 0;
		}
	} else {
		r = atoi(i->val);
	}
	return r;
}

static void is_defined()
{
	ex_item_t * i = op_queue_tail;
	if( i->type == ei_val ) {
		def_pair_t * def = check_for_preprocessor_define(i->val);
		i->val[0] = def ? '1' : '0';
		i->val[1] = '\0';
		i->type = ei_dig;
	}
	else throw "Preprocessor syntax error";
}

static void do_operation(ex_item_t * i)
{
	ex_item_t * pv2 = op_queue_tail ? op_queue_tail->next : 0;
	if( ! pv2 ) return;
	int		v1, v2;

	v1 = get_value(op_queue_tail);
	v2 = get_value(pv2);
	switch(i->type) {
		case ei_eq:		v2 = v2 == v1;	break;
		case ei_ne:		v2 = v2 != v1;	break;
		case ei_gr:		v2 = v2 > v1;	break;
		case ei_le:		v2 = v2 < v1;	break;
		case ei_greq:	v2 = v2 >= v1;	break;
		case ei_leeq:	v2 = v2 <= v1;	break;
		case ei_and:	v2 = v2 && v1;	break;
		case ei_or:		v2 = v2 || v1;	break;
		case ei_add:	v2 = v2 + v1;	break;
		case ei_sub:	v2 = v2 - v1;	break;
		default: throw "FIX ME ASAP";
	}
	free(op_queue_tail);
	op_queue_tail = pv2;
	pv2->prev = 0;
	pv2->type = ei_dig;
	_snprintf(pv2->val, sizeof(pv2->val), "%d", v2 );
}

static void do_negate()
{
	int		val;
	if( op_queue_tail ) {
		if( op_queue_tail->type == ei_dig ) {
			val = atoi( op_queue_tail->val );
			op_queue_tail->val[0] = !val ? '1' : '0';
			op_queue_tail->val[1] = 0;
		} else if( op_queue_tail->type == ei_val  ) {
			def_pair_t * pair = check_for_preprocessor_define(op_queue_tail->val);
			if( ! pair ) 
				op_queue_tail->val[0] = '1';
			else if(pair->payload)
				op_queue_tail->val[0] = ! atoi(pair->payload);
			else
				op_queue_tail->val[0] = '0';
			op_queue_tail->val[1] = '\0';
			op_queue_tail->type = ei_dig;
		} else throw "Value type not match";
	}
	else throw "Negate expression absent";
}

static void execute_expression()
{
	ex_item_t * i;
	while(op_queue_tail) {
		i = op_queue_tail;
		op_queue_tail = op_queue_tail->prev;
		i->prev = value_queue_tail;
		value_queue_tail->next = i;
		value_queue_tail = i;
		i->next = 0;
	}

	while( value_queue_head ) {
		i = value_queue_head;
		value_queue_head = i->next;
		switch( i->type ) {
			case ei_val:
			case ei_dig:
				if(op_queue_tail) op_queue_tail->prev = i;
				if(i->next) i->next->prev = 0;
				i->next = op_queue_tail;
				op_queue_tail = i;
				i->prev = 0;
				break;
			case ei_ob: 
			case ei_cb: 
				throw "Brackets already optinized";
			case ei_def: 
				is_defined();
				break;
			case ei_not: 
				do_negate();
				break;
			case ei_eq: 
			case ei_ne: 
			case ei_gr: 
			case ei_le: 
			case ei_greq: 
			case ei_leeq: 
			case ei_and:
			case ei_or:
			case ei_add:
			case ei_sub:
				do_operation(i);
				break;
		default:
				throw "Unparsed preprocessor lexem";
		}
	}
	value_queue_tail = value_queue_head;
}

//#if NATAM == 0
//#error "Yes"
//#endif
static void on_close_braket()
{
	ex_item_t * i;
	
	i=item_queue_head->next;
	free(item_queue_head);
	item_queue_head=i;
	if(!i)
		item_queue_head = item_queue_head;
	else
		i->prev = 0;

	while(op_queue_tail) {
		i = op_queue_tail;
		op_queue_tail=i->prev;
		if( i->type == ei_ob ) return;
		if(value_queue_tail)
			value_queue_tail->next = i;
		i->next = 0;
		i->prev = value_queue_tail;
		value_queue_tail = i;
	}
	throw "No open bracket found in preprocessor exporession";
}

// Parse operator "defined"
static int parse_word_defined()
{
	ex_item_t * d1  = item_queue_head->next;
	ex_item_t * d2;
	ex_item_t * d3;

	if(!d1) 
		return -949;
	switch(d1->type) {
		case ei_val:
			move_lexem( d1, false );
			move_lexem( item_queue_head, true );
			break;

		case ei_ob:
			d2 = d1->next;
			if(!d2) 
				return -949;
			if( d2->type != ei_val ) 
				return -949;
			d3  = d2->next;
			if(!d3) break;
			if( d3->type != ei_cb ) 
				return -949;

			move_lexem( d2, false );
			move_lexem( item_queue_head, true );

			free(d1);
			item_queue_head = d3->next;
			free(d3);
			if(item_queue_head) 
				item_queue_head->prev = 0;
			break;

		default:
			return -949;
	}
	return 0;
}

// Parse preprocessor expressions
static int parse_local_ex()
{
	int status;

	// Look onto queue
	while( item_queue_head ) {
		switch( item_queue_head->type ) {
			case ei_val:
			case ei_dig:
				move_lexem( item_queue_head, false );
				break;
			case ei_cb: 
				on_close_braket();
				break;
			case ei_def: 
				status = parse_word_defined();
				if( status < 0 )
					return status;
				break;
			case ei_ob: 
			case ei_not: 
			case ei_eq: 
			case ei_ne: 
			case ei_gr: 
			case ei_le: 
			case ei_greq: 
			case ei_leeq: 
			case ei_and:
			case ei_or:
			case ei_add:
			case ei_sub:
				move_lexem( item_queue_head, true );
				break;
		default:
				throw "Unparsed preprocessor lexem";
		}
	}

	item_queue_tail = item_queue_head;

	execute_expression();

	if( ! op_queue_tail || value_queue_head /*|| op_queue_tail->*/ ) status = -944; else
	{
		status = atoi( op_queue_tail->val );
		free(op_queue_tail);
		op_queue_tail = 0;
	}

	// Clear expressions queue
	//while( item_queue_head ) {
	//	i = item_queue_head;
	//	item_queue_head = i->next;
	//	free(i);
	//}
	//item_queue_tail = item_queue_head;
	return status;
}


// States of expression's parser
typedef enum fsm_states_t { 
		fsm_clearspaces, fsm_chkdef, fsm_item, fsm_digit, fsm_eq, fsm_not, fsm_great, fsm_less, fsm_and, fsm_or,
		fsm_div, fsm_param, fsm_skip_comment
} local_state_t;

static local_state_t	state = fsm_clearspaces;

// Конечный автомат для разбора выражений препроцессора
static int local_fsm( char c )
{
	int						status;
	static int				i = 0;
	static int				bra_cnt = 0;
	static def_pair_t	*	gdp = 0;
	static char				lbuff[256];

	switch(state) {
		clearspaces:
			state = fsm_clearspaces;
		case fsm_clearspaces:
			if(!c) return 1;
			if( isspace(c) ) break;
			if( isalpha(c) || c == '_') {
				lbuff[i++] = c;
				state = fsm_item;
				break;
			}
			if( isdigit(c) ) {
				lbuff[i++] = c;
				state = fsm_digit;
				break;
			}
			switch(c) {
				case '=':	state = fsm_eq;		break;
				case '!':	state = fsm_not;	break;
				case '>':	state = fsm_great;	break;
				case '<':	state = fsm_less;	break;
				case '&':	state = fsm_and;	break;
				case '|':	state = fsm_or;		break;
				case '/':	state = fsm_div;	break;
				case '(':	appen_ex_item( ei_ob, NO_PRIO, 0 );		break;
				case ')':	appen_ex_item( ei_cb, TOP_PRIO, 0 );	break;
				case '+':	appen_ex_item( ei_add, ADDITIVE_PRIO, 0 );	break;
				case '-':	appen_ex_item( ei_sub, ADDITIVE_PRIO, 0 );	break;
				default:
					printf("TOOOOODO\n");
			}
			break;

		case fsm_item:
			if(isalnum(c) || c == '_') {
				lbuff[i++] = c;
				break;
			}
			lbuff[i] = '\0';
			i=0;
			if( strcmp( lbuff, "defined" ) == 0) {
				appen_ex_item( ei_def, TOP_PRIO, 0 );
				goto chkdef;
			} else {
				def_pair_t * dp = check_for_preprocessor_define( lbuff );
				if(! dp ) {
					appen_ex_item( ei_val, NO_PRIO, lbuff );
				} else if( ! dp->args ) {
					char * payload = dp->payload;
					if (payload == 0)
					{
						appen_ex_item(ei_val, NO_PRIO, lbuff);
						return 1;
					}
					state = fsm_clearspaces;
					do status = local_fsm(*payload++); while( !status ) ;
					if(status > 0) status = (c==0) ? 1 : 0;
					state = fsm_clearspaces;
					return status;
				} else {
					// This is a case of expression with arguments
					// Such counstruction was noticed in Debian headers
					gdp = dp; // Cannot be parsed by this version if nested
					goto param;
				}
			}
			goto clearspaces;
		
		param:
			state = fsm_param;
		case fsm_param:
			if( isspace(c) ) break;
			if( c == '(' ) break;
			if( c == ')' ) {
				lbuff[i] = '\0';
				state = fsm_clearspaces;
				status = substitute_def_expression(gdp, lbuff);
				if(status >= 0) {
					char * payload = lbuff;
					state = fsm_clearspaces;
					do status = local_fsm(*payload++); while( !status ) ;
					if(status > 0) status = (c==0) ? 1 : 0;
				}
				state = fsm_clearspaces;
				return status;
			} else
				lbuff[i++] = c;
			break;

		chkdef:
			state = fsm_chkdef;
		case fsm_chkdef:
			if( isspace(c) ) break;
			if(isalnum(c) || c == '_') {
				lbuff[i++] = c;
				break;
			}
			if( c == '(' ) {
				if( bra_cnt ) 
					return -944;
				bra_cnt++;
				break;
			}
			if( c == ')' ) {
			}
			lbuff[i] = '\0';
			i=0;
			appen_ex_item( ei_val, NO_PRIO, lbuff );
			if( c != ')' ) 
				goto clearspaces;
			if( ! bra_cnt ) 
				goto clearspaces;
			bra_cnt--;
			state = fsm_clearspaces;
			break;

		case fsm_digit:
			if( isdigit(c) ) {
				lbuff[i++] = c;
				break;
			}
			if( c == 'L' ) 
				lbuff[i++] = c;
			lbuff[i] = '\0';
			i=0;
			appen_ex_item( ei_dig, NO_PRIO, lbuff );
			if( c != 'L' )
				goto clearspaces;
			state = fsm_clearspaces;
			break;
			

		case fsm_and:
			if( c != '&' ) return -944;
			appen_ex_item( ei_and, LOGAND_PRIO, 0 );
			state = fsm_clearspaces;
			break;

		case fsm_or:
			if( c != '|' ) return -944;
			appen_ex_item( ei_or, LOGOR_PRIO, 0 );
			state = fsm_clearspaces;
			break;

		case fsm_div:
			if( c == '/' ) {
				state = fsm_skip_comment;
				break;
			}
			throw "TODO: preprocessor opertion divide";
			break;

		case fsm_eq:
			if( c != '=' ) return -944;
			appen_ex_item( ei_eq, EQUAL_PRIO, 0 );
			state = fsm_clearspaces;
			break;

		case fsm_not:
			if( c == '=' ) {
				appen_ex_item( ei_ne, EQUAL_PRIO, 0 );
				state = fsm_clearspaces;
				break;
			}
			appen_ex_item( ei_not, PREFIX_PRIO, 0 );
			goto clearspaces;

		case fsm_great:
			if(c == '=') {
				appen_ex_item( ei_greq, COMPARE_PRIO, 0 );
				state = fsm_clearspaces;
				break;
			}
			appen_ex_item( ei_gr, COMPARE_PRIO, 0 );
			goto clearspaces;

		case fsm_less:
			if(c == '=') {
				appen_ex_item( ei_leeq, COMPARE_PRIO, 0 );
				state = fsm_clearspaces;
				break;
			}
			appen_ex_item( ei_le, COMPARE_PRIO, 0 );
			goto clearspaces;

		case fsm_skip_comment:
			if(!c) {
				state = fsm_clearspaces;
				return 1;
			}
			break;

		default:
			printf("ASAP TOOOOODO \n");

	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//	Разбор директивы #if
//
int parse_preprocessor_if( char * payload  )
{
	int			status = 0;
	bool		invert = false;
	
	state = fsm_clearspaces; // 20180630
	
	do status = local_fsm(*payload++); while( !status ) ;
	if( status > 0 )
		status = parse_local_ex();
	return status;
}

static int check_include_file(
    const char          *   directory,
    flow_control_t		*	subflow,
    char				*	payload )
{
    int j;
    for (j = 0; directory[j] && j < sizeof(subflow->filename) - 56; j++)
        subflow->filename[j] = directory[j];
    subflow->filename[j++] = '/';
    for (char * p = payload + 1; ; p++) 
    {
        if (*p == '\0')
            return -946; // Format error of #include command
        if (*p == '>')	break;
        subflow->filename[j++] = *p;
    }
    subflow->filename[j] = '\0';
    return _access(subflow->filename, 0) == 0 ? 0 : -947;
}

//////////////////////////////////////////////////////////////////////////
//
//	Разбор директивы #include
//
static int parse_include( 
	flow_control_t		*	flow, 
	char				*	payload )
{
	int						status = 0;
	flow_control_t			subflow;
	int						i = 0;

	char			*	ptr = strrchr( flow->filename, '/' );
	if( ! ptr )			ptr = strrchr( flow->filename, '\\' );

	if( *payload == '"') {

		if(ptr) {
			for( ; ptr >= flow->filename + i; i++ )
				subflow.filename[i] = flow->filename[i];
		}
		for( payload++; ; payload++ ) {
			if( *payload == '\0' )	
				return -946;
			if( *payload == '"' )	break;
			subflow.filename[i++] = *payload;
		}
		subflow.filename[i] = '\0';
	} 
	else if( *payload == '<' ) 
	{
		status = -947; // Header file not found

        // Try envionment variable
        char const* c_path = getenv("C_INCLUDE_PATH");
        if (c_path != NULL)
        {
            status = check_include_file(c_path, &subflow, payload);
        }
		for( int i=0; status < 0 && ipath[i]; i++ ) 
        {
            status = check_include_file(ipath[i], &subflow, payload);
		}
		if( status != 0 ) 
			return status;
	} 
	else 
	{	
		printf("#include %s\n", subflow.filename);
		status = -945; // Format error of #include command
		return status;
	}
	if (add_include_to_list(subflow.filename) > 1)
	{
		printf("skip due to already included %s\nCheck this code. It have issue.\n", subflow.filename);
		return 1; // But return positive status
	}
	printf("#include %s\n", subflow.filename);

//if (strstr(subflow.filename, "defdef.h") != NULL)
//	fprintf(stderr, "Debug me!\n");

	subflow.f = fopen( subflow.filename, "rt" );
	if( subflow.f ) {	
		subflow.global_state = gs_expression;
		subflow.line_number = 1;
		subflow.of = flow->of;
		subflow.mode = flow->mode;
		fprintf( flow->of, "// --> $ %s\n", subflow.filename );
//if( strstr(subflow.filename, "stdlib.h") && subflow.line_number==636)
//printf("Leave with shadow count %d\n", 1 /*shadow_count*/ );
		status = parse_input( &subflow );
		fclose(subflow.f);
		fprintf( flow->of, "// <-- $ %s\n", flow->filename );
		if( !status ) status = 1;
	} else 
		status = -193;

	return status;
}

	static int			shadow_count = 0;
	static int			indirection = -1;
	static struct {
		bool	disabled;
		int		count;
	}	shadow_state[16];

	bool IsInativeCode() { return shadow_count != 0; }

//////////////////////////////////////////////////////////////////////////
//
//	Разбор строкие препроцессора
//
int parse_preprocessor_line( flow_control_t	* flow, char * line )
{
	int					status = 0;
	char			*	directive;
	char			*	payload;
	

if( strstr(flow->filename, "libio.h"))
{
if( flow->line_number== 509)
printf("\\\\\\ %s %d\n", flow->filename, flow->line_number );
//if( flow->line_number== 284)
//printf("\\\\\\ %s %d\n", flow->filename, flow->line_number );
}


	for( directive=line; isspace(*directive); directive++);
	payload = strpbrk( directive, " \t" );
	if( payload ) *payload++ = '\0';

	if( strcmp( directive, "include" ) == 0 ) {
		status = shadow_count ? 0 : parse_include( flow, payload );
	} else if( strcmp( directive, "define" ) == 0 ) {
		status = shadow_count ? 0 : parse_definition( payload );
	} else if( strcmp( directive, "if" ) == 0 ) {
		indirection++;
		status = parse_preprocessor_if( payload  );
		if( status == 0 ) {
			shadow_count++;
			shadow_state[indirection].disabled = true;
			shadow_state[indirection].count=1;
		} else {
			shadow_state[indirection].disabled = false;
			shadow_state[indirection].count=0;
		}
	} else if( strcmp( directive, "endif" ) == 0 ) {
		if( payload ) {
			// MSVC headera sometimes have comments after "endif'
			while( isspace(*payload) ) payload++;
			if(*payload != '/' && *payload != '\0' ) 
				throw "some text after #endif directive";
			// Check this point twice. Looks like a logic error if commends beside "endif" have form /* and occupied several strings of text
		}
//		printf("#endif\n");
		shadow_count-=shadow_state[indirection].count;
		indirection--;
		if( ! shadow_count ) 
			status = 1;
	} else if(strcmp( directive, "ifdef" ) == 0 ) {
		indirection++;
		def_pair_t * p = check_for_preprocessor_define( payload );
		if( p!=0) {		
			status  = shadow_count ? 0 : 1;		
			shadow_state[indirection].disabled = false;
			shadow_state[indirection].count=0;
		} else {			
			shadow_count++;
			status = 0;		
			shadow_state[indirection].disabled = true;
			shadow_state[indirection].count=1;
		}
	} else if(strcmp( directive, "ifndef" ) == 0 ) {
		indirection++;
		def_pair_t * p = check_for_preprocessor_define( payload );
		if( p==0) {		
			status  = shadow_count ? 0 : 1;		
			shadow_state[indirection].disabled = false;
			shadow_state[indirection].count=0;
		} else {			
			shadow_count++;
			shadow_state[indirection].disabled = true;
			shadow_state[indirection].count=1;
			status = 0;		
		}
	} else if( strcmp( directive, "else" ) == 0 ) {
		if( shadow_state[indirection].disabled ) {
			shadow_state[indirection].count--;
			--shadow_count;
		}
		else {
			shadow_state[indirection].count++;
			++shadow_count; 
		}
		status  = shadow_count ? 0 : 1;
	} else if( strcmp( directive, "elif" ) == 0 ) {
		if( shadow_state[indirection].disabled ) {
			shadow_state[indirection].count--;
			--shadow_count;
		}
		else {
			shadow_state[indirection].count++;
			++shadow_count; 
		}
		status = parse_preprocessor_if( payload );
		if( status == 0 ) {
			shadow_state[indirection].disabled = true;
			shadow_count++;
			shadow_state[indirection].count++;
		}
		else {
			shadow_state[indirection].disabled = false;
		}
		status  = shadow_count ? 0 : 1;
	} else if( strcmp( directive, "undef" ) == 0 ) {
		status = shadow_count ? 0 : remove_definition( payload );
	} else if( strcmp( directive, "pragma" ) == 0 ) {
		status = shadow_count ? 0 : 1;
	} else if( strcmp( directive, "error" ) == 0 ) {
		if( ! shadow_state ) {
			printf("\n#error %s\n\n", payload );
			status = -941;
		}
	} else {
		printf("unknown prerocessor directive #%s\n", directive );
		status = -940;
	}

	return status;
}


static struct
{
	const char *	filename;
	int				counter;
} list_of_included_files[500];

static int	counter = 0;

int add_include_to_list(const char * filename)
{
	int i;
	for (i=0; i< counter; ++i)
	{
		if (strcmp(filename, list_of_included_files[i].filename) == 0)
			break;
	}
	if (i < counter)
	{
		++list_of_included_files[i].counter;
	}
	else
	{
		char * name = (char*) malloc(strlen(filename) + 1);
		strcpy(name, filename);
		list_of_included_files[counter].filename = name;
		list_of_included_files[counter].counter = 1;
		counter++;
	}

	return list_of_included_files[i].counter;
}
