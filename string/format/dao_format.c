/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2014, Limin Fu
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// 2011-01: Danilov Aleksey, initial implementation.

#include<ctype.h>
#include<stdio.h>
#include<string.h>

#include"dao.h"
#include"daoValue.h"
#include"daoStdtype.h"
#include"daoNumtype.h"


struct Format
{
	char sign;
	char notation;
	int precision;
	int alignment;
	char centered;
	int indexed, sliced;
	int index, index2;
	char *name;
	int namelen;
};

typedef struct Format Format;

static int PrintValue( DaoValue *value, DString *dest, Format *format, DString *tmp, void *buffer)
{
	char *buf = buffer, fmt[20] = {'%', 0};
	int i = 1, diff, alignment = format->alignment, notation, len = 0, error = 0, integer = 0, res;
	int indexed = format->indexed, sliced = format->sliced, index = format->index, index2 = format->index2;
	int centered = format->centered;
	int type = DaoValue_Type( value );
	DNode *node;
	DString *valstr;
	DaoList *vallist;
	DaoMap *valmap;
	DaoTuple *valtuple;
	DaoArray *valarray;
	void *number = NULL;
	notation = ( format->notation != 0 );
	if( notation ){
		if( format->sign )
			fmt[i++] = ( format->sign == 1)? '+' : ' ';
		if( format->precision ){
			fmt[i++] = '.';
			sprintf( fmt + i, "%i", format->precision );
		}
		if( format->notation == 'i' || format->notation == 'x' || format->notation == 'X' ){
			strcat( fmt, "ll" );
			integer = 1;
		}
		len = strlen( fmt );
		fmt[len++] = format->notation;
	}
	switch( type ){
	case DAO_NONE:
		strcpy( buf, "none" );
		break;
	case DAO_INTEGER:
		if( notation ){
			if( integer )
				sprintf( buf, fmt, DaoValue_TryGetInteger( value ) );
			else
				sprintf( buf, fmt, (double)DaoValue_TryGetInteger( value ) );
		}
		else
			sprintf( buf, "%lli", DaoValue_TryGetInteger( value ) );
		break;
	case DAO_FLOAT:
		if( notation ){
			if( integer )
				sprintf( buf, fmt, (dao_integer)DaoValue_TryGetFloat( value ) );
			else
				sprintf( buf, fmt, DaoValue_TryGetFloat( value ) );
		}
		else
			sprintf( buf, "%g", DaoValue_TryGetFloat( value ) );
		break;
	case DAO_COMPLEX:
		if( notation ){
			dao_complex comp = DaoValue_TryGetComplex( value );
			strcat( fmt, "%+" );
			if( format->sign )
				strncpy( fmt + len + 2, fmt + 2, len - 2 );
			else
				strncpy( fmt + len + 2, fmt + 1, len - 1 );
			if( integer )
				sprintf( buf, fmt, (dao_integer)comp.real, (dao_integer)comp.imag );
			else
				sprintf( buf, fmt, comp.real, comp.imag );
		}
		else
			sprintf( buf, "%g%+g$", DaoValue_TryGetComplex( value ).real, DaoValue_TryGetComplex( value ).imag );
		break;
	case DAO_ENUM:
		DaoEnum_MakeName( DaoValue_CastEnum( value ), tmp );
		break;
	case DAO_STRING:
		valstr = DaoString_Get( DaoValue_CastString( value ) );
		if( sliced ){
			if( index < 0 )
				index += DString_Size( valstr );
			if( index2 < 0 )
				index2 += DString_Size( valstr );
			if( index < 0 || index >= DString_Size( valstr ) || index2 < 0 || index2 >= DString_Size( valstr ))
				return 9;
			if ( index2 < index )
				return 10;
			if( index != index2 ){
				DString_SetBytes( tmp,  DString_GetData( valstr ) + index, index2 - index + 1 );
				break;
			}
			sliced = 0;
			indexed = 1;
		}
		if( indexed ){
			if( index < 0 )
				index += DString_Size( valstr );
			if( index < 0 || index >= DString_Size( valstr ) )
				return 6;
			sprintf( buf, notation? fmt : "%i", (int)DString_GetData( valstr )[index] );
		}
		else
			DString_Assign( tmp, valstr );
		break;
	case DAO_LIST:
		vallist = DaoValue_CastList( value );
		if( sliced ){
			if( index < 0 )
				index += DaoList_Size( vallist );
			if( index2 < 0 )
				index2 += DaoList_Size( vallist );
			if( index < 0 || index >= DaoList_Size( vallist ) || index2 < 0 || index2 >= DaoList_Size( vallist ) )
				return 9;
			if ( index2 < index )
				return 10;
			if( index != index2 ){
				format->sliced = 0;
				for( i = index; i <= index2; i++ ){
					if( i != index )
						DString_AppendChars( dest, ", " );
					if( ( res = PrintValue( DaoList_GetItem( vallist, i ), dest, format, tmp, buffer ) ) == 1
							|| res == 2)
						return 2;
					else if( res == 3 || res == 4 )
						error = 4;
				}
				break;
			}
			sliced = 0;
			indexed = 1;
		}
		if( indexed ){
			if( index < 0 )
				index += DaoList_Size( vallist );
			if( index < 0 || index >= DaoList_Size( vallist ) )
				return 6;
			format->indexed = 0;
			return PrintValue( DaoList_GetItem( vallist, index ), dest, format, tmp, buffer );
		}
		else
			for( i = 0; i < DaoList_Size( vallist ); i++ ){
				if( i )
					DString_AppendChars( dest, ", " );
				if( ( res = PrintValue( DaoList_GetItem( vallist, i ), dest, format, tmp, buffer ) ) == 1
						|| res == 2)
					return 2;
				else if( res == 3 || res == 4 )
					error = 4;
			}
		break;
	case DAO_MAP:
		valmap = DaoValue_CastMap( value );
		for( i = 0, node = DaoMap_First( valmap ); node; node = DaoMap_Next( valmap, node ), i++ ){
			if( i )
				DString_AppendChars( dest, ", " );
			if( ( res = PrintValue( DNode_Key( node ), dest, format, tmp, buffer ) ) == 1 || res == 2 )
				return 2;
			else if( res == 3 || res == 4 )
				error = 4;
			DString_AppendChars( dest, valmap->value->hashing ? " : " : " => " );
			if( ( res = PrintValue( DNode_Value( node ), dest, format, tmp, buffer ) ) == 1 || res == 2 )
				return 2;
			else if( res == 3 || res == 4 )
				error = 4;
		}
		break;
#ifdef DAO_WITH_NUMARRAY
	case DAO_ARRAY:
		valarray = DaoValue_CastArray( value );
		if( sliced ){
			dao_complex comp = {0, 0};
			int rowsize = 1, maxdim;
			if( DaoArray_SizeOfDim( valarray, 0 ) != 1 ){
				maxdim = DaoArray_SizeOfDim( valarray, 0 );
				for( i = 1; i < DaoArray_DimCount( valarray ); i++ )
					rowsize *= DaoArray_SizeOfDim( valarray, i );
			}
			else
				maxdim = DaoArray_SizeOfDim( valarray, 1 );
			if( index < 0 )
				index += maxdim;
			if( index2 < 0 )
				index2 += maxdim;
			if( index < 0 || index >= maxdim || index2 < 0 || index2 >= maxdim )
				return 9;
			if ( index2 < index )
				return 10;
			switch( DaoArray_NumType( valarray ) ){
			case DAO_INTEGER: number = DaoInteger_New( 0 ); break;
			case DAO_FLOAT:   number = DaoFloat_New( 0 ); break;
			case DAO_COMPLEX: number = DaoComplex_New( comp ); break;
			default: break;
			}
			format->sliced = 0;
			for( i = index*rowsize; i < ( index2 + 1 )*rowsize; i++ ){
				if( i != index*rowsize )
					DString_AppendChars( dest, ", " );
				switch( DaoArray_NumType( valarray ) ){
				case DAO_INTEGER: DaoInteger_Set( (DaoInteger*)number, DaoArray_GetInteger( valarray, i ) ); break;
				case DAO_FLOAT:   DaoFloat_Set( (DaoFloat*)number, DaoArray_GetFloat( valarray, i ) ); break;
				case DAO_COMPLEX: DaoComplex_Set( (DaoComplex*)number, DaoArray_GetComplex( valarray, i ) ); break;
				default: break;
				}
				PrintValue( (DaoValue*)number, dest, format, tmp, buffer );
			}
			dao_free( number );
		}
		else if( indexed ){
			if( index < 0 )
				index += DaoArray_Size( valarray );
			if( index < 0 || index >= DaoArray_Size( valarray ) )
				return 6;
			format->indexed = 0;
			switch( DaoArray_NumType( valarray ) ){
			case DAO_INTEGER: number = DaoInteger_New( DaoArray_GetInteger( valarray, index ) ); break;
			case DAO_FLOAT:   number = DaoFloat_New( DaoArray_GetFloat( valarray, index ) ); break;
			case DAO_COMPLEX: number = DaoComplex_New( DaoArray_GetComplex( valarray, index ) ); break;
			default: break;
			}
			res = PrintValue( (DaoValue*)number, dest, format, tmp, buffer );
			dao_free( number );
			return res;
		}
		else{
			dao_complex comp = {0, 0};
			switch( DaoArray_NumType( valarray ) ){
			case DAO_INTEGER: number = DaoInteger_New( 0 ); break;
			case DAO_FLOAT:   number = DaoFloat_New( 0 ); break;
			case DAO_COMPLEX: number = DaoComplex_New( comp ); break;
			default: break;
			}
			for( i = 0; i < DaoArray_Size( valarray ); i++ ){
				if( i )
					DString_AppendChars( dest, ", " );
				switch( DaoArray_NumType( valarray ) ){
				case DAO_INTEGER: DaoInteger_Set( (DaoInteger*)number, DaoArray_GetInteger( valarray, i ) ); break;
				case DAO_FLOAT:   DaoFloat_Set( (DaoFloat*)number, DaoArray_GetFloat( valarray, i ) ); break;
				case DAO_COMPLEX: DaoComplex_Set( (DaoComplex*)number, DaoArray_GetComplex( valarray, i ) ); break;
				default: break;
				}
				PrintValue( (DaoValue*)number, dest, format, tmp, buffer );
			}
			dao_free( number );
		}
		break;
#endif
	case DAO_TUPLE:
		valtuple = DaoValue_CastTuple( value );
		if( format->name ){
			DString_SetBytes( tmp, format->name, format->namelen );
			if( ( value = DaoTuple_GetItem( valtuple, DaoTuple_GetIndex( valtuple, tmp ) ) ) == NULL )
				return 8;
			format->name = NULL;
			return PrintValue( value, dest, format, tmp, buffer );
		}
		else if( indexed ){
			if( index < 0 )
				index += DaoTuple_Size( valtuple );
			if( index < 0 || index >= DaoTuple_Size( valtuple ) )
				return 6;
			format->indexed = 0;
			return PrintValue( DaoTuple_GetItem( valtuple, index ), dest, format, tmp, buffer );
		}
		else
			for( i = 0; i < DaoTuple_Size( valtuple ); i++ ){
				if( i )
					DString_AppendChars( dest, ", " );
				if( ( res = PrintValue( DaoTuple_GetItem( valtuple, i ), dest, format, tmp, buffer ) ) == 1 || res == 2 )
					return 2;
				else if( res == 3 || res == 4 )
					error = 4;
			}
		break;
	default: return 1;
	}
	if( format->name && type != DAO_TUPLE )
		return 7;
	if( sliced && type != DAO_STRING && type != DAO_LIST && type != DAO_ARRAY )
		return 11;
	if( !indexed || ( type >= DAO_STRING && type != DAO_MAP ) ){
		if( type <= DAO_COMPLEX || ( type == DAO_STRING && indexed ) ){
			len = strlen( buf );
			if( centered )
				diff = ( alignment - len )/2 + ( ( ( alignment - len )%2 )? 1 : 0 );
			else
				diff = alignment - len;
			if( diff > 0 )
				for( i = 0; i < diff; i++ )
					DString_AppendChar( dest, ' ' );
			DString_AppendChars( dest, buf );
			if( centered )
				diff = ( alignment - len )/2;
			else
				diff = ( alignment < 0 )? -alignment - len : 0;
			if( diff > 0 )
				for( i = 0; i < diff; i++ )
					DString_AppendChar( dest, ' ' );
		}
		else if( type == DAO_ENUM || ( type == DAO_STRING && !indexed ) ){
			len = DString_Size( tmp );
			if( centered )
				diff = ( alignment - len )/2 + ( ( ( alignment - len )%2 )? 1 : 0 );
			else
				diff = alignment - len;
			if( diff > 0 )
				for( i = 0; i < diff; i++ )
					DString_AppendChar( dest, ' ' );
			DString_Append( dest, tmp );
			if( centered )
				diff = ( alignment - len )/2;
			else
				diff = ( alignment < 0 )? -alignment - len : 0;
			if( diff > 0 )
				for( i = 0; i < diff; i++ )
					DString_AppendChar( dest, ' ' );
		}
	}
	else
		return 5;
	if( notation )
		return ( type != DAO_ENUM && ( type != DAO_STRING || indexed ) && type != DAO_NONE )?
					error : 3;
	else
		return error;
}

int IsNamedValue( DaoTuple *tup )
{
	if ( tup->size == 2 && tup->values[0]->type == DAO_ENUM ){
		DaoEnum *en = &tup->values[0]->xEnum;
		return en->subtype == DAO_ENUM_SYM || en->subtype == DAO_ENUM_STATE;
	}
	return 0;
}

static void DaoFormat( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *fmt = DaoString_Get( DaoValue_CastString( p[0] ) );
	DString *str = DString_New();
	DString *tmp = DString_New();
	Format format;
	DaoValue *value;
	int pos, pos2, prevpos, sliced;
	int num, error, width, alignment, sign, precision, notation, argend, index, index2, indexed, centered, namelen;
	int argnamelen;
	char *mbs, buf[200], *name, *argname;
	DString_Reserve( str, DString_Size( fmt ) );
	format.name = NULL;

	mbs = DString_GetData( fmt );
	for( pos = 0; pos < DString_Size( fmt ); pos += 2){
		num = error = alignment = sign = precision = notation = argend = index = indexed = centered = namelen = 0;
		sliced = index2 = argnamelen = 0;
		name = argname = NULL;
		value = NULL;
		prevpos = pos;
		pos = DString_FindChar( fmt, '$', pos );
		if( pos == -1 ){
			DString_AppendBytes( str, mbs + prevpos, DString_Size( fmt ) - prevpos );
			break;
		}
		else{
			DString_AppendBytes( str, mbs + prevpos, pos - prevpos );
			if( mbs[pos + 1] == '$' ){
				DString_AppendChar( str, '$' );
				continue;
			}
			else if( mbs[pos + 1] == '(' ){
				prevpos = pos + 2;
				pos = DString_FindChar( fmt, ')', prevpos );
				if( pos == -1 ){
					DaoProcess_RaiseWarning( proc, NULL, "Placeholder bracket not closed!" );
					break;
				}
				else if( pos == prevpos ){
					DaoProcess_RaiseWarning( proc, NULL, "Empty placeholder!" );
					continue;
				}
				if( ( isalpha( mbs[prevpos] ) || mbs[prevpos] == '_' ) && !argend ){
					int litera = 0;
					argname = mbs + prevpos;
					for(; prevpos < pos; prevpos++, argnamelen++ )
						if( isalpha( mbs[prevpos] ) )
							litera = 1;
						else if( isdigit( mbs[prevpos] ) ){
							if( !litera )
								break;
						}
						else if( mbs[prevpos] != '_' )
							break;
					if( !litera ){
						sprintf( buf, "Invalid placeholder name (stopped on char '%c')!", mbs[prevpos] );
						DaoProcess_RaiseWarning( proc, NULL, buf );
						pos--;
						DString_AppendChar( str, '?' );
						continue;
					}
				}
				for(; prevpos < pos; prevpos++ )
					if( isdigit( mbs[prevpos] ) && !argend && !argname )
						num = num*10 + ( mbs[prevpos] - '0' );
					else if( mbs[prevpos - 1] == '(' ){
						sprintf( buf, "Invalid character in placeholder ID ('%c')!", mbs[prevpos] );
						DaoProcess_RaiseWarning( proc, NULL, buf );
						error = 1;
						break;
					}
					else if( mbs[prevpos] == '.' && !argend ){
						argend = 1;
						prevpos++;
						if( strchr( ":<>=)", mbs[prevpos] ) ){
							sprintf( buf, "Empty argument field name!" );
							DaoProcess_RaiseWarning( proc, NULL, buf );
							error = 1;
							break;
						}
						if( !isalpha( mbs[prevpos] ) && mbs[prevpos] != '_' ){
							sprintf( buf, "Invalid character in argument field name ('%c')!", mbs[prevpos] );
							DaoProcess_RaiseWarning( proc, NULL, buf );
							error = 1;
							break;
						}
						name = mbs + prevpos;
						namelen = 1;
						for( prevpos++; isalnum( mbs[prevpos] ) || mbs[prevpos] == '_'; prevpos++  )
							namelen++;
						if( namelen > 100 ){
							sprintf( buf, "Argument field name too large (greater than 100 characters)!" );
							DaoProcess_RaiseWarning( proc, NULL, buf );
							error = 1;
							break;
						}
						prevpos--;
					}
					else if( mbs[prevpos] == '[' && !argend ){
						argend = 1;
						prevpos++;
						pos2 = DString_FindChar( fmt, ']', prevpos );
						if( pos2 == -1 ){
							DaoProcess_RaiseWarning( proc, NULL, "Index bracket not closed!" );
							error = 1;
							break;
						}
						if( prevpos == pos2 ){
							DaoProcess_RaiseWarning( proc, NULL, "Empty index!" );
							error = 1;
							break;
						}
						if( mbs[prevpos] == '-' || mbs[prevpos] == '+' ){
							if( prevpos == pos2 - 1 || mbs[prevpos + 1] == ':' ){
								DaoProcess_RaiseWarning( proc, NULL, "Missing index number!" );
								error = 1;
								break;
							}
							indexed = ( mbs[prevpos] == '-' )? -1 : 1;
							prevpos++;
						}
						else
							indexed = 1;
						for(; prevpos < pos2; prevpos++ )
							if( isdigit( mbs[prevpos] ) )
								index = index*10 + ( mbs[prevpos] - '0' );
							else if ( mbs[prevpos] == ':' ){
								prevpos++;
								if( prevpos == pos2 ){
									sliced = 1;
									index2 = -1;
									break;
								}
								if( mbs[prevpos] == '-' || mbs[prevpos] == '+' ){
									if( prevpos == pos2 - 1 ){
										DaoProcess_RaiseWarning( proc, NULL, "Missing index number!" );
										error = 1;
										break;
									}
									sliced = ( mbs[prevpos] == '-' )? -1 : 1;
									prevpos++;
								}
								else
									sliced = 1;
								for(; prevpos < pos2; prevpos++ )
									if( isdigit( mbs[prevpos] ) )
										index2 = index2*10 + ( mbs[prevpos] - '0' );
									else{
										sprintf( buf, "Invalid character in index ('%c')!", mbs[prevpos] );
										DaoProcess_RaiseWarning( proc, NULL, buf );
										sliced = 0;
										error = 1;
										break;
									}
								index2 *= sliced;
								if( error )
									break;
							}
							else{
								sprintf( buf, "Invalid character in index ('%c')!", mbs[prevpos] );
								DaoProcess_RaiseWarning( proc, NULL, buf );
								indexed = 0;
								error = 1;
								break;
							}
						index *= indexed;
						if( sliced )
							indexed = 0;
						prevpos = pos2;
					}
					else if( strchr( "<>=", mbs[prevpos] ) ){
						argend = 1;
						if( prevpos == pos - 1 ){
							DaoProcess_RaiseWarning( proc, NULL, "Field width not specified!" );
							break;
						}
						alignment = ( mbs[prevpos] == '<' )? -1 : 1;
						if( mbs[prevpos] == '=' )
							centered = 1;
						width = 0;
						for( prevpos++; prevpos < pos; prevpos++)
							if( isdigit(mbs[prevpos] ) )
								width = width*10 + ( mbs[prevpos] - '0' );
							else{
								sprintf( buf, "Invalid character in field width ('%c')!", mbs[prevpos] );
								DaoProcess_RaiseWarning( proc, NULL, buf );
								width = 0;
								break;
							}
						if( width > 1000 || width < 0 ){
							DaoProcess_RaiseWarning( proc, NULL,
									"Field width too large (greater than 1000)!" );
							width = 0;
						}
						alignment = width? alignment*width : 0;
					}
					else if( mbs[prevpos] == ':' ){
						argend = 1;
						if( prevpos == pos - 1 || strchr( "<>=", mbs[prevpos + 1] ) ){
							DaoProcess_RaiseWarning( proc, NULL, "Empty numeric format!" );
							continue;
						}
						prevpos++;
						if( mbs[prevpos] == '+' ){
							sign = 1;
							prevpos++;
						}
						else if( mbs[prevpos] == ' ' ){
							sign = -1;
							prevpos++;
						}
						if( prevpos != pos && !strchr( "<>=.", mbs[prevpos] ) ){
							notation = mbs[prevpos];
							if( !strchr( "ixXfgG", notation ) ){
								sprintf( buf, "Invalid numeric format ('%c')!", mbs[prevpos] );
								DaoProcess_RaiseWarning( proc, NULL, buf );
								notation = 0;
								continue;
							}
							if( sign && ( notation == 'x' || notation == 'X' ) )
								DaoProcess_RaiseWarning( proc, NULL, "Signed hexadecimal numeric format!" );
							prevpos++;
						}
						if( prevpos != pos && mbs[prevpos] == '.' ){
							if( prevpos == pos - 1 || mbs[prevpos + 1] == '<'
									|| mbs[prevpos + 1] == '>' ){
								DaoProcess_RaiseWarning( proc, NULL, "Empty precision specifier!" );
								continue;
							}
							for( prevpos++; isdigit( mbs[prevpos] ); prevpos++ )
								precision = precision*10 + ( mbs[prevpos] - '0' );
							if( precision > 1000 || precision < 0 ){
								DaoProcess_RaiseWarning( proc, NULL,
										"Precision too large (greater than 1000)" );
								precision = 0;
							}
						}
						prevpos--;
						if( ( sign || precision ) && !notation )
							DaoProcess_RaiseWarning( proc, NULL, "Incomplete numeric format!" );
					}
					else{
						sprintf( buf, "Invalid character in placeholder ('%c')!", mbs[prevpos] );
						DaoProcess_RaiseWarning( proc, NULL, buf );
						error = 1;
						break;
					}
				pos--;
				if( error ){
					DString_AppendChar( str, '?' );
					continue;
				}
			}
			else{
				if( !isdigit( mbs[pos + 1] ) ){
					sprintf( buf, "Invalid placeholder index ('%c')!", mbs[pos + 1] );
					DaoProcess_RaiseWarning( proc, NULL, buf );
					DString_AppendChar( str, '?' );
					continue;
				}
				num = mbs[pos + 1] - '0';
			}
			if( argname ){
				int i;
				for( i = 1; i < N; i++ )
					if( p[i]->type == DAO_TUPLE && IsNamedValue( &p[i]->xTuple ) ){
						DString *sname = DString_New();
						DaoEnum_MakeName( &p[i]->xTuple.values[0]->xEnum, sname );
						if( DString_Size( sname ) - 1 == argnamelen &&
								!strncmp( DString_GetData( sname ) + 1, argname, argnamelen ) ){
							value = p[i]->xTuple.values[1];
							break;
						}
						DString_Delete( sname );
					}
				if( !value ){
					DString *sname = DString_New( 1 );
					DString_SetBytes( sname, argname, argnamelen );
					sprintf( buf, "No argument matching placeholder name ('%s')!", DString_GetData( sname ) );
					DString_Delete( sname );
					DaoProcess_RaiseWarning( proc, NULL, buf );
					DString_AppendChar( str, '?' );
					continue;
				}
			}
			else if( num > N - 2 || num < 0 ){
				sprintf( buf, "Placeholder index too large (%i)!", num );
				DaoProcess_RaiseWarning( proc, NULL, buf );
				DString_AppendChar( str, '?' );
				continue;
			}
			format.alignment = alignment;
			format.centered = centered;
			format.notation = notation;
			format.sign = sign;
			format.precision = precision;
			format.indexed = indexed;
			format.sliced = sliced;
			format.index = index;
			format.index2 = index2;
			format.name = name;
			format.namelen = namelen;
			if( value )
				error = PrintValue( value, str, &format, tmp, buf );
			else if( DaoValue_Type( p[num + 1] ) == DAO_TUPLE && IsNamedValue( &p[num + 1]->xTuple ) )
				error = PrintValue( p[num + 1]->xTuple.values[1], str, &format, tmp, buf );
			else
				error = PrintValue( p[num + 1], str, &format, tmp, buf );
			switch( error ){
			case 1: sprintf( buf, "Unsupported argument type (argument %i)!", num ); break;
			case 2: sprintf( buf, "Unsupported element type (argument %i)!", num ); break;
			case 3: sprintf( buf, "Conflicting numeric format and argument type (argument %i)!", num ); break;
			case 4: sprintf( buf, "Conflicting numeric format and element type (argument %i)!", num ); break;
			case 5: sprintf( buf, "Conflicting indexing and argument type (argument %i)!", num ); break;
			case 6: sprintf( buf, "Index out of range (argument %i, index %i)!", num, index ); break;
			case 7: sprintf( buf, "Named field for argument which is not a tuple (argument %i)!", num ); break;
			case 8: sprintf( buf, "Named field does not exist (argument %i)!", num ); break;
			case 9: sprintf( buf, "Slice out of range (argument %i, slice %i : %i)!", num, index, index2 ); break;
			case 10: sprintf( buf, "Invalid slice (argument %i, slice %i : %i)!", num, index, index2 ); break;
			case 11: sprintf( buf, "Conflicting slicing and argument type (argument %i)!", num ); break;
			default: break;
			}
			if( error ){
				DaoProcess_RaiseWarning( proc, NULL, buf );
				if( error != 3 && error != 4 )
					DString_AppendChar( str, '?' );
			}
			DString_Clear( tmp );
		}
	}
	DString_Delete( tmp );
	DaoProcess_PutString( proc, str );
	DString_Delete( str );
}

static DaoFunctionEntry formatMeths[] =
{
	/*! Returns the string constructed from \a template and specified arguments. For the description of the grammar and possible use,
	 * see *format.dao* */
	{ DaoFormat,	"format(template: string, ...) => string" },
	{ NULL, NULL }
};

DAO_DLL int DaoFormat_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *strns = DaoVmSpace_GetNamespace( vmSpace, "str" );
	DaoNamespace_AddConstValue( ns, "str", (DaoValue*)strns );
	DaoNamespace_WrapFunctions( strns, formatMeths );
	return 0;
}
