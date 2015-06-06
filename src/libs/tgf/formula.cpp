/***************************************************************************

    file                 : formula.cpp
    copyright            : (C) 2009 by Mart Kelder
    web                  : http://speed-dreams.sourceforge.net   
    web                  : speed-dreams.sourceforge.net
    version              : $Id: formula.cpp 5353 2013-03-24 10:26:22Z pouillot $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		Mathematical formula interpreter
*/

#include <cmath>

#include <portability.h>

#include "tgf.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define PSTYPE_DOUBLE 1
#define PSTYPE_COMMAND 2
#define PSTYPE_BOOL 3

#define FORMNODE_TYPE_NUMBER 1
#define FORMNODE_TYPE_STRING 2
#define FORMNODE_TYPE_VARIABLE 3
#define FORMNODE_TYPE_FUNCTION 4
#define FORMNODE_TYPE_TOPARSE_STRING 64
#define FORMNODE_TYPE_TOPARSE_BLOCK 128

#define FORMANSWER_TYPE_BOOLEAN 0x01
#define FORMANSWER_TYPE_INTEGER 0x02
#define FORMANSWER_TYPE_NUMBER  0x04
#define FORMANSWER_TYPE_STRING  0x08

typedef struct FormAnswer
{
	int validFields;
	
	bool boolean;
	int integer;
	tdble number;
	char *string;
} tFormAnswer;

typedef struct FormNode tFormNode;

typedef tFormAnswer (*tEvalFunc)( tFormNode* node, void *parmHandle, char const* path );

typedef struct FormNode
{
	struct FormNode *firstChild;
	struct FormNode *next;
	int type;
	tdble number;
	char *string;
	tEvalFunc func;
} tFormNode;

typedef struct Formula
{
	tFormNode *node;
	tFormAnswer lastAnswer;
} tFormula;


typedef struct FuncList
{
	int arity;
	char hasArgBeforeFuncName;
	char funcName[ 15 ];
	tEvalFunc func;
} tFuncList;

typedef struct FuncBindList
{
	int length;
	tFuncList list[ 10 ];
} tFuncBindList;

static tFormAnswer func_if( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_sqrt( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_log( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_toString( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_toAlpha( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_op_add_cat( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_op_sub_not( tFormNode *node, void *parmHandle, char const *path );
static tFormAnswer func_op_mul( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_div( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_max_or( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_min_and( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_lt( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_le( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_eq( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_ne( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_ge( tFormNode *node, void* parmHandle, char const *path );
static tFormAnswer func_op_gt( tFormNode *node, void* parmHandle, char const *path );

static tFuncList f_if       = { 3, FALSE, "if",         &func_if };
static tFuncList f_max      = { 2, FALSE, "max",        &func_max_or };
static tFuncList f_min      = { 2, FALSE, "min",        &func_max_or };
static tFuncList f_sqrt     = { 1, FALSE, "sqrt",       &func_sqrt };
static tFuncList f_log      = { 1, FALSE, "log",        &func_log };
static tFuncList f_toString = { 1, FALSE, "toString",   &func_toString };
static tFuncList f_toAlpha1 = { 1, FALSE, "toAlpha",    &func_toAlpha };
static tFuncList f_toAlpha2 = { 2, FALSE, "toAlphaMax", &func_toAlpha };
static tFuncList op_add     = { 2, TRUE,  "+",          &func_op_add_cat };
static tFuncList op_cat     = { 2, TRUE,  "cat",        &func_op_add_cat };
static tFuncList op_sub     = { 2, TRUE,  "-",          &func_op_sub_not };
static tFuncList op_mul     = { 2, TRUE,  "*",          &func_op_mul };
static tFuncList op_div     = { 2, TRUE,  "/",          &func_op_div };
static tFuncList op_and1    = { 2, TRUE,  "and",        &func_max_or };
static tFuncList op_and2    = { 2, TRUE,  "/\\",        &func_max_or };
static tFuncList op_or1     = { 2, TRUE,  "or",         &func_min_and };
static tFuncList op_or2     = { 2, TRUE,  "\\/",        &func_min_and };
static tFuncList op_lt      = { 2, TRUE,  "lt",         &func_op_lt };
static tFuncList op_le      = { 2, TRUE,  "le",         &func_op_le };
static tFuncList op_eq      = { 2, TRUE,  "eq",         &func_op_eq };
static tFuncList op_ne      = { 2, TRUE,  "ne",         &func_op_ne };
static tFuncList op_ge      = { 2, TRUE,  "ge",         &func_op_ge };
static tFuncList op_gt      = { 2, TRUE,  "gt",         &func_op_gt };

static tFuncBindList func1       = { 5, { f_if, f_max, f_min, f_sqrt, f_log } };
static tFuncBindList func2       = { 3, { f_toString, f_toAlpha1, f_toAlpha2 } };
static tFuncBindList op_muldiv   = { 4, { op_mul, op_and1, op_and2, op_div } };
static tFuncBindList op_plusmin  = { 5, { op_add, op_or1, op_or2, op_cat, op_sub } };
static tFuncBindList op_compare  = { 6, { op_lt, op_le, op_eq, op_ne, op_ge, op_gt } };
static tFuncBindList op_and = { 2, { op_and1, op_and2 } };
static tFuncBindList op_or  = { 2, { op_or1, op_or2 } };

static tFuncBindList funclist[] = { func1, func2, op_muldiv, op_plusmin, op_compare, op_and, op_or };
static int funclistlength = 6;

struct PSStackItem;
typedef char (*tExecFunc)( struct PSStackItem **topStack, void* userData, const char* path );

typedef struct PSCommand {
	tExecFunc func;
	void * data;
	struct PSCommand *next;
} tPSCommand;

typedef struct PSStackItem {
	int type;
	union  {
		double doublefloat;
		char boolean;
		tPSCommand *command;
	} d;
	void *varList;
	struct PSStackItem *prev;
} tPSStackItem;

static tPSCommand* parseFormulaStringIntern( char **string );

static PSStackItem* pop( tPSStackItem **topStack )
{
	tPSStackItem *result = *topStack;
	if( topStack )
	{
		*topStack = (*topStack)->prev;
		result->prev = NULL;
	}

	return result;
}

static void popFree( tPSStackItem **topStack )
{
	tPSStackItem *tmp = pop( topStack );
	if( tmp )
		free( tmp );
}

static double popDouble( tPSStackItem **topStack, char *error )
{
	tPSStackItem *item = pop( topStack );
	double result;
	if( item->type != PSTYPE_DOUBLE )
	{
		*error = TRUE;
		return 0.0f;
	}
	result = item->d.doublefloat;
	free( item );
	return result;
}

static char popBool( tPSStackItem **topStack, char *error )
{
	tPSStackItem *item = pop( topStack );
	char result;
	if( item->type != PSTYPE_BOOL )
	{
		*error = TRUE;
		return FALSE;
	}
	result = item->d.boolean;
	free( item );
	return result;
}

static tPSCommand* popCommand( tPSStackItem **topStack, char *error )
{
	tPSStackItem *item = pop( topStack );
	tPSCommand* result;
	if( item->type != PSTYPE_COMMAND )
	{
		*error = TRUE;
		return NULL;
	}
	result = item->d.command;
	free( item );
	return result;
}

static char push( tPSStackItem **topStack, tPSStackItem *item )
{
	if( *topStack )
		item->varList = (*topStack)->varList;
	item->prev = *topStack;
	*topStack = item;
	return TRUE;
}

static char pushBool( tPSStackItem **topStack, char boolean )
{
	tPSStackItem *item = (tPSStackItem*)malloc( sizeof(tPSStackItem) );

	//GfLogDebug( "pushBool( ...., %s )\n", boolean ? "true" : "false" );

	item->type = PSTYPE_BOOL;
	item->d.boolean = boolean;
	item->prev = NULL;
	if( !push( topStack, item ) ) {
		free( item );
		return FALSE;
	}
	return TRUE;
}

static char pushDouble( tPSStackItem **topStack, double value )
{
	//GfLogDebug( "pushDouble( ..., %f )\n", value );
	tPSStackItem *item = (tPSStackItem*)malloc( sizeof(tPSStackItem) );
	item->type = PSTYPE_DOUBLE;
	item->d.doublefloat = value;
	item->prev = NULL;
	if( !push( topStack, item ) ) {
		free( item );
		return FALSE;
	}
	return TRUE;
}

static char pushCommand( tPSStackItem **topStack, void *v_commands )
{
	//GfLogDebug( "pushCommand\n" );

	tPSCommand *commands = (tPSCommand*)v_commands;
	tPSStackItem *item = (tPSStackItem*)malloc( sizeof(tPSStackItem) );
	item->type = PSTYPE_COMMAND;
	item->d.command = commands;
	item->prev = NULL;
	if( !push( topStack, item ) ) {
		free( item );
		return FALSE;
	}
	return TRUE;
}

static char execCommands( tPSStackItem **topStack, tPSCommand *firstCommand, const char *path )
{
	tPSCommand *curCommand = firstCommand;

	while( curCommand ) {
		if( !curCommand->func( topStack, curCommand->data, path ) )
			return FALSE;
		curCommand = curCommand->next;
	}
	return TRUE;
}

static char cmdAdd( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushDouble( topStack, aa + bb );
	return TRUE;
}

static char cmdSub( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushDouble( topStack, aa - bb );
	return TRUE;
}

static char cmdMul( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error;
	double bb;
	double aa;

	error = FALSE;
	
	bb = popDouble( topStack, &error );
	aa = popDouble( topStack, &error );
	if( error )
		return FALSE;
	pushDouble( topStack, aa * bb );
	return TRUE;
}

static char cmdDiv( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushDouble( topStack, aa / bb );
	return TRUE;
}

static char cmdLt( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushBool( topStack, aa < bb );
	return TRUE;
}

static char cmdLe( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushBool( topStack, aa <= bb );
	return TRUE;
}

static char cmdEq( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushBool( topStack, aa == bb );
	return TRUE;
}

static char cmdGe( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushBool( topStack, aa >= bb );
	return TRUE;
}

static char cmdGt( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushBool( topStack, aa > bb );
	return TRUE;
}

static char cmdExch( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	tPSStackItem *item1 = pop( topStack );
	tPSStackItem *item2 = pop( topStack );

	if( !item2 )
		return FALSE;
	
	push( topStack, item1 );
	push( topStack, item2 );
	return TRUE;
}

static char cmdRoll( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	int jj = (int)floor( popDouble( topStack, &error ) + 0.5f );
	int nn = (int)floor( popDouble( topStack, &error ) + 0.5f );
	tPSStackItem **stackItems = (tPSStackItem**)malloc( sizeof( tPSStackItem* ) * nn );
	int xx;

	if( nn <= 0 ) {
		free( stackItems );
		return FALSE;
	}

	for( xx = 0; xx < nn; ++xx )
		stackItems[ xx ] = pop( topStack );
	if( !error || !stackItems[ nn - 1 ] ) {
		free( stackItems );
		return FALSE;
	}

	jj %= nn;
	while( jj < 0 )
		jj += nn;
	
	for( xx = nn + jj - 1; xx >= jj; --xx )
		push( topStack, stackItems[ xx % nn ] );
	free( stackItems );
	return TRUE;
}

static char cmdIf( tPSStackItem **topStack, void* /*dummy*/, const char* path )
{
	char error = FALSE;
	char aa = popBool( topStack, &error );
	tPSCommand *cmdFalse = popCommand( topStack, &error );
	tPSCommand *cmdTrue = popCommand( topStack, &error );

	if( error )
		return FALSE;

	if( aa )
		return execCommands( topStack, cmdTrue, path );
	else
		return execCommands( topStack, cmdFalse, path );
}

static char cmdMax( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushDouble( topStack, aa > bb ? aa : bb );
	return TRUE;
}

static char cmdMin( tPSStackItem **topStack, void* /*dummy*/, const char* /*path*/ )
{
	char error = FALSE;
	double bb = popDouble( topStack, &error );
	double aa = popDouble( topStack, &error );

	if( error )
		return FALSE;
	pushDouble( topStack, aa < bb ? aa : bb );
	return TRUE;
}

static char cmdPushNumber( tPSStackItem **topStack, void* number, const char* /*path*/ )
{
	double d_number;

	if( !number )
		return FALSE;
	d_number = *((double*)number);
	pushDouble( topStack, d_number );
	return TRUE;
}

static char cmdPushCommand( tPSStackItem **topStack, void* cmd, const char* /*path*/ )
{
	tPSCommand *command = (tPSCommand*)cmd;

	if( !command )
		return FALSE;
	pushCommand( topStack, command );
	return TRUE;
}

static char cmdPushVar( tPSStackItem **topStack, void* v_varname, const char* path )
{
	char *key = strdup( (char*)v_varname );
	tdble var;

	var = GfParmGetVariable((*topStack)->varList, path, key);
	pushDouble( topStack, (double)var );
	free(key);
	return TRUE;
}

static void parseSkipWhitespace( char **string )
{
	while( (*string)[0] == ' ' || (*string)[0] == '\r' || (*string)[0] == '\n' )	
		++(*string);
}

static void parseSkipComment( char **string )
{
	while( (*string)[0] != '\0' && (*string)[0] != '\r' && (*string)[0] != '\n' )
		++(*string);
}

static char parseSubCommands( char **string, tPSCommand **parent )
{
	tPSCommand *command;
	tPSCommand *newCommand;
	++(*string);
	command = parseFormulaStringIntern( string );

	newCommand = (tPSCommand*)malloc( sizeof( tPSCommand ) );
	newCommand->func = &cmdPushCommand;
	newCommand->data = command;
	newCommand->next = NULL;
	if( *parent )
		(*parent)->next = newCommand;
	*parent = newCommand;

	return TRUE;
}

static char parseNumber( char **string, tPSCommand **current )
{
	double dec = 1.0f;
	double result = 0.0f;
	tPSCommand *newCommand;

	while( ( (*string)[0] >= '0' && (*string)[0] <= '9' ) || (*string)[0] == '.' ) {
		if( (*string)[0] == '.' ) {
			dec /= 10.0f;
		} else {
			if( dec == 1.0f )
				result *= 10.0f;
			result += (double)((*string)[0] - '0');
			if( dec < 1.0f )
				dec /= 10.0f;
		}
		++(*string);
	}

	newCommand = (tPSCommand*)malloc( sizeof(tPSCommand) );
	newCommand->func = &cmdPushNumber;
	newCommand->data = (void*)malloc( sizeof( double ) );
	*((double*)(newCommand->data)) = result;
	newCommand->next = NULL;
	if( *current )
		(*current)->next = newCommand;
	*current = newCommand;

	return TRUE;
}

static char parseCommandVar( char **string, tPSCommand **current )
{
	tPSCommand *newCommand;
	char *cmdVar;
	int len = 0;
	int xx;

	while( ( (*string)[ len ] >= 'a' && (*string)[len] <= 'z' ) || ( (*string)[len] >= 'A' && (*string)[len] <= 'Z' ) )
		++len;
	
	cmdVar = (char*)malloc( sizeof( char ) * ( len + 1 ) );
	for( xx = 0; xx < len; ++xx ) {
		cmdVar[ xx ] = (*string)[0];
		++(*string);
	}
	cmdVar[ len ] = '\0';

	newCommand = (tPSCommand*)malloc( sizeof(tPSCommand) );
	newCommand->data = NULL;
	newCommand->next = NULL;
	
	if( strcmp( cmdVar, "add" ) == 0 )
		newCommand->func = &cmdAdd;
	else if( strcmp( cmdVar, "sub" ) == 0 )
		newCommand->func = &cmdSub;
	else if( strcmp( cmdVar, "mul" ) == 0 )
		newCommand->func = &cmdMul;
	else if( strcmp( cmdVar, "div" ) == 0 )
		newCommand->func = &cmdDiv;
	else if( strcmp( cmdVar, "lt" ) == 0 )
		newCommand->func = &cmdLt;
	else if( strcmp( cmdVar, "le" ) == 0 )
		newCommand->func = &cmdLe;
	else if( strcmp( cmdVar, "eq" ) == 0 )
		newCommand->func = &cmdEq;
	else if( strcmp( cmdVar, "gt" ) == 0 )
		newCommand->func = &cmdGt;
	else if( strcmp( cmdVar, "ge" ) == 0 )
		newCommand->func = &cmdGe;
	else if( strcmp( cmdVar, "exch" ) == 0 )
		newCommand->func = &cmdExch;
	else if( strcmp( cmdVar, "roll" ) == 0 )
		newCommand->func = &cmdRoll;
	else if( strcmp( cmdVar, "if" ) == 0 )
		newCommand->func = &cmdIf;
	else if( strcmp( cmdVar, "max" ) == 0 )
		newCommand->func = &cmdMax;
	else if( strcmp( cmdVar, "min" ) == 0 )
		newCommand->func = &cmdMin;
	else {
		newCommand->func = &cmdPushVar;
		newCommand->data = strdup( cmdVar );
	}
	free( cmdVar );

	if( *current )
		(*current)->next = newCommand;
	*current = newCommand;
	return TRUE;
}

static tPSCommand* parseFormulaStringIntern( char **string )
{
	parseSkipWhitespace( string );
	char ret = TRUE;
	tPSCommand *result = NULL;
	tPSCommand *current = NULL;
	
	while( ret ) {
		if( (*string)[0] == '{' ) {
			ret = parseSubCommands( string, &current );
		} else if( (*string)[0] >= '0' && (*string)[0] <= '9' ) {
			ret = parseNumber( string, &current );
		} else if( ( (*string)[0] >= 'a' && (*string)[0] <= 'z' ) || ( (*string)[0] >= 'A' && (*string)[0] <= 'Z' ) ) {
			ret = parseCommandVar( string, &current );
		} else if( (*string)[0] == '\0' || (*string)[0] == '}' ) {
			return result;
		} else if( (*string)[0] == '%' ) {
			parseSkipComment( string );
		} else {
			GfError( "Invalid token found: %c.", (*string)[0] );
			ret = FALSE;
		}
		if( current && !result )
			result = current;
		parseSkipWhitespace( string );
	}

	return result;
}

void* GfFormParseFormulaString( const char *string )
{
	char *str = strdup(string);
	char *dup = str;
	void *ret = (void*)parseFormulaStringIntern( &str );
	free(dup);
	return ret;
}

void GfFormFreeCommand( void *commands )
{
	tPSCommand *next;
	tPSCommand *current = (tPSCommand*)commands;
	while( current ) {
		if( current->data ) {
			if( current->func == &cmdPushVar )
				free( current->data );
			else if( current->func == &cmdPushNumber )
				free( current->data );
			else if( current->func == &cmdPushCommand )
				GfFormFreeCommand( (tPSCommand*)current->data );
			else
				GfError( "WARNING: Data found, but no clue about it's contents\n" );
		}
		next = current->next;
		free( current );
		current = next;
	}
}

tdble GfFormCalcFunc( void *cmd, void *parmHandle, const char *path )
{
	tdble result;
	char error = FALSE;
	tPSCommand *command = (tPSCommand *)cmd;
	tPSStackItem *item = NULL;
	pushDouble( &item, 0.0f );
	item->varList = parmHandle;
	execCommands( &item, command, path );
	result = (tdble)popDouble( &item, &error );
	while( item != NULL && !error )
		popFree( &item );
	return result;
}

/* New parse functions */

static void printFormTreeRec( tFormNode *node, int level )
{
	int xx;
	char *preOutput = (char*)malloc( sizeof( char ) * ( level * 2 + 1 ) );
	tFormNode *subnode;

	/* Make preoutput */
	for( xx = 0; xx < level; ++xx ) {
		preOutput[ xx * 2 ] = '|';
		preOutput[ xx * 2 + 1 ] = ' ';
	}
	if( level > 0 )
		preOutput[ level * 2 - 1 ] = '-';
	preOutput[ level * 2 ] = '\0';

	switch( node->type ) {
	case FORMNODE_TYPE_NUMBER:
		GfOut( "%s\"%f\" (number)\n", preOutput, node->number );
		break;
	case FORMNODE_TYPE_STRING:
		GfOut( "%s\"%s\" (string)\n", preOutput, node->string );
		break;
	case FORMNODE_TYPE_VARIABLE:
		GfOut( "%s\"%s\" (variable)\n", preOutput, node->string );
		break;
	case FORMNODE_TYPE_FUNCTION:
		GfOut( "%s\"%s\" (function)\n", preOutput, node->string );
		break;
	case FORMNODE_TYPE_TOPARSE_STRING:
		GfOut( "%s\"%s\" (to parse: string)\n", preOutput, node->string );
		break;
	case FORMNODE_TYPE_TOPARSE_BLOCK:
		GfOut( "%s\"%s\" (to parse: block)\n", preOutput, node->string );
		break;
	default:
		GfOut( "%s\"\" (unknown)\n", preOutput );
		break;
	}

	subnode = node->firstChild;
	while( subnode ) {
		printFormTreeRec( subnode, level + 1 );
		subnode = subnode->next;
	}

	free( preOutput );
}

/*static void printFormTree( tFormNode *node )
{
	tFormNode *nextNode = node;
	while( nextNode ) {
		printFormTreeRec( nextNode, 0 );
		nextNode = nextNode->next;
	}
}*/

static void parseFormStringStep1( tFormNode **node, const char *string )
{
	tFormNode *newNode;
	tFormNode *prevNode = NULL;
	int type = 0;
	int startPos = -1;
	int currentPos = 0;
	int stringLength = strlen( string );
	int xx;

	(*node) = NULL;

	while( currentPos < stringLength ) {
		if( startPos >= 0 ) {
			/* Check if block has ended */
			if( type == FORMNODE_TYPE_STRING && string[ currentPos ] == '#' ) {
				/* String ended here */
				newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
				newNode->firstChild = NULL;
				newNode->next = NULL;
				newNode->type = FORMNODE_TYPE_STRING;
				newNode->number = 0.0f;
				newNode->string = (char*)malloc( sizeof( char ) * ( currentPos - startPos + 1 ) );
				newNode->func = NULL;

				for( xx = startPos; xx < currentPos; ++xx )
					newNode->string[ xx - startPos ] = string[ xx ];
				newNode->string[ currentPos - startPos ] = '\0';
				if( *node )
					prevNode->next = newNode;
				else
					(*node) = newNode;
				startPos = -1;
				prevNode = newNode;
			} else if( type == FORMNODE_TYPE_NUMBER && ( string[ currentPos ] < '0' || string[ currentPos ] > '9' )
			                                        && string[ currentPos ] != '.' ) {
				/* Number ended here */
				newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
				newNode->firstChild = NULL;
				newNode->next = NULL;
				newNode->type = FORMNODE_TYPE_NUMBER;
				newNode->number = 0.0f;
				newNode->string = (char*)malloc( sizeof( char ) * ( currentPos - startPos + 1 ) );
				newNode->func = NULL;

				for( xx = startPos; xx < currentPos; ++xx )
					newNode->string[ xx - startPos ] = string[ xx ];
				newNode->string[ currentPos - startPos ] = '\0';
				newNode->number = (tdble)atof( newNode->string );
				FREEZ( newNode->string );

				if( *node )
					prevNode->next = newNode;
				else
					(*node) = newNode;
				prevNode = newNode;
				startPos = -1;
				--currentPos;
			} else if( type == FORMNODE_TYPE_TOPARSE_STRING &&
			           ( string[ currentPos ] < 'a' || string[ currentPos ] > 'z' ) &&
				   ( string[ currentPos ] < 'A' || string[ currentPos ] > 'Z' ) &&
				   ( string[ currentPos ] != '_' ) ) {
				/* Toparse String ended here */
				newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
				newNode->firstChild = NULL;
				newNode->next = NULL;
				newNode->type = FORMNODE_TYPE_TOPARSE_STRING;
				newNode->number = 0.0f;
				newNode->string = (char*)malloc( sizeof( char ) * ( currentPos - startPos + 1 ) );
				newNode->func = NULL;

				for( xx = startPos; xx < currentPos; ++xx )
					newNode->string[ xx - startPos ] = string[ xx ];
				newNode->string[ currentPos - startPos ] = '\0';

				if( *node )
					prevNode->next = newNode;
				else
					(*node) = newNode;
				prevNode = newNode;
				startPos = -1;
				--currentPos;
			}
		} else {
			if( string[ currentPos ] == '#' ) {
				startPos = currentPos + 1;
				type = FORMNODE_TYPE_STRING;
			} else if( string[ currentPos ] >= '0' && string[ currentPos ] <= '9' ) {
				startPos = currentPos;
				type = FORMNODE_TYPE_NUMBER;
			} else if( ( string[ currentPos ] >= 'a' && string[ currentPos ] <= 'z' ) ||
			           ( string[ currentPos ] >= 'A' && string[ currentPos ] <= 'Z' ) ) {
				startPos = currentPos;
				type = FORMNODE_TYPE_TOPARSE_STRING;
			} else if( string[ currentPos ] == '(' || string[ currentPos ] == ')' || string[ currentPos ] == ',' ||
			           string[ currentPos ] == '+' || string[ currentPos ] == '-' || string[ currentPos ] == '*' || string[ currentPos ] == '/' ||
				   string[ currentPos ] == '\\' )
			{
				/* Extra token */
				newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
				newNode->firstChild = NULL;
				newNode->next = NULL;
				newNode->type = FORMNODE_TYPE_TOPARSE_STRING;
				newNode->number = 0.0f;
				newNode->string = (char*)malloc( sizeof( char ) * 3 );
				newNode->func = NULL;

				if( ( string[ currentPos ] == '/' || string[ currentPos ] == '\\' ) &&
				    ( string[ currentPos + 1 ] == '/' || string[ currentPos + 1 ] == '\\' ) &&
				    string[ currentPos ] != string[ currentPos + 1 ] ) {
					newNode->string[ 0 ] = string[ currentPos ];
					newNode->string[ 1 ] = string[ currentPos + 1 ];
					newNode->string[ 2 ] = '\0';
					++currentPos;
				} else {
					newNode->string[ 0 ] = string[ currentPos ];
					newNode->string[ 1 ] = '\0';
				}

				if( *node )
					prevNode->next = newNode;
				else
					(*node) = newNode;
				prevNode = newNode;
			} else if( string[ currentPos ] != ' ' && string[ currentPos ] != '\n' && string[ currentPos ] != '\r' ) {
				GfError( "Formula parser: invalid token: \'%c\'\n", string[ currentPos ] );
			}
		}
		++currentPos;
	}
}

static void parseIntoBlocks( tFormNode **node )
{
	tFormNode *curNode = *node;
	tFormNode *prevNode = NULL;
	tFormNode *newNode;
	tFormNode *startNodeBracket1 = NULL;
	tFormNode *startNodeBracket2 = NULL;
	tFormNode *startNodeKomma1 = NULL;
	tFormNode *startNodeKomma2 = NULL;
	int bracketLevel = 0;
	char hasKomma = FALSE;

	while( curNode ) {
		if( curNode->type == FORMNODE_TYPE_TOPARSE_STRING && curNode->string[ 0 ] == '(' ) {
			if( bracketLevel == 0 ) {
				startNodeBracket1 = prevNode;
				startNodeBracket2 = curNode;
			}
			++bracketLevel;	
		} else if( curNode->type == FORMNODE_TYPE_TOPARSE_STRING && curNode->string[ 0 ] == ')' ) {
			if( bracketLevel == 1 ) {
				newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
				if( startNodeBracket2->next != curNode )
					newNode->firstChild = startNodeBracket2->next;
				else
					newNode->firstChild = NULL;
				newNode->next = curNode->next;
				newNode->type = FORMNODE_TYPE_TOPARSE_BLOCK;
				newNode->number = 0.0f;
				newNode->string = NULL;
				newNode->func = NULL;
				prevNode->next = NULL;
				if( startNodeBracket1 )
					startNodeBracket1->next = newNode;
				else
					*node = newNode;
				FREEZ( startNodeBracket2->string );
				free( startNodeBracket2 );
				prevNode = startNodeBracket1;
				FREEZ( curNode->string );
				free( curNode );
				curNode = newNode;
			}
			if( bracketLevel > 0 )
				--bracketLevel;
		} else if( bracketLevel == 0 && curNode->type == FORMNODE_TYPE_TOPARSE_STRING && curNode->string[ 0 ] == ',' ) {
			newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
			if( hasKomma )
				newNode->firstChild = startNodeKomma2->next;
			else
				newNode->firstChild = *node;
			newNode->next = curNode->next;
			newNode->type = FORMNODE_TYPE_TOPARSE_BLOCK;
			newNode->number = 0.0f;
			newNode->string = NULL;
			newNode->func = NULL;

			if( prevNode )
				prevNode->next = NULL;

			if( hasKomma ) {
				startNodeKomma1->next = newNode;
				FREEZ(startNodeKomma2->string);
				free( startNodeKomma2 );
			} else {
				*node = newNode;
			}
	
			startNodeKomma2 = curNode;
			startNodeKomma1 = newNode;

			hasKomma = TRUE;
		}
		if( curNode->firstChild )
			parseIntoBlocks( &(curNode->firstChild) );
		prevNode = curNode;
		curNode = curNode->next;
	}

	if( hasKomma ) {
		/* Wrap finishing block */
		newNode = (tFormNode*)malloc( sizeof( tFormNode ) );
		newNode->firstChild = startNodeKomma2->next;
		newNode->next = NULL;
		newNode->type = FORMNODE_TYPE_TOPARSE_BLOCK;
		newNode->number = 0.0f;
		newNode->string = NULL;
		newNode->func = NULL;

		parseIntoBlocks( &newNode );

		startNodeKomma1->next = newNode;

		FREEZ( startNodeKomma2->string );
		free( startNodeKomma2 );

		parseIntoBlocks( &newNode );
	}
}

static void parseFunctionList( tFormNode **node, tFuncBindList *funcList )
{
	tFormNode *prevNode = NULL;
	tFormNode *prevPrevNode = NULL;
	tFormNode *curNode = *node;
	tFormNode *newNext = NULL;
	int xx, yy;

	while( curNode ) {
		for( xx = 0; xx < funcList->length; ++xx ) {
			if( curNode->type == FORMNODE_TYPE_TOPARSE_STRING && curNode->firstChild == NULL &&
			    strcmp( curNode->string, funcList->list[ xx ].funcName ) == 0 ) {
				/* Function found */
				curNode->type = FORMNODE_TYPE_FUNCTION;
				curNode->func = funcList->list[ xx ].func;
				if( funcList->list[ xx ].hasArgBeforeFuncName ) {
					curNode->firstChild = prevNode;
					curNode->firstChild->next = NULL;
					if( prevPrevNode )
						prevPrevNode->next = curNode;
					else
						*node = curNode;
					prevNode = prevPrevNode;
					prevPrevNode = NULL;
					if( funcList->list[ xx ].arity > 1 ) {
						curNode->firstChild->next = curNode->next;
						newNext = curNode;
					}
					yy = 1;
				} else if( funcList->list[ xx ].arity > 1 ) {
					yy = funcList->list[ xx ].arity;
					if( curNode->next ) {
						curNode->firstChild = curNode->next->firstChild;
						newNext = curNode->next->next;
						FREEZ( curNode->next->string );
						free( curNode->next );
						curNode->next = newNext;
					}
					newNext = NULL;
				} else {
					curNode->firstChild = curNode->next;
					newNext = curNode;
					yy = 0;
				}
				while( yy < funcList->list[ xx ].arity ) {
					if( newNext )
						newNext = newNext->next;
					++yy;
				}
				/* newNext is the last new child */
				if( newNext ) {
					if( curNode != newNext ) {
						curNode->next = newNext->next;
						newNext->next = NULL;
					} else {
						/* Elsewise no additional arity */
						if( funcList->list[ xx ].hasArgBeforeFuncName )
							curNode->firstChild->next = NULL;
						else
							curNode->firstChild = NULL;
					}
				}

				FREEZ( curNode->string );
				break;
			}
		}

		if( curNode->firstChild )
			parseFunctionList( &curNode->firstChild, funcList );

		prevPrevNode = prevNode;
		prevNode = curNode;
		curNode = curNode->next;
	}
}

static void simplifyToParse( tFormNode **node )
{
	tFormNode *curNode = *node;
	tFormNode *prevNode = NULL;
	char skipChilds;
	char againThisNode;

	while( curNode ) {
		skipChilds = FALSE;
		againThisNode = FALSE;
		if( curNode->type == FORMNODE_TYPE_TOPARSE_BLOCK ) {
			if( curNode->firstChild == NULL ) {
				prevNode->next = curNode->next;
				FREEZ( curNode->string );
				free( curNode );
				curNode = prevNode;
				skipChilds = TRUE;
			} else {
				if( curNode->firstChild->next == NULL ) {
					if( prevNode ) {
						prevNode->next = curNode->firstChild;
						prevNode->next->next = curNode->next;
						FREEZ( curNode->string );
						free( curNode );
						curNode = prevNode->next;
					} else {
						*node = curNode->firstChild;
						(*node)->next = curNode->next;
						FREEZ( curNode->string );
						free( curNode );
						curNode = *node;
					}
					againThisNode = TRUE;
				} else {
					GfError( "WARNING: could not simplify all blocks in a formula\n" );
				}
			}
		} else if( curNode->type == FORMNODE_TYPE_TOPARSE_STRING ) {
			/* Unparsed string found: assume variable */
			curNode->type = FORMNODE_TYPE_VARIABLE;
		}

		if( !skipChilds && curNode->firstChild )
			simplifyToParse( &(curNode->firstChild) );

		if( !againThisNode ) {
			prevNode = curNode;
			curNode = curNode->next;
		}
	}
}

void* GfFormParseFormulaStringNew( const char *string )
{
	tFormula *formula = (tFormula*)malloc( sizeof( tFormula ) );
	int xx;

	parseFormStringStep1( &(formula->node), string );
	//printFormTree( formula->node );
	parseIntoBlocks( &(formula->node) );
	//GfOut( "\nparseIntoBlocks\n\n" );
	//printFormTree( formula->node );
	for( xx = 0; xx < funclistlength; ++xx ) {
		//GfOut( "\nfunction list %d\n\n", xx );
		parseFunctionList( &(formula->node), &(funclist[ xx ]) );
		//printFormTree( formula->node );
	}
	//GfOut( "\nsimplifyToParse\n\n" );
	simplifyToParse( &(formula->node) );
	//printFormTree( formula->node );

	formula->lastAnswer.validFields = 0;
	formula->lastAnswer.boolean = FALSE;
	formula->lastAnswer.integer = 0;
	formula->lastAnswer.number = 0.0f;
	formula->lastAnswer.string = NULL;

	return (void*)formula;
	//Clean
	
}

void GfFormFreeCommandNewRec( tFormNode *node )
{
	tFormNode *curNode = node;
	tFormNode *prevNode;
	while( curNode ) {
		FREEZ( curNode->string );
		if( curNode->firstChild )
			GfFormFreeCommandNewRec( curNode->firstChild );
		prevNode = curNode;
		curNode = curNode->next;
		free( prevNode );
	}
}

void GfFormFreeCommandNew(void *cmd)
{
	tFormula *formula = (tFormula*)cmd;

	GfFormFreeCommandNewRec( formula->node );
	if( formula->lastAnswer.string )
		free( formula->lastAnswer.string );
	free( formula );
}

static tFormAnswer eval( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer result;
	char *key;
	tdble var;

	switch( node->type ) {
	case FORMNODE_TYPE_NUMBER:
		result.validFields = FORMANSWER_TYPE_NUMBER;
		result.number = node->number;
		result.boolean = FALSE;
		result.integer = 0;
		result.string = NULL;
		if( floor( node->number + 0.5f ) == node->number ) {
			result.integer = (int)floor( node->number + 0.5f );
			result.validFields |= FORMANSWER_TYPE_INTEGER;
			if( result.integer == 0 ) {
				result.boolean = FALSE;
				result.validFields |= FORMANSWER_TYPE_BOOLEAN;
			} else if( result.integer == 1 ) {
				result.boolean = TRUE;
				result.validFields |= FORMANSWER_TYPE_BOOLEAN;
			}
		}
		break;
	case FORMNODE_TYPE_STRING:
		result.validFields = FORMANSWER_TYPE_STRING;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = strdup( node->string );
		break;
	case FORMNODE_TYPE_VARIABLE:
		if( !node->string ) {
			result.validFields = 0;
			result.boolean = FALSE;
			result.integer = 0;
			result.number = 0.0f;
			result.string = NULL;
			break;
		}
		key = strdup( node->string );
		var = GfParmGetVariable( parmHandle, path, key );
		free( key );
		result.validFields = FORMANSWER_TYPE_NUMBER;
		result.number = var;
		result.boolean = FALSE;
		result.integer = 0;
		result.string = NULL;
		if( floor( var + 0.5f ) == var ) {
			result.integer = (int)floor( var + 0.5f );
			result.validFields |= FORMANSWER_TYPE_INTEGER;
			if( result.integer == 0 ) {
				result.boolean = FALSE;
				result.validFields |= FORMANSWER_TYPE_BOOLEAN;
			} else if( result.integer == 1 ) {
				result.boolean = TRUE;
				result.validFields |= FORMANSWER_TYPE_BOOLEAN;
			}
		}
		break;
	case FORMNODE_TYPE_FUNCTION:
		result = (*node->func)( node->firstChild, parmHandle, path );
		break;
	default:
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		break;
	}

	// if( result.string )
	// 	GfLogDebug( "Result after eval: %s\n", result.string );
	// else
	// 	GfLogDebug( "No string result after eval (%x)\n", node->type );

	return result;
}

char GfFormCalcFuncNew(void *cmd, void *parmHandle, char const *path, char *boolean, int *integer, tdble *number, char ** string)
{
	tFormula *formula = (tFormula*)cmd;
	tFormAnswer answer;

	answer = eval( formula->node, parmHandle, path );
	//GfLogDebug( "answer = %x\n", answer.validFields );
	if( boolean && ( answer.validFields & FORMANSWER_TYPE_BOOLEAN ) )
		*boolean = answer.boolean;
	if( integer && ( answer.validFields & FORMANSWER_TYPE_INTEGER ) )
		*integer = answer.integer;
	if( number && ( answer.validFields & FORMANSWER_TYPE_NUMBER ) )
		*number = answer.number;
	if( string && ( answer.validFields & FORMANSWER_TYPE_STRING ) )
		*string = answer.string;
 
 	/* A string result is stored until the formula is evaluated again */
	if( formula->lastAnswer.string )
		free( formula->lastAnswer.string );
	formula->lastAnswer = answer;

	return ( answer.validFields != 0 ) ? TRUE : FALSE; 
}

/* Helper functions */

/* Definitions of function which can be called from in a attfunc */
static tFormAnswer func_op_add_cat( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg1;
	tFormAnswer arg2;
	tFormAnswer result;

	if( !node || !node->next ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	arg1 = eval( node, parmHandle, path );
	arg2 = eval( node->next, parmHandle, path );

	result.validFields = arg1.validFields & arg2.validFields &
	                     ( FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER | FORMANSWER_TYPE_STRING );
	result.boolean = ( result.validFields & FORMANSWER_TYPE_BOOLEAN ) ? arg1.boolean || arg2.boolean : FALSE;
	result.integer = ( result.validFields & FORMANSWER_TYPE_INTEGER ) ? arg1.integer + arg2.integer : 0;
	result.number = ( result.validFields & FORMANSWER_TYPE_NUMBER ) ? arg1.number + arg2.number : 0.0f;
	if( result.validFields & FORMANSWER_TYPE_STRING ) {
		result.string = (char*)malloc( sizeof( char ) * ( strlen( arg1.string ) + strlen( arg2.string ) + 1 ) );
		strcpy( result.string, arg1.string );
		strcat( result.string, arg2.string );
	} else {
		result.string = NULL;
	}

	if( arg1.string )
		free( arg1.string );
	if( arg2.string )
		free( arg2.string );

	return result;
}

static tFormAnswer func_op_sub_not( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg1;
	tFormAnswer arg2;
	tFormAnswer result;

	if( !node || !node->next ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	arg1 = eval( node, parmHandle, path );
	arg2 = eval( node->next, parmHandle, path );

	result.validFields = arg1.validFields & arg2.validFields & ( FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER );
	result.boolean = ( result.validFields & FORMANSWER_TYPE_BOOLEAN ) ? ( arg1.boolean == TRUE && arg2.boolean == FALSE ) : FALSE;
	result.integer = ( result.validFields & FORMANSWER_TYPE_INTEGER ) ? arg1.integer - arg2.integer : 0;
	result.number = ( result.validFields & FORMANSWER_TYPE_NUMBER ) ? arg1.number - arg2.number : 0.0f;
	result.string = NULL;

	if( arg1.string )
		free( arg1.string );
	if( arg2.string )
		free( arg2.string );

	return result;
}

static tFormAnswer func_op_mul( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg1;
	tFormAnswer arg2;
	tFormAnswer result;

	if( !node || !node->next ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	arg1 = eval( node, parmHandle, path );
	arg2 = eval( node->next, parmHandle, path );

	result.validFields = arg1.validFields & arg2.validFields & ( FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER );
	result.boolean = ( result.validFields & FORMANSWER_TYPE_BOOLEAN ) ? arg1.boolean && arg2.boolean : FALSE;
	result.integer = ( result.validFields & FORMANSWER_TYPE_INTEGER ) ? arg1.integer * arg2.integer : 0;
	result.number = ( result.validFields & FORMANSWER_TYPE_NUMBER ) ? arg1.number * arg2.number : 0.0f;
	result.string = NULL;

	if( arg1.string )
		free( arg1.string );
	if( arg2.string )
		free( arg2.string );

	return result;
}

static tFormAnswer func_op_div( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg1;
	tFormAnswer arg2;
	tFormAnswer result;

	if( !node || !node->next ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	arg1 = eval( node, parmHandle, path );
	arg2 = eval( node->next, parmHandle, path );

	result.validFields = arg1.validFields & arg2.validFields & ( FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER );
	result.boolean = FALSE;
	if( ( result.validFields & FORMANSWER_TYPE_INTEGER ) && arg2.integer != 0 && arg1.integer % arg2.integer == 0 ) {
		result.integer = arg1.integer / arg2.integer;
	} else {
		result.integer = 0;
		result.validFields &= ~FORMANSWER_TYPE_INTEGER;
	}
	if( ( result.validFields & FORMANSWER_TYPE_NUMBER ) && arg2.number != 0.0f  ) {
		result.number = arg1.number / arg2.number;
	} else {
		result.number = 0.0f;
		result.validFields &= ~FORMANSWER_TYPE_NUMBER;
	}
	result.string = NULL;

	if( arg1.string )
		free( arg1.string );
	if( arg2.string )
		free( arg2.string );

	return result;
}

static tFormAnswer func_max_or( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg;
	tFormAnswer result;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	result = eval( node, parmHandle, path );
	result.validFields &= FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER;
	if( result.string )
		free( result.string );
	result.string = NULL;
	while( ( node = node->next ) )
	{
		arg = eval( node, parmHandle, path );

		result.validFields &= arg.validFields;
		result.boolean = ( result.validFields & FORMANSWER_TYPE_BOOLEAN ) ? result.boolean || arg.boolean : FALSE;
		result.integer = ( result.validFields & FORMANSWER_TYPE_INTEGER ) ? (result.integer > arg.integer ? result.integer : arg.integer ) : 0;
		result.number = ( result.validFields & FORMANSWER_TYPE_NUMBER ) ? ( result.number > arg.number ? result.number : arg.number ) : 0.0f;
	
		if( arg.string )
			free( arg.string );
	}

	return result;
}

static tFormAnswer func_min_and( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer arg;
	tFormAnswer result;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	result = eval( node, parmHandle, path );
	result.validFields &= FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER;
	if( result.string )
		free( result.string );
	result.string = NULL;
	while( ( node = node->next ) )
	{
		arg = eval( node, parmHandle, path );

		result.validFields &= arg.validFields;
		result.boolean = ( result.validFields & FORMANSWER_TYPE_BOOLEAN ) ? result.boolean && arg.boolean : FALSE;
		result.integer = ( result.validFields & FORMANSWER_TYPE_INTEGER ) ? (result.integer < arg.integer ? result.integer : arg.integer ) : 0;
		result.number = ( result.validFields & FORMANSWER_TYPE_NUMBER ) ? ( result.number < arg.number ? result.number : arg.number ) : 0.0f;
	
		if( arg.string )
			free( arg.string );
	}

	return result;
}

static tFormAnswer func_op_compare( tFormNode *node, void *parmHandle, char const *path, char smaller, char equal, char greater )
{
	tFormAnswer arg1;
	tFormAnswer arg2;
	tFormAnswer result;
	int field;
	char found = FALSE;

	if( !node || !node->next ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	arg1 = eval( node, parmHandle, path );
	arg2 = eval( node->next, parmHandle, path );

	field = arg1.validFields & arg2.validFields;

	if( field & FORMANSWER_TYPE_INTEGER ) {
		found = TRUE;
		result.boolean = ( arg1.integer < arg2.integer && smaller ) ||
		                 ( arg1.integer == arg2.integer && equal ) ||
				 ( arg1.integer > arg2.integer && greater );
	} else if( field & FORMANSWER_TYPE_NUMBER ) {
		found = TRUE;
		result.boolean = ( arg1.number < arg2.number && smaller ) ||
		                 ( arg1.number == arg2.number && equal ) ||
				 ( arg1.number > arg2.number && greater );
	} else if( field & FORMANSWER_TYPE_STRING ) {
		found = TRUE;
		result.boolean = ( strcmp( arg1.string, arg2.string ) < 0 && smaller ) ||
		                 ( strcmp( arg1.string, arg2.string ) == 0 && equal ) ||
				 ( strcmp( arg1.string, arg2.string ) > 0 && greater );
	} else if( field & FORMANSWER_TYPE_BOOLEAN ) {
		found = equal || ( smaller && greater );
		result.boolean = ( arg1.boolean == arg2.boolean && equal ) ||
		                 ( arg1.boolean != arg2.boolean && smaller && greater );
	} else {
		result.boolean = FALSE;
	}

	if( found ) {
		result.validFields = FORMANSWER_TYPE_BOOLEAN | FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER;
		result.number = result.boolean ? 1.0f : 0.0f;
		result.integer = result.boolean ? 1 : 0;
		result.string = NULL;
	} else {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
	}
		
	return result;
}

static tFormAnswer func_op_lt( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, TRUE, FALSE, FALSE );
}

static tFormAnswer func_op_le( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, TRUE, TRUE, FALSE );
}

static tFormAnswer func_op_eq( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, FALSE, TRUE, FALSE );
}

static tFormAnswer func_op_ne( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, TRUE, FALSE, TRUE );
}

static tFormAnswer func_op_ge( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, FALSE, TRUE, TRUE );
}

static tFormAnswer func_op_gt( tFormNode *node, void *parmHandle, char const *path )
{
	return func_op_compare( node, parmHandle, path, FALSE, FALSE, TRUE );
}

static tFormAnswer func_if( tFormNode *node, void *parmHandle, const char *path )
{
	tFormAnswer result;
	tFormAnswer isTrueAnswer;
	char isTrue;

	result.validFields = 0;
	result.boolean = FALSE;
	result.integer = 0;
	result.number = 0.0f;
	result.string = NULL;
	
	if( !node )
		return result;

	isTrueAnswer = eval( node, parmHandle, path );
	if( isTrueAnswer.validFields & FORMANSWER_TYPE_BOOLEAN )
		isTrue = isTrueAnswer.boolean;
	else if( isTrueAnswer.validFields & FORMANSWER_TYPE_INTEGER )
		isTrue = isTrueAnswer.integer != 0;
	else if( isTrueAnswer.validFields & FORMANSWER_TYPE_NUMBER )
		isTrue = isTrueAnswer.number != 0.0f;
	else if( isTrueAnswer.validFields & FORMANSWER_TYPE_STRING )
		isTrue = !( strcmp( isTrueAnswer.string, "" ) == 0 );
	else
		return result;
	
	if( !node->next || ( !isTrue && !node->next->next ) )
		return result;
	
	if( isTrue )
		result = eval( node->next, parmHandle, path );
	else
		result = eval( node->next->next, parmHandle, path );

	return result;
}

static tFormAnswer func_sqrt( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer result;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	result = eval( node, parmHandle, path );
	result.validFields &= FORMANSWER_TYPE_INTEGER | FORMANSWER_TYPE_NUMBER;
	if( result.string )
		free( result.string );
	result.string = NULL;
	result.boolean = FALSE;

	if( result.number < 0 ) {
		result.integer = 0;
		result.number = 0.0f;
		result.validFields = 0;
	} else {
		result.number = sqrt( result.number );
		if( (int)floor( result.number + 0.5f ) * (int)floor( result.number + 0.5f ) == result.integer ) {
			result.integer = (int)floor( result.number + 0.5f );
		} else {
			result.integer = 0;
			result.validFields &= ~FORMANSWER_TYPE_INTEGER;
		}
	}

	return result;

}

static tFormAnswer func_log( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer result;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	result = eval( node, parmHandle, path );
	result.validFields &= FORMANSWER_TYPE_NUMBER;
	if( result.string )
		free( result.string );
	result.string = NULL;
	result.integer = 0;
	result.boolean = FALSE;

	if( result.number <= 0 ) {
		result.number = 0.0f;
		result.validFields = 0;
	} else if( result.validFields & FORMANSWER_TYPE_NUMBER ) {
		result.number = log( result.number );
	}

	return result;

}

static tFormAnswer func_toString( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer result;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	
	result = eval( node, parmHandle, path );
	if( result.string )
		free( result.string );
	
	if( result.validFields & FORMANSWER_TYPE_INTEGER ) {
		result.validFields = FORMANSWER_TYPE_STRING;
		result.string = (char*)malloc( sizeof( char ) * 20 );
		snprintf( result.string, 20, "%d", result.integer );
	} else if( result.validFields & FORMANSWER_TYPE_NUMBER ) {
		result.validFields = FORMANSWER_TYPE_STRING;
		result.string = (char*)malloc( sizeof( char ) * 20 );
		snprintf( result.string, 20, "%f", result.number );
	} else {
		result.validFields = 0;
		result.string = NULL;
	}
		
	result.integer = 0;
	result.number = 0.0f;
	result.boolean = FALSE;

	return result;
}

static tFormAnswer func_toAlpha( tFormNode *node, void *parmHandle, char const *path )
{
	tFormAnswer result;
	tFormAnswer lengthArg;
	int length;
	int maxLength = -1;
	int number = -1;
	int xx;

	if( !node ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}
	if( node->next )
	{
		lengthArg = eval( node->next, parmHandle, path );
		if( lengthArg.validFields & FORMANSWER_TYPE_INTEGER )
			maxLength = lengthArg.integer;
		else if( lengthArg.validFields & FORMANSWER_TYPE_NUMBER )
			maxLength = (int)ceil( lengthArg.number );
		else
			maxLength = -1;
		if( lengthArg.string )
			free( lengthArg.string );
	}
	
	result = eval( node, parmHandle, path );
	if( result.string )
		free( result.string );
	
	if( result.validFields & FORMANSWER_TYPE_INTEGER ) {
		number = result.integer;
	} else if( result.validFields & FORMANSWER_TYPE_NUMBER ) {
		if( floor( result.number + 0.5f ) == result.number )
			number = (int)floor( result.number + 0.5f );
	}

	if( number < 0 ) {
		result.validFields = 0;
		result.boolean = FALSE;
		result.integer = 0;
		result.number = 0.0f;
		result.string = NULL;
		return result;
	}

	length = (int)floor( log( (double)number ) / log( (double)26 ) );
	length += 1;
	if( number <= 0 )
		length = 1;
	if( length < maxLength )
		length = maxLength;

	result.string = (char*)malloc( sizeof( char ) * ( length + 1 ) );
	result.string[ length ] = '\0';
	//GfLogDebug( "result.string[ %d ] = \'\\0\'\n", length );
	for( xx = length - 1; xx >= 0; --xx ) {
		result.string[ xx ] = (char)( 'A' + ( number % 26 ) );
		number -= number % 26;
		number /= 26;
	}

	result.validFields = FORMANSWER_TYPE_STRING;
	result.integer = 0;
	result.number = 0.0f;
	result.boolean = FALSE;

	return result;
}

