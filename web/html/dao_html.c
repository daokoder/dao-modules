/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2014, Limin Fu
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

// 2014-08: Danilov Aleksey, initial implementation.

#include <string.h>
#include <ctype.h>
#include "dao.h"
#include "daoString.h"
#include "daoGC.h"
#include "daoValue.h"
#include "daoStdtype.h"
#include "daoProcess.h"
#include "daoVmspace.h"

typedef struct DaoHtmlContext DaoHtmlContext;

struct DaoHtmlContext
{
	DList *content;
	daoint size, indent;
	int pending;
};

DaoHtmlContext* DaoHtmlContext_New()
{
	DaoHtmlContext *res = (DaoHtmlContext*)dao_malloc( sizeof(DaoHtmlContext) );
	res->content = DList_New( DAO_DATA_STRING );
	res->size = res->indent = 0;
	res->pending = 0;
	return res;
}

void DaoHtmlContext_Delete( DaoHtmlContext *self )
{
	if ( self == NULL )
		return;
	DList_Delete( self->content );
	dao_free( self );
}

void SetContext( DaoProcess *proc, DaoHtmlContext *ctx )
{
	DaoProcess_SetAuxData( proc, DaoHtmlContext_Delete, ctx );
}

DaoHtmlContext* GetContext( DaoProcess *proc )
{
	return (DaoHtmlContext*)DaoProcess_GetAuxData( proc, DaoHtmlContext_Delete );
}

void ClearContext( DaoProcess *proc )
{
	DaoProcess_SetAuxData( proc, DaoHtmlContext_Delete, NULL );
}

void WriteAttrName( DaoEnum *en, DString *dest )
{
	char *nc;
	DString *name = DString_New();
	DaoEnum_MakeName( en, name );
	nc = name->chars + 1;
	if ( strcmp( nc, "xml_lang" ) == 0 )
		DString_AppendChars( dest, "xml:lang" );
	else {
		if ( *nc == '_' )
			nc++;
		for ( ; *nc != '\0'; nc++ )
			if ( *nc == '_' )
				DString_AppendChar( dest, '-' );
			else
				DString_AppendChar( dest, *nc );
	}
	DString_Delete( name );
}

void WriteAttrValue( DaoValue *value, DString *dest )
{
	DString *buf = DString_New();
	DString_AppendChar( dest, '"' );
	switch ( value->type ){
	case DAO_STRING:
		DString_Append( dest, value->xString.value );
		break;
	case DAO_INTEGER:
	case DAO_FLOAT:
	case DAO_BOOLEAN:
		DaoValue_GetString( value, buf );
		DString_Append( dest, buf );
		break;
	case DAO_ENUM:
		if ( 1 ){
			char *ch;
			DaoEnum_MakeName( &value->xEnum, buf );
			if ( strcmp( buf->chars + 1, "blank" ) == 0 || strcmp( buf->chars + 1, "self" ) == 0 ||
				 strcmp( buf->chars + 1, "parent" ) == 0 || strcmp( buf->chars + 1, "top" ) == 0 ){ // target & formtarget
				buf->chars[0] = '_';
				DString_Append( dest, buf );
			}
			else for ( ch = buf->chars + 1; *ch != '\0'; ch++ )
				if ( *ch == '_' )
					DString_AppendChar( dest, '-' );
				else if ( *ch == '$' )
					DString_AppendChar( dest, ' ' );
				else
					DString_AppendChar( dest, *ch );
		}
		break;
	case DAO_TUPLE: // coords
		if ( 1 ){
			int i;
			for ( i = 0; i < value->xTuple.size; i++ ){
				DaoValue_GetString( value->xTuple.values[i], buf );
				DString_Append( dest, buf );
				if ( i < value->xTuple.size - 1 )
					DString_AppendChar( dest, ',' );
			}
		}
		break;
	case DAO_LIST: // headers
		if ( 1 ){
			int i;
			for ( i = 0; i < DaoList_Size( &value->xList ); i++ ){
				DString_Append( dest, DaoList_GetItem( &value->xList, i )->xString.value );
				if ( i < DaoList_Size( &value->xList ) - 1 )
					DString_AppendChar( dest, ' ' );
			}
		}
		break;
	}
	DString_AppendChar( dest, '"' );
	DString_Delete( buf );
}

void WriteAttrs( DaoValue *attrs[], int N, DString *dest ){
	DString *name = DString_New();
	DaoValue *value;
	int i;
	for ( i = 0; i < N; i++ )
		switch ( attrs[i]->type ){
		case DAO_TUPLE: // attribute with a value
			value = attrs[i]->xTuple.values[1];
			DString_AppendChar( dest, ' ' );
			if ( value->type == DAO_TUPLE && value->xTuple.size && value->xTuple.values[0]->type == DAO_ENUM ){ // data-*
				daoint i;
				DString_AppendChars( dest, "data-" );
				DaoEnum_MakeName( &value->xTuple.values[0]->xEnum, name );
				for ( i = 1; i < name->size; i++ )
					if ( name->chars[i] == '_' && i != 1 && i < name->size - 1 )
						name->chars[i] = '-';
				DString_AppendChars( dest, name->chars + 1 );
				value = value->xTuple.values[1];
			}
			else
				WriteAttrName( &attrs[i]->xTuple.values[0]->xEnum, dest );
			DString_AppendChar( dest, '=' );
			WriteAttrValue( value, dest );
			break;
		case DAO_ENUM: // void attribute
			DaoEnum_MakeName( &attrs[i]->xEnum, name );
			DString_AppendChar( dest, ' ' );
			DString_AppendChars( dest, name->chars + 1 );
			break;
		}
	DString_Delete( name );
}

void DaoHtmlContext_AddContent( DaoHtmlContext *self, DString *content, int pending )
{
	DList_Append( self->content, content );
	self->size += content->size;
	self->pending = pending;
}

void DaoHtmlContext_NewLine( DaoHtmlContext *self )
{
	DString *line = DString_NewChars( "\n" );
	int i;
	for ( i = 0; i < self->indent; i++ )
		DString_AppendChars( line, "  " );
	DList_Append( self->content, line );
	self->size += line->size;
	self->pending = 1;
	DString_Delete( line );
}

void DaoHtmlContext_Fold( DaoHtmlContext *self, DString *dest )
{
	DString_Reserve( dest, dest->size + self->size );
	if ( self->content->size ){
		int i;
		char *line = self->content->items.pString[0]->chars;
		if ( *line == '\n' )
			line++;
		DString_AppendChars( dest, line );
		for ( i = 1; i < self->content->size; i++ )
			DString_Append( dest, self->content->items.pString[i] );
		DString_AppendChar( dest, '\n' );
	}
}

static void Html_EmptyTag( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoValue **attrs = p;
	DString *tag = DString_New();
	DString *name = proc->topFrame->routine->routName;
	DaoHtmlContext *ctx = GetContext( proc );
	if ( !ctx )
		DaoProcess_RaiseError( proc, "HTML", "Tag out of context (must be inside document(){} or fragment(){} and in the same task)" );
	else {
		DaoHtmlContext_NewLine( ctx );
		DString_AppendChars( tag, "<" );
		DString_Append( tag, name );
		if ( strcmp( name->chars, "command" ) == 0 || strcmp( name->chars, "input" ) == 0 ){
			DString_AppendChars( tag, " type=" );
			WriteAttrValue( p[0], tag );
			attrs++;
			N--;
		}
		WriteAttrs( attrs, N, tag );
		DString_AppendChars( tag, ">" );
		DaoHtmlContext_AddContent( ctx, tag, 0 );
	}
	DString_Delete( tag );
}

static void Html_Tag( DaoProcess *proc, DaoValue *p[], int N )
{
	const char* rnames[] = {"anchor", "bold", "italic", "paragraph", "quoted", "struck", "underlined", "variable"};
	const char* tnames[] = {"a", "b", "i", "p", "q", "s", "u", "var"};
	const char* phnames[] = {"em", "strong", "small", "mark", "abbr", "dfn", "i", "b", "s", "u", "var", "samp", "kbd", "sup", "q",
							 "cite", "span", "bdo", "bdi", "ins", "del"};
	DaoValue **attrs = p;
	DaoHtmlContext *ctx = GetContext( proc );
	char *name = proc->topFrame->routine->routName->chars;
	int i, phrazing = 0;
	for ( i = 0; i < sizeof(rnames)/sizeof(char*); i++ )
		if ( strcmp( name, rnames[i] ) == 0 ){
			name = (char*)tnames[i];
			break;
		}
	for ( i = 0; i < sizeof(phnames)/sizeof(char*); i++ )
		if ( strcmp( name, phnames[i] ) == 0 ){
			phrazing = 1;
			break;
		}
	if ( !ctx )
		DaoProcess_RaiseError( proc, "HTML", "Tag out of context (must be inside document(){} or fragment(){} and in the same task)" );
	else {
		DString *tag = DString_New();
		DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
		if ( !phrazing || !ctx->pending )
			DaoHtmlContext_NewLine( ctx );
		DString_AppendChar( tag, '<' );
		DString_AppendChars( tag, name );
		if ( strcmp( name, "button" ) == 0 ){
			DString_AppendChars( tag, " type=" );
			WriteAttrValue( p[0], tag );
			attrs++;
			N--;

		}
		WriteAttrs( attrs, N, tag );
		DString_AppendChar( tag, '>' );
		DaoHtmlContext_AddContent( ctx, tag, phrazing );
		if ( !phrazing )
			ctx->indent++;
		if ( sect ){
			DaoValue *value;
			DaoProcess_Execute( proc );
			value = proc->stackValues[0];
			if ( value && value->type != DAO_NONE ){
				if ( !ctx->pending )
					DaoHtmlContext_NewLine( ctx );
				DaoHtmlContext_AddContent( ctx, value->xString.value, 1 );
			}
			DaoProcess_PopFrame( proc );
		}
		DString_Clear( tag );
		if ( !phrazing ){
			ctx->indent--;
			DaoHtmlContext_NewLine( ctx );
		}
		DString_AppendChars( tag, "</" );
		DString_AppendChars( tag, name );
		DString_AppendChars( tag, ">" );
		DaoHtmlContext_AddContent( ctx, tag, phrazing );
		DString_Delete( tag );
	}
}

static void Html_Comment( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoHtmlContext *ctx = GetContext( proc );
	if ( !ctx )
		DaoProcess_RaiseError( proc, "HTML", "Comment out of context (must be inside document(){} or fragment(){} and in the same task)" );
	else {
		DString *comm = DString_NewChars( "<!-- " );
		DString_Append( comm, p[0]->xString.value );
		DString_AppendChars( comm, " -->" );
		DaoHtmlContext_NewLine( ctx );
		DaoHtmlContext_AddContent( ctx, comm, 0 );
		DString_Delete( comm );
	}
}

static void Html_Text( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoHtmlContext *ctx = GetContext( proc );
	if ( !ctx )
		DaoProcess_RaiseError( proc, "HTML", "Text out of context (must be inside document(){} or fragment(){} and in the same task)" );
	else {
		if ( !ctx->pending )
			DaoHtmlContext_NewLine( ctx );
		DaoHtmlContext_AddContent( ctx, p[0]->xString.value, 1 );
	}
}

static void Html_Document( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoHtmlContext *ctx = DaoHtmlContext_New();
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
	DString *res;
	if ( GetContext( proc ) ){
		DaoProcess_RaiseError( proc, "HTML", "Nested contexts not allowed (nested document(){} or fragment(){} sections)" );
		DaoHtmlContext_Delete( ctx );
		return;
	}
	res = DaoProcess_PutChars( proc, "<!DOCTYPE html>\n<html" );
	WriteAttrs( p, N, res );
	DString_AppendChars( res, ">\n" );
	SetContext( proc, ctx );
	if ( sect ){
		DaoProcess_Execute( proc );
		DaoProcess_PopFrame( proc );
	}
	DaoHtmlContext_Fold( ctx, res );
	ClearContext( proc );
	DString_AppendChars( res, "</html>\n" );
}

static void Html_Fragment( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoHtmlContext *ctx = DaoHtmlContext_New();
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 0 );
	DString *res;
	if ( GetContext( proc ) ){
		DaoProcess_RaiseError( proc, "HTML", "Nested contexts not allowed (nested document(){} or fragment(){} sections)" );
		DaoHtmlContext_Delete( ctx );
		return;
	}
	res = DaoProcess_PutChars( proc, "" );
	SetContext( proc, ctx );
	if ( sect ){
		DaoProcess_Execute( proc );
		DaoProcess_PopFrame( proc );
	}
	DaoHtmlContext_Fold( ctx, res );
	ClearContext( proc );
}

const char *html_global_attr =
	"tuple<enum<accesskey,_class,contextmenu,id,lang,style,title>, string> | tuple<enum<dropzone>, enum<copy;move;link>|string> | "
	"tuple<enum<width,height>, string> | "
	"tuple<enum<data_>, tuple<enum, string>> | tuple<enum<dir>, enum<ltr,rtl,auto>> | enum<contenteditable,hidden,spellcheck> | "
	"tuple<enum<contenteditable,draggable,spellcheck>, int> | tuple<enum<tabindex>, int> | "
	"tuple<enum<translate>, enum<yes,no>> | "
	"tuple<enum<onabort,onblur,oncancel,oncanplay,oncanplaythrough,onchange,onclick,onclose,oncontextmenu,oncuechange,"
			   "ondblclick,ondrag,ondragend,ondragenter,ondragleave,ondragover,ondragstart,ondrop,ondurationchange,onemptied,"
			   "onended,onerror,onfocus,oninput,oninvalid,onkeydown,onkeypress,onkeyup,onload,onloadeddata,onloadedmetadata,"
			   "onloadstart,onmousedown,onmousemove,onmouseout,onmouseover,onmouseup,onmousewheel,onpause,onplay,onplaying,"
			   "onprogress,onratechange,onreset,onscroll,onseeked,onseeking,onselect,onshow,onstalled,onsubmit,onsuspend"
			   "ontimeupdate,onvolumechange,onwaiting>, string>";

const char *html_step_attr =
	"tuple<enum<step>, float|enum<any>>";

const char *html_target_attr =
	"tuple<enum<target>, enum<blank,self,parent,top>|string>";

const char *html_formtarget_attr =
	"tuple<enum<formtarget>, enum<blank,self,parent,top>|string>";

static DaoFunctionEntry htmlMeths[] =
{
	/*! Returns HTML 5 document composed from content specified in the code section. Includes '<!DOCTYPE html>' and 'html' tag with
	 * the specified attributes */
	{ Html_Document,	"document(...: tuple<enum<manifest,xml_lang>, string> | GlobalAttr)[] => string" },

	/*! Returns HTML 5 code fragment composed from content specified in the code section. */
	{ Html_Fragment,	"fragment()[] => string" },

	/*! Specifies plain \a text, but may as well contain arbitrary HTML markup (\a text is included 'as-is' into the resulting
	 * HTML code) */
	{ Html_Text,		"text(text: string)" },

	/*! Specifies comment with the given \a text */
	{ Html_Comment,		"comment(text: string)" },

	/*! Each tag routine corresponds to individual HTML element. Code section routine indicates element which may have content, i.e.
	 * nested tags, text or comments. Tag routines, \c text() and \c comment() called within such code sections are automatically nested
	 * inside the corresponding parent tag in the resulting HTML code. To avoid unnecessary text() calls, all code sections support
	 * \c string as returned value to be used for elements whose content can be specified as a single \c string value.
	 *
	 * \note For elements which wrap short text fragments ('em', 'q', 'span', etc.) it is often better to directly use HTML markup
	 * within strings -- it may appear more concise and clear in certain cases. Strings which are code section results and \c text()
	 * arguments may include arbitrary HTML code.
	 *
	 * Element attributes may be specified via variadic parameters, each of which is either a named value (attribute with a value)
	 * or \c enum (void attribute). All tag routines support parameters of type `GlobalAttr`, which covers all global HTML attributes.
	 *
	 * Routine names mostly match those of HTML tags; however, there are few exceptions made to avoid naming conflicts:
	 * - 'a' -> 'anchor';
	 * - 'b' -> 'bold';
	 * - 'i' -> 'italic';
	 * - 'p' -> 'paragraph';
	 * - 'q' -> 'qouted';
	 * - 's' -> 'struck';
	 * - 'u' -> 'underlined';
	 * - 'var' -> 'variable'.
	 *
	 * That is, all single-letter tags (plus 'var') are replaced with the corresponding full words.
	 *
	 * Attribute names also mostly match HTML specification; the exceptions are:
	 * - 'type', 'class' and 'for' start with '_' as they are Dao keywords;
	 * - the names originally containing '-' and ':' ('xml:lang') use '_' instead;
	 * - 'data-*' attributes are provided via \c data_ parameter accepting name-value tuple.
	 *
	 * \note For \c data_ parameter, any '_' in its name suffix is replaced with '-'.
	 *
	 * Additionally, \c button(), \c command() and \c input() do not accept 'type' attribute as named value, instead expecting single
	 * \c enum value as the first, non-variadic routine argument. This allows to provide overloaded version of these routines with
	 * different parameter sets similar to HTML specification for these tags. */
	{ Html_Tag,			"anchor(...: tuple<enum<href,hreflang,media,_type>, string> | Target | "
									"tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;"
														  "prev;search;tag>> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"abbrev(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"address(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"area(...: tuple<enum<alt,href,media,hreflang,_type>, string> | Target | "
								  "tuple<enum<shape>, enum<rect,circle,poly,default>> | tuple<enum<coords>, tuple<...: int>> | "
								  "tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;"
														"prev;search;tag>> | GlobalAttr)" },
	{ Html_Tag,			"article(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"aside(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"audio(...: enum<autoplay,preload,controls,loop,muted> | tuple<enum<preload>, enum<none,metadata,auto>> | "
								   "tuple<enum<mediagroup,src>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"base(...: tuple<enum<href>, string> | Target | GlobalAttr)" },
	{ Html_Tag,			"bdi(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"bdo(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"blockquote(...: tuple<enum<cite>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"body(...: tuple<enum<onafterprint,onbeforeprint,onbeforeunload,onhashchange,onmessage,onoffline,ononline,"
											 "onpagehide,onpageshow,onpopstate,onresize,onstorage,onunload>, string> | "
								  "GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"bold(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"br(...: GlobalAttr)" },
	{ Html_Tag,			"button(kind: enum<submit>, ...: "
							   "tuple<enum<name,form,value,formaction,formenctype>, string> | FormTarget |  "
							   "enum<disabled,autofocus,formnovalidate> | tuple<enum<formmethod>, enum<get,post>> | GlobalAttr)"
							   "[ => string|none ]" },
	{ Html_Tag,			"button(kind: enum<reset,button>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
															  "GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"canvas(...: tuple<enum<height,width>, int> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"caption(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"cite(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"code(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"col(...: tuple<enum<span>, int> | GlobalAttr)" },
	{ Html_Tag,			"colgroup(...: tuple<enum<span>, int> | GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"command(kind: enum<command>, ...: tuple<enum<label,icon>, string> | enum<disabled> | GlobalAttr)" },
	{ Html_EmptyTag,	"command(kind: enum<radio>, ...: tuple<enum<radiogroup,icon,label>, string> | enum<checked,disabled> | "
														"GlobalAttr)" },
	{ Html_EmptyTag,	"command(kind: enum<checkbox>, ...: tuple<enum<label,icon>, string> | enum<checked,disabled> | "
														   "GlobalAttr)" },
	{ Html_Tag,			"datalist(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"dd(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"del(...: tuple<enum<cite,datetime>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"details(...: enum<open> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"dfn(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"div(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"dl(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"dt(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"em(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"embed(...: tuple<enum<src,_type>, string> | tuple<enum<height,width>, int> | GlobalAttr)" },
	{ Html_Tag,			"fieldset(...: tuple<enum<name,form>, string> | enum<disabled> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"figcaption(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"figure(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"footer(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"form(...: tuple<enum<action,enctype,name,accept_charset>, string> | Target | enum<novalidate> | "
								  "tuple<enum<method>, enum<get,post>> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)"
							"[ => string|none ]" },
	{ Html_Tag,			"h1(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"h2(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"h3(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"h4(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"h5(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"h6(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"head(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"header(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"hgroup(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"hr(...: GlobalAttr)" },
	{ Html_Tag,			"iframe(...: tuple<enum<src,srcdoc,name>, string> | tuple<enum<height,width>, int> | "
									"tuple<enum<sandbox>, enum<allow_forms;allow_scripts;allow_top_navigation;allow_same_origin>> | "
									"enum<sandbox,seamless> | GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"img(...: tuple<enum<src,alt,usemap>, string> | tuple<enum<height,width>, int> | enum<ismap> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<text,search>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder,dirname>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<password>, ...: "
							  "tuple<enum<name,form,value,placeholder>, string> | tuple<enum<maxlength,size>, int> | "
							  "enum<disabled,readonly,autofocus,required> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<checkbox,radio>, ...: tuple<enum<name,form,value>, string> | "
															   "enum<checked,disabled,autofocus,required> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<button,reset>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
															 "GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<submit>, ...: "
							  "tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
							  "tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | FormTarget | "
							  "tuple<enum<formmethod>, enum<get,post>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<file>, ...: tuple<enum<name,form,accept>, string> | "
													 "enum<disabled,autofocus,required,multiple> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<hidden>, ...: tuple<enum<name,form,value>, string> | enum<disabled> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<image>, ...: "
							  "tuple<enum<name,form,alt,src>, string> | enum<disabled,autofocus> | "
							  "tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | FormTarget | "
							  "tuple<enum<formmethod>, enum<get,post>> | tuple<enum<height,width>, int> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<datetime,datetime_local,date,month,time,week>, ...: "
							  "tuple<enum<name,form,value,list,min,max>, string> | enum<disabled,autofocus,readonly,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | Step | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<number>, ...: "
							  "tuple<enum<name,form,value,list,placeholder>, string> | tuple<enum<min,max>, float> | Step | "
							  "enum<disabled,autofocus,readonly,required> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<range>, ...: "
							  "tuple<enum<name,form,value,list>, string> | tuple<enum<min,max>, float> | enum<disabled,autofocus> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | Step | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<email>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<url,tel>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<color>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder,dir>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_EmptyTag,	"input(kind: enum<color>, ...: tuple<enum<name,form,list,value>, string> | enum<disabled,autofocus> | "
													  "tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)" },
	{ Html_Tag,			"ins(...: tuple<enum<cite,datetime>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"italic(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"kbd(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"keygen(...: tuple<enum<challenge,name,form>, string> | tuple<enum<keytype>, enum<rsa>> | "
									"enum<autofocus,disabled> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"label(...: tuple<enum<id,_for,form>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"legend(...: tuple<enum<value>, int> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"li(...: tuple<enum<value>, int> | GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"link(...: tuple<enum<href,hreflang,media,_type,sizes>, string> | "
								  "tuple<enum<rel>, enum<alternate;author;help;icon;license;next;prefetch;prev;search;stylesheet>> | "
								  "GlobalAttr)" },
	{ Html_Tag,			"map(...: tuple<enum<name>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"mark(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"menu(kind: enum<toolbar,context>, ...: tuple<enum<label>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<name,content>, string> | GlobalAttr)" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<http_equiv>, enum<refresh,default_style,content_type>> | tuple<enum<content>, string> | "
								  "GlobalAttr)" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<charset>, string> | GlobalAttr)" },
	{ Html_Tag, 		"meter(...: tuple<enum<value,min,low,high,max,optimum>, float> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"nav(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"noscript(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"object(...: tuple<enum<data,_type,usemap,name,form>, string> | tuple<enum<height,width>, int> | "
									"GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"ol(...: tuple<enum<_type>, string> | tuple<enum<start>, int> | enum<reversed> | GlobalAttr)"
						  "[ => string|none ]" },
	{ Html_Tag,			"optgroup(...: tuple<enum<label>, string> | enum<disabled> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"option(...: tuple<enum<label,value>, string> | enum<disabled,selected> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"output(...: tuple<enum<name,form,_for>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"paragraph(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"param(...: tuple<enum<name,value>, string> | GlobalAttr)" },
	{ Html_Tag,			"pre(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"progress(...: tuple<enum<value,max>, float> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"quoted(...: tuple<enum<cite>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"rp(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"ruby(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"struck(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"samp(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"script(...: tuple<enum<_type,src,charset>, string> | enum<defer,async> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"section(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"select(...: tuple<enum<name,form>, string> | tuple<enum<size>, int> | "
									"enum<disabled,multiple,autofocus,required> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"small(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"source(...: tuple<enum<_type,src,media>, string> | GlobalAttr)" },
	{ Html_Tag,			"span(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"strong(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"style(...: tuple<enum<_type,media>, string> | enum<scoped> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"sub(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"summary(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"sup(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"table(...: tuple<enum<border>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"tbody(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"td(...: tuple<enum<colspan,rowspan>, int> | tuple<enum<headers>, invar<list<string>>> | GlobalAttr)"
						  "[ => string|none ]" },
	{ Html_Tag,			"textarea(...: tuple<enum<name,form,placeholder,dirname>, string> | "
									  "enum<disabled,readonly,autofocus,required> | tuple<enum<maxlength,rows,cols>, int> | "
									  "tuple<enum<wrap>, enum<hard,soft>> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"tfoot(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"th(...: tuple<enum<scope>, enum<row,col,rowgroup,colgroup>> | tuple<enum<colspan,rowspan>, int> | "
								"tuple<enum<headers>, invar<list<string>>> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"thead(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"time(...: tuple<enum<datetime>, string> | GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"title(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"tr(...: GlobalAttr)[ => string|none ]" },
	{ Html_EmptyTag,	"track(...: tuple<enum<kind>, enum<subtitles,captions,descriptions,chapters,metadata>> | "
								   "tuple<enum<src,srclang,label>, string> | enum<default> | GlobalAttr)" },
	{ Html_Tag,			"underlined(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"ul(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"variable(...: GlobalAttr)[ => string|none ]" },
	{ Html_Tag,			"video(...: tuple<enum<preload>, enum<none,metadata,auto>> | enum<autoplay,controls,loop,muted> | "
								   "tuple<enum<poster,mediagroup,src>, string> | tuple<enum<height,width>, int> | GlobalAttr)"
							 "[ => string|none ]" },
	{ Html_EmptyTag,	"wbr(...: GlobalAttr)" },
	{ NULL, NULL }
};



#include "gumbo.h"

typedef struct DaoxHtmlTag DaoxHtmlTag;

struct DaoxHtmlTag
{
	const char *name;
	short       tag;
	short       ctx;
};

DaoxHtmlTag html_tags[] =
{
	{ "html",           GUMBO_TAG_HTML,            GUMBO_TAG_LAST },
	{ "head",           GUMBO_TAG_HEAD,            GUMBO_TAG_LAST },
	{ "title",          GUMBO_TAG_TITLE,           GUMBO_TAG_LAST },
	{ "base",           GUMBO_TAG_BASE,            GUMBO_TAG_LAST },
	{ "link",           GUMBO_TAG_LINK,            GUMBO_TAG_LAST },
	{ "meta",           GUMBO_TAG_META,            GUMBO_TAG_LAST },
	{ "style",          GUMBO_TAG_STYLE,           GUMBO_TAG_LAST },
	{ "script",         GUMBO_TAG_SCRIPT,          GUMBO_TAG_LAST },
	{ "noscript",       GUMBO_TAG_NOSCRIPT,        GUMBO_TAG_LAST },
	{ "template",       GUMBO_TAG_TEMPLATE,        GUMBO_TAG_LAST },
	{ "body",           GUMBO_TAG_BODY,            GUMBO_TAG_LAST },
	{ "article",        GUMBO_TAG_ARTICLE,         GUMBO_TAG_LAST },
	{ "section",        GUMBO_TAG_SECTION,         GUMBO_TAG_LAST },
	{ "nav",            GUMBO_TAG_NAV,             GUMBO_TAG_LAST },
	{ "aside",          GUMBO_TAG_ASIDE,           GUMBO_TAG_LAST },
	{ "h1",             GUMBO_TAG_H1,              GUMBO_TAG_LAST },
	{ "h2",             GUMBO_TAG_H2,              GUMBO_TAG_LAST },
	{ "h3",             GUMBO_TAG_H3,              GUMBO_TAG_LAST },
	{ "h4",             GUMBO_TAG_H4,              GUMBO_TAG_LAST },
	{ "h5",             GUMBO_TAG_H5,              GUMBO_TAG_LAST },
	{ "h6",             GUMBO_TAG_H6,              GUMBO_TAG_LAST },
	{ "hgroup",         GUMBO_TAG_HGROUP,          GUMBO_TAG_LAST },
	{ "header",         GUMBO_TAG_HEADER,          GUMBO_TAG_LAST },
	{ "footer",         GUMBO_TAG_FOOTER,          GUMBO_TAG_LAST },
	{ "address",        GUMBO_TAG_ADDRESS,         GUMBO_TAG_LAST },
	{ "p",              GUMBO_TAG_P,               GUMBO_TAG_LAST },
	{ "hr",             GUMBO_TAG_HR,              GUMBO_TAG_LAST },
	{ "pre",            GUMBO_TAG_PRE,             GUMBO_TAG_LAST },
	{ "blockquote",     GUMBO_TAG_BLOCKQUOTE,      GUMBO_TAG_LAST },
	{ "ol",             GUMBO_TAG_OL,              GUMBO_TAG_LAST },
	{ "ul",             GUMBO_TAG_UL,              GUMBO_TAG_LAST },
	{ "li",             GUMBO_TAG_LI,              GUMBO_TAG_OL },
	{ "dl",             GUMBO_TAG_DL,              GUMBO_TAG_LAST },
	{ "dt",             GUMBO_TAG_DT,              GUMBO_TAG_LAST },
	{ "dd",             GUMBO_TAG_DD,              GUMBO_TAG_LAST },
	{ "figure",         GUMBO_TAG_FIGURE,          GUMBO_TAG_LAST },
	{ "figcaption",     GUMBO_TAG_FIGCAPTION,      GUMBO_TAG_LAST },
	{ "main",           GUMBO_TAG_MAIN,            GUMBO_TAG_LAST },
	{ "div",            GUMBO_TAG_DIV,             GUMBO_TAG_LAST },
	{ "a",              GUMBO_TAG_A,               GUMBO_TAG_LAST },
	{ "em",             GUMBO_TAG_EM,              GUMBO_TAG_LAST },
	{ "strong",         GUMBO_TAG_STRONG,          GUMBO_TAG_LAST },
	{ "small",          GUMBO_TAG_SMALL,           GUMBO_TAG_LAST },
	{ "s",              GUMBO_TAG_S,               GUMBO_TAG_LAST },
	{ "cite",           GUMBO_TAG_CITE,            GUMBO_TAG_LAST },
	{ "q",              GUMBO_TAG_Q,               GUMBO_TAG_LAST },
	{ "dfn",            GUMBO_TAG_DFN,             GUMBO_TAG_LAST },
	{ "abbr",           GUMBO_TAG_ABBR,            GUMBO_TAG_LAST },
	{ "data",           GUMBO_TAG_DATA,            GUMBO_TAG_LAST },
	{ "time",           GUMBO_TAG_TIME,            GUMBO_TAG_LAST },
	{ "code",           GUMBO_TAG_CODE,            GUMBO_TAG_LAST },
	{ "var",            GUMBO_TAG_VAR,             GUMBO_TAG_LAST },
	{ "samp",           GUMBO_TAG_SAMP,            GUMBO_TAG_LAST },
	{ "kbd",            GUMBO_TAG_KBD,             GUMBO_TAG_LAST },
	{ "sub",            GUMBO_TAG_SUB,             GUMBO_TAG_LAST },
	{ "sup",            GUMBO_TAG_SUP,             GUMBO_TAG_LAST },
	{ "i",              GUMBO_TAG_I,               GUMBO_TAG_LAST },
	{ "b",              GUMBO_TAG_B,               GUMBO_TAG_LAST },
	{ "u",              GUMBO_TAG_U,               GUMBO_TAG_LAST },
	{ "mark",           GUMBO_TAG_MARK,            GUMBO_TAG_LAST },
	{ "ruby",           GUMBO_TAG_RUBY,            GUMBO_TAG_LAST },
	{ "rt",             GUMBO_TAG_RT,              GUMBO_TAG_LAST },
	{ "rp",             GUMBO_TAG_RP,              GUMBO_TAG_LAST },
	{ "bdi",            GUMBO_TAG_BDI,             GUMBO_TAG_LAST },
	{ "bdo",            GUMBO_TAG_BDO,             GUMBO_TAG_LAST },
	{ "span",           GUMBO_TAG_SPAN,            GUMBO_TAG_LAST },
	{ "br",             GUMBO_TAG_BR,              GUMBO_TAG_LAST },
	{ "wbr",            GUMBO_TAG_WBR,             GUMBO_TAG_LAST },
	{ "ins",            GUMBO_TAG_INS,             GUMBO_TAG_LAST },
	{ "del",            GUMBO_TAG_DEL,             GUMBO_TAG_LAST },
	{ "image",          GUMBO_TAG_IMAGE,           GUMBO_TAG_LAST },
	{ "img",            GUMBO_TAG_IMG,             GUMBO_TAG_LAST },
	{ "iframe",         GUMBO_TAG_IFRAME,          GUMBO_TAG_LAST },
	{ "embed",          GUMBO_TAG_EMBED,           GUMBO_TAG_LAST },
	{ "object",         GUMBO_TAG_OBJECT,          GUMBO_TAG_LAST },
	{ "param",          GUMBO_TAG_PARAM,           GUMBO_TAG_LAST },
	{ "video",          GUMBO_TAG_VIDEO,           GUMBO_TAG_LAST },
	{ "audio",          GUMBO_TAG_AUDIO,           GUMBO_TAG_LAST },
	{ "source",         GUMBO_TAG_SOURCE,          GUMBO_TAG_LAST },
	{ "track",          GUMBO_TAG_TRACK,           GUMBO_TAG_LAST },
	{ "canvas",         GUMBO_TAG_CANVAS,          GUMBO_TAG_LAST },
	{ "map",            GUMBO_TAG_MAP,             GUMBO_TAG_LAST },
	{ "area",           GUMBO_TAG_AREA,            GUMBO_TAG_LAST },
	{ "math",           GUMBO_TAG_MATH,            GUMBO_TAG_LAST },
	{ "mi",             GUMBO_TAG_MI,              GUMBO_TAG_LAST },
	{ "mo",             GUMBO_TAG_MO,              GUMBO_TAG_LAST },
	{ "mn",             GUMBO_TAG_MN,              GUMBO_TAG_LAST },
	{ "ms",             GUMBO_TAG_MS,              GUMBO_TAG_LAST },
	{ "mtext",          GUMBO_TAG_MTEXT,           GUMBO_TAG_LAST },
	{ "mglyph",         GUMBO_TAG_MGLYPH,          GUMBO_TAG_LAST },
	{ "malignmark",     GUMBO_TAG_MALIGNMARK,      GUMBO_TAG_LAST },
	{ "annotation-xml", GUMBO_TAG_ANNOTATION_XML,  GUMBO_TAG_LAST },
	{ "svg",            GUMBO_TAG_SVG,             GUMBO_TAG_LAST },
	{ "foreignobject",  GUMBO_TAG_FOREIGNOBJECT,   GUMBO_TAG_LAST },
	{ "desc",           GUMBO_TAG_DESC,            GUMBO_TAG_LAST },
	{ "table",          GUMBO_TAG_TABLE,           GUMBO_TAG_LAST },
	{ "caption",        GUMBO_TAG_CAPTION,         GUMBO_TAG_TABLE },
	{ "colgroup",       GUMBO_TAG_COLGROUP,        GUMBO_TAG_TABLE },
	{ "col",            GUMBO_TAG_COL,             GUMBO_TAG_TABLE },
	{ "tbody",          GUMBO_TAG_TBODY,           GUMBO_TAG_TABLE },
	{ "thead",          GUMBO_TAG_THEAD,           GUMBO_TAG_TABLE },
	{ "tfoot",          GUMBO_TAG_TFOOT,           GUMBO_TAG_TABLE },
	{ "tr",             GUMBO_TAG_TR,              GUMBO_TAG_TABLE },
	{ "td",             GUMBO_TAG_TD,              GUMBO_TAG_TABLE },
	{ "th",             GUMBO_TAG_TH,              GUMBO_TAG_TABLE },
	{ "form",           GUMBO_TAG_FORM,            GUMBO_TAG_LAST },
	{ "fieldset",       GUMBO_TAG_FIELDSET,        GUMBO_TAG_LAST },
	{ "legend",         GUMBO_TAG_LEGEND,          GUMBO_TAG_LAST },
	{ "label",          GUMBO_TAG_LABEL,           GUMBO_TAG_LAST },
	{ "input",          GUMBO_TAG_INPUT,           GUMBO_TAG_LAST },
	{ "button",         GUMBO_TAG_BUTTON,          GUMBO_TAG_LAST },
	{ "select",         GUMBO_TAG_SELECT,          GUMBO_TAG_LAST },
	{ "datalist",       GUMBO_TAG_DATALIST,        GUMBO_TAG_LAST },
	{ "optgroup",       GUMBO_TAG_OPTGROUP,        GUMBO_TAG_LAST },
	{ "option",         GUMBO_TAG_OPTION,          GUMBO_TAG_LAST },
	{ "textarea",       GUMBO_TAG_TEXTAREA,        GUMBO_TAG_LAST },
	{ "keygen",         GUMBO_TAG_KEYGEN,          GUMBO_TAG_LAST },
	{ "output",         GUMBO_TAG_OUTPUT,          GUMBO_TAG_LAST },
	{ "progress",       GUMBO_TAG_PROGRESS,        GUMBO_TAG_LAST },
	{ "meter",          GUMBO_TAG_METER,           GUMBO_TAG_LAST },
	{ "details",        GUMBO_TAG_DETAILS,         GUMBO_TAG_LAST },
	{ "summary",        GUMBO_TAG_SUMMARY,         GUMBO_TAG_LAST },
	{ "menu",           GUMBO_TAG_MENU,            GUMBO_TAG_LAST },
	{ "menuitem",       GUMBO_TAG_MENUITEM,        GUMBO_TAG_LAST },
	{ "applet",         GUMBO_TAG_APPLET,          GUMBO_TAG_LAST },
	{ "acronym",        GUMBO_TAG_ACRONYM,         GUMBO_TAG_LAST },
	{ "bgsound",        GUMBO_TAG_BGSOUND,         GUMBO_TAG_LAST },
	{ "dir",            GUMBO_TAG_DIR,             GUMBO_TAG_LAST },
	{ "frame",          GUMBO_TAG_FRAME,           GUMBO_TAG_LAST },
	{ "frameset",       GUMBO_TAG_FRAMESET,        GUMBO_TAG_LAST },
	{ "noframes",       GUMBO_TAG_NOFRAMES,        GUMBO_TAG_LAST },
	{ "isindex",        GUMBO_TAG_ISINDEX,         GUMBO_TAG_LAST },
	{ "listing",        GUMBO_TAG_LISTING,         GUMBO_TAG_LAST },
	{ "xmp",            GUMBO_TAG_XMP,             GUMBO_TAG_LAST },
	{ "nextid",         GUMBO_TAG_NEXTID,          GUMBO_TAG_LAST },
	{ "noembed",        GUMBO_TAG_NOEMBED,         GUMBO_TAG_LAST },
	{ "plaintext",      GUMBO_TAG_PLAINTEXT,       GUMBO_TAG_LAST },
	{ "rb",             GUMBO_TAG_RB,              GUMBO_TAG_LAST },
	{ "strike",         GUMBO_TAG_STRIKE,          GUMBO_TAG_LAST },
	{ "basefont",       GUMBO_TAG_BASEFONT,        GUMBO_TAG_LAST },
	{ "big",            GUMBO_TAG_BIG,             GUMBO_TAG_LAST },
	{ "blink",          GUMBO_TAG_BLINK,           GUMBO_TAG_LAST },
	{ "center",         GUMBO_TAG_CENTER,          GUMBO_TAG_LAST },
	{ "font",           GUMBO_TAG_FONT,            GUMBO_TAG_LAST },
	{ "marquee",        GUMBO_TAG_MARQUEE,         GUMBO_TAG_LAST },
	{ "multicol",       GUMBO_TAG_MULTICOL,        GUMBO_TAG_LAST },
	{ "nobr",           GUMBO_TAG_NOBR,            GUMBO_TAG_LAST },
	{ "spacer",         GUMBO_TAG_SPACER,          GUMBO_TAG_LAST },
	{ "tt",             GUMBO_TAG_TT,              GUMBO_TAG_LAST },
	{ "rtc",            GUMBO_TAG_RTC,             GUMBO_TAG_LAST },
	{ NULL, 0, 0 }
};


typedef struct DaoxHtmlNode    DaoxHtmlNode;
typedef struct DaoxHtmlDocument  DaoxHtmlDocument;


struct DaoxHtmlNode
{
	DAO_CSTRUCT_COMMON;

	DaoxHtmlDocument  *document;  /* For GC, to ensure GumboOutput is never freed before this; */
	GumboNode         *node;
	DString           *text;
	short              html;
};

extern DaoTypeCore daoHtmlNodeCore;



DaoxHtmlNode* DaoxHtmlNode_New( DaoxHtmlDocument *document )
{
	DaoCstruct *cstruct = (DaoCstruct*) document;
	DaoVmSpace *vmspace = DaoType_GetVmSpace( cstruct->ctype );
	DaoType *type = DaoVmSpace_GetType( vmspace, & daoHtmlNodeCore );
	DaoxHtmlNode *self = (DaoxHtmlNode*) dao_calloc(1, sizeof(DaoxHtmlNode));
	DaoCstruct_Init( (DaoCstruct*)self, type );
	self->text = DString_New();
	self->document = document;
	GC_IncRC( document );
	return self;
}

void DaoxHtmlNode_Delete( DaoxHtmlNode *self )
{
	DString_Delete( self->text );
	GC_IncRC( self->document );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}

static int DaoxHtmlNode_GetAttValue( DaoxHtmlNode *self, const char *name, DString *value )
{
	GumboAttribute *att;

	if( self->node->type != GUMBO_NODE_ELEMENT ) return 0;

	att = gumbo_get_attribute( & self->node->v.element.attributes, name );
	if( att == NULL ) return 0;

	DString_SetChars( value, att->value );
	return 1;
}
static int DaoxHtmlNode_SetAttValue( DaoxHtmlNode *self, const char *name, const char *value )
{
	GumboOptions options = kGumboDefaultOptions;
	GumboAttribute *att;
	int len = strlen( value );

	if( self->node->type != GUMBO_NODE_ELEMENT ) return 0;

	att = gumbo_get_attribute( & self->node->v.element.attributes, name );
	if( att == NULL ) return 0;

	options.deallocator( options.userdata, (void*) att->value );
	att->value = options.allocator( options.userdata, len + 1 );
	strcpy( (char*)att->value, value );
	return 1;
}
static DaoxHtmlNode* DaoxHtmlNode_FindByAttValue( DaoxHtmlNode *self, const char *name, const char *value )
{
	GumboAttribute *att;
	int i;

	if( self->node->type != GUMBO_NODE_ELEMENT ) return NULL;

	att = gumbo_get_attribute( & self->node->v.element.attributes, name );
	if( att && strcmp( att->value, value ) == 0 ) return self;

	for(i=0; i<self->node->v.element.children.length; ++i){
		GumboNode *child = (GumboNode*) self->node->v.element.children.data[i];
		DaoxHtmlNode *find = DaoxHtmlNode_FindByAttValue( (DaoxHtmlNode*) child->userdata, name, value );
		if( find ) return find;
	}
	return NULL;
}



struct DaoxHtmlDocument
{
	DAO_CSTRUCT_COMMON;

	GumboOutput  *output;
	DList        *allWrappers;
	DList        *freeWrappers;
	DList        *strings;
	DMap         *tags; /* TODO: move elsewhere; */
	daoint        useCount;
};



DaoxHtmlDocument* DaoxHtmlDocument_New( DaoType *type )
{
	int i;
	DaoxHtmlDocument *self = (DaoxHtmlDocument*) dao_calloc(1, sizeof(DaoxHtmlDocument));
	DaoCstruct_Init( (DaoCstruct*)self, type );
	self->strings = DList_New( DAO_DATA_STRING );
	self->allWrappers = DList_New( DAO_DATA_VALUE );
	self->freeWrappers = DList_New(0);
	self->tags = DHash_New( DAO_DATA_STRING, 0 );
	self->useCount = 0;
	for(i=0; html_tags[i].name != NULL; ++i){
		DString key = DString_WrapChars( html_tags[i].name );
		DMap_Insert( self->tags, & key, (void*)(size_t) html_tags[i].tag );
	}
	return self;
}

void DaoxHtmlDocument_Reset( DaoxHtmlDocument *self );

void DaoxHtmlDocument_Delete( DaoxHtmlDocument *self )
{
	DaoxHtmlDocument_Reset( self );
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->allWrappers );
	DList_Delete( self->freeWrappers );
	DList_Delete( self->strings);
	DMap_Delete( self->tags );
	dao_free( self );
}

void DaoxHtmlDocument_Reset( DaoxHtmlDocument *self )
{
	int i;

	self->useCount = 0;
	self->freeWrappers->size = 0;
	for(i=0; i<self->allWrappers->size; ++i){
		DList_Append( self->freeWrappers, self->allWrappers->items.pVoid[i] );
	}
	
	if( self->output == NULL ) return;
	gumbo_destroy_output( & kGumboDefaultOptions, self->output );
	self->output = NULL;
}
DString* DaoxHtmlDocument_AcquireString( DaoxHtmlDocument *self )
{
	DString *res;
	DString temp = DString_WrapChars( "" );
	if( self->strings->size <= self->useCount ) DList_Append( self->strings, & temp );
	res = self->strings->items.pString[ self->useCount ++ ];
	DString_Reset( res, 0 );
	return res;
}

static DaoxHtmlNode* DaoxHtmlDocument_MakeWrapper( DaoxHtmlDocument *self, GumboNode *node )
{
	DaoxHtmlNode *wrapper = (DaoxHtmlNode*) DList_PopBack( self->freeWrappers );
	if( wrapper == NULL ){
		wrapper = DaoxHtmlNode_New( self );
		DList_Append( self->allWrappers, wrapper );
	}
	wrapper->node = node;
	node->userdata = wrapper;
	return wrapper;
}
static void DaoxHtmlDocument_WrapNode( DaoxHtmlDocument *self, GumboNode *node )
{
	int i;

	node->userdata = DaoxHtmlDocument_MakeWrapper( self, node );

	if( node->type > GUMBO_NODE_TEXT ) return;

	//if( node->type == GUMBO_NODE_TEXT ){ }
	if( node->type != GUMBO_NODE_DOCUMENT && node->type != GUMBO_NODE_ELEMENT ){
		return;
	}
	for(i=0; i<node->v.element.children.length; ++i){
		GumboNode* child = (GumboNode*) node->v.element.children.data[i];
		DaoxHtmlDocument_WrapNode( self, child );
	}
}
static DaoxHtmlNode* DaoxHtmlDocument_FindRealRoot( DaoxHtmlDocument *self, GumboNode *node )
{
	int i;

	if( node->type != GUMBO_NODE_ELEMENT ) return NULL;
	if( node->v.element.original_tag.length > 0 ) return (DaoxHtmlNode*) node->userdata;

	for(i=0; i<node->v.element.children.length; ++i){
		GumboNode *child = (GumboNode*) node->v.element.children.data[i];
		DaoxHtmlNode *wrapper = DaoxHtmlDocument_FindRealRoot( self, child );
		if( wrapper ) return wrapper;
	}
	return NULL;
}
void DaoxHtmlDocument_Parse( DaoxHtmlDocument *self, DString *source )
{
	daoint start = 0, end = 0;
	char *chars = source->chars;
	DString *part = DaoxHtmlDocument_AcquireString( self );
	GumboOptions options = kGumboDefaultOptions;
	DNode *it;

	while( start < source->size && isalpha( chars[start] ) == 0 ) start += 1;
	end = start;
	while( end < source->size && (isalnum( chars[end] ) || chars[end] == '-') ) end += 1;

	DString_SubString( source, part, start, end - start );

	it = DMap_Find( self->tags, part );
	if( it ) options.fragment_context = html_tags[ it->value.pInt ].ctx;

	self->useCount --;

	DaoxHtmlDocument_Reset( self );
	self->output = gumbo_parse_with_options( & options, source->chars, source->size );

	DaoxHtmlDocument_WrapNode( self, self->output->document );
}



/*
// Adapted from gumbo-document/examples/serialize.cc!
*/

const char *nonbreaking_inline  = "|a|abbr|acronym|b|bdo|big|cite|code|dfn|em|font|i|img|kbd|nobr|s|small|span|strike|strong|sub|sup|tt|";
const char *empty_tags          = "|area|base|basefont|bgsound|br|command|col|embed|event-source|frame|hr|image|img|input|keygen|link|menuitem|meta|param|source|spacer|track|wbr|";
const char *special_handling    = "|html|body|";
const char *no_entity_sub       = "|script|style|";


static void substitute_xml_entities_into_text(DString *text)
{
	DString_Change( text, "{{&}}", "&amp;", 0 );
	DString_Change( text, "{{<}}", "&lt;", 0 );
	DString_Change( text, "{{>}}", "&gt;", 0 );
}


static void substitute_xml_entities_into_attributes(char quote, DString *text)
{
	substitute_xml_entities_into_text(text);
	if (quote == '"') {
		DString_Change( text, "{{\"}}", "&quot;", 0 );
	} else if (quote == '\'') {
		DString_Change( text,"{{'}}", "&apos;", 0 );
	}
}


static void handle_unknown_tag(GumboStringPiece *text, DString *tagname )
{
	// work with copy GumboStringPiece to prevent asserts 
	// if try to read same unknown tag name more than once
	GumboStringPiece gsp = *text;

	DString_Reset( tagname, 0 );

	if (text->data == NULL) return;

	gumbo_tag_from_original_text(&gsp);
	DString_SetBytes( tagname, gsp.data, gsp.length );
}


static void get_tag_name(GumboNode *node, DString *tagname )
{
	DString_Reset( tagname, 0 );
	// work around lack of proper name for document node
	if (node->type == GUMBO_NODE_DOCUMENT) {
		DString_SetChars( tagname, "document" );
	} else {
		DString_SetChars( tagname, gumbo_normalized_tagname(node->v.element.tag) );
	}
	if (tagname->size == 0) {
		handle_unknown_tag(&node->v.element.original_tag, tagname);
	}
}


static void build_doctype(GumboNode *node, DString *results)
{
	DString_Reset( results, 0 );
	if (node->v.document.has_doctype) {
		DString pi = DString_WrapChars(node->v.document.public_identifier);
		DString_AppendChars( results, "<!DOCTYPE ");
		DString_AppendChars( results, node->v.document.name);
		if ((node->v.document.public_identifier != NULL) && pi.size ) {
			DString_AppendChars( results, " PUBLIC \"");
			DString_AppendChars( results, node->v.document.public_identifier);
			DString_AppendChars( results, "\" \"");
			DString_AppendChars( results, node->v.document.system_identifier);
			DString_AppendChars( results, "\"");
		}
		DString_AppendChars( results, ">\n");
	}
}


static void DaoxHtmlNode_BuildAttribute( DaoxHtmlNode *self, GumboAttribute *at, int no_entities, DString *atts)
{
	DString *value = DaoxHtmlDocument_AcquireString( self->document );

	DString_AppendChar( atts, ' ' );
	DString_AppendChars( atts, at->name );

	DString_SetChars( value, at->value );
	if ( value->size || 
			(at->original_value.data[0] == '"') || 
			(at->original_value.data[0] == '\'') ) {

		char quote = at->original_value.data[0];
		char qs = quote == '\'' || quote == '"' ? quote : 0;

		DString_AppendChar( atts, '=' );
		if( qs ) DString_AppendChar( atts, qs );

		if (no_entities) {
			DString_Append( atts, value );
		} else {
			substitute_xml_entities_into_attributes(quote, value);
			DString_Append( atts, value );
		}
		if( qs ) DString_AppendChar( atts, qs );
	}
	self->document->useCount --;
}


static void DaoNode_Serialize( DaoxHtmlNode *self, int level, DString *indent, DString *output );

static void DaoNode_SerializeContents( DaoxHtmlNode *self, int level, DString *indent, DString *contents)
{
	int useCount = self->document->useCount;
	int i, no_entity_substitution;
	GumboNode *node = self->node;
	GumboVector* children;
	DString *tagname = DaoxHtmlDocument_AcquireString( self->document );
	DString *key = DaoxHtmlDocument_AcquireString( self->document );
	DString *val = DaoxHtmlDocument_AcquireString( self->document );

	get_tag_name( node, tagname );
	DString_SetChars( key, "|" );
	DString_Append( key, tagname );
	DString_AppendChar( key, '|' );

	no_entity_substitution = strstr( no_entity_sub, key->chars ) != NULL;

	children = & node->v.element.children;

	for (i = 0; i < children->length; ++i) {
		GumboNode* child = (GumboNode*) children->data[i];

		if (child->type == GUMBO_NODE_TEXT) {

			DString_SetChars( val, child->v.text.text );

			if (! no_entity_substitution) {
				substitute_xml_entities_into_text( val );
			}

			DString_Append( contents, val );

		} else if ((child->type == GUMBO_NODE_ELEMENT) || (child->type == GUMBO_NODE_TEMPLATE)) {

			DString_Reset( val, 0 );
			DaoNode_Serialize( (DaoxHtmlNode*) child->userdata, level, indent, val);
			DString_Append( contents, val );

		} else if (child->type == GUMBO_NODE_WHITESPACE) {

			DString_AppendChars( contents, child->v.text.text );

		} else if (child->type != GUMBO_NODE_COMMENT) {
			fprintf(stderr, "unknown element of type: %d\n", child->type); 

		}

	}
	if( self && self->text->size ){
		DString_Assign( val, self->text );
		if( self->html == 0 ) substitute_xml_entities_into_text( val );
		DString_Append( contents, val );
	}
	self->document->useCount = useCount;
}


static void DaoNode_Serialize( DaoxHtmlNode *self, int level, DString *indent, DString *output )
{
	int useCount = self->document->useCount;
	DString *close    = DaoxHtmlDocument_AcquireString( self->document );
	DString *closeTag = DaoxHtmlDocument_AcquireString( self->document );
	DString *atts     = DaoxHtmlDocument_AcquireString( self->document );
	DString *tagname  = DaoxHtmlDocument_AcquireString( self->document );
	DString *key      = DaoxHtmlDocument_AcquireString( self->document );
	DString *contents = DaoxHtmlDocument_AcquireString( self->document );
	GumboNode *node = self->node;
	const GumboVector *attribs;
	int need_special_handling;
	int no_entity_substitution;
	int is_empty_tag;
	int i; 

	if( node->v.element.original_tag.length == 0 ){
		DaoNode_SerializeContents( self, level+1, indent, output );
		self->document->useCount = useCount;
		return;
	}

	if (node->type == GUMBO_NODE_DOCUMENT) {
		build_doctype(node, output);
		DaoNode_SerializeContents( self, level+1, indent, output );
		self->document->useCount = useCount;
		return;
	}

	get_tag_name( node, tagname );
	DString_SetChars( key, "|" );
	DString_Append( key, tagname );
	DString_AppendChar( key, '|' );

	need_special_handling   = strstr( special_handling, key->chars ) != NULL;
	is_empty_tag            = strstr( empty_tags, key->chars ) != NULL;
	no_entity_substitution  = strstr( no_entity_sub, key->chars ) != NULL;

	attribs = & node->v.element.attributes;
	for (i=0; i< attribs->length; ++i) {
		GumboAttribute* at = (GumboAttribute*) attribs->data[i];
		DaoxHtmlNode_BuildAttribute( self, at, no_entity_substitution, atts);
	}

	if (is_empty_tag) {
		DString_SetChars( close, "/" );
	} else {
		DString_SetChars( closeTag, "</" );
		DString_Append( closeTag, tagname );
		DString_AppendChar( closeTag, '>' );
	}

	DaoNode_SerializeContents( self, level+1, indent, contents );

	if (need_special_handling) {
		DString_Trim( contents, 1, 1, 0 );
		DString_AppendChar( contents, '\n' );
	}

	DString_AppendChar( output, '<' );
	DString_Append( output, tagname );
	DString_Append( output, atts );
	DString_Append( output, close );
	DString_AppendChar( output, '>' );

	if (need_special_handling) DString_AppendChar( output, '\n' );

	DString_Append( output, contents );
	DString_Append( output, closeTag );
	if (need_special_handling)  DString_AppendChar( output, '\n' );

	self->document->useCount = useCount;
}




static void NODE_ID( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *id = p[1]->xString.value;
	DaoxHtmlNode *res = DaoxHtmlNode_FindByAttValue( self, "id", id->chars );
	if( res ){
		DaoProcess_PutValue( proc, (DaoValue*) res );
	}else{
		DaoProcess_PutNone( proc );
	}
}
static void NODE_SetID( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *id = p[1]->xString.value;
}
static void NODE_GetATT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *name = p[1]->xString.value;
	DString *value = DaoProcess_PutChars( proc, "" );
	if( DaoxHtmlNode_GetAttValue( self, name->chars, value ) == 0 ){
		DaoProcess_PutNone( proc );
	}
}
static void NODE_SetATT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *value = p[1]->xString.value;
	DString *name = p[2]->xString.value;
	if( DaoxHtmlNode_SetAttValue( self, name->chars, value->chars ) == 0 ){
		DaoProcess_PutNone( proc );
	}
}
static void NODE_GetText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *text = DaoProcess_PutChars( proc, "" );
	int i;
	if( self->node->type == GUMBO_NODE_TEXT ){
		DString_SetChars( text, self->node->v.text.text );
		return;
	}
	for(i=0; i<self->node->v.element.children.length; ++i){
		GumboNode *child = (GumboNode*) self->node->v.element.children.data[i];
		if( child->type == GUMBO_NODE_TEXT ){
			DString_AppendChars( text, child->v.text.text );
		}
	}
}
static void GumboNode_SetText( GumboNode *node, const char *text )
{
	GumboOptions options = kGumboDefaultOptions;
	int len = strlen( text );

	options.deallocator( options.userdata, (void*) node->v.text.text );
	node->v.text.text = options.allocator( options.userdata, len + 1 );
	strcpy( (char*)node->v.text.text, text );
}
static void NODE_SetText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *text = p[1]->xString.value;
	int i;
	if( self->node->type == GUMBO_NODE_TEXT ){
		GumboNode_SetText( self->node, text->chars );
		return;
	}
	for(i=0; i<self->node->v.element.children.length; ++i){
		GumboNode *child = (GumboNode*) self->node->v.element.children.data[i];
		if( child->type == GUMBO_NODE_TEXT ){
			GumboNode_SetText( child, text->chars );
			return;
		}
	}
	DString_Assign( self->text, text );
	self->html = 0;
}
static void NODE_GetHtml( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *html = DaoProcess_PutChars( proc, "" );
	DString indent = DString_WrapChars( "  " );
	DaoNode_Serialize( self, 1, & indent, html );
}
static void NODE_SetHtml( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DString *text = p[1]->xString.value;
	DString_Assign( self->text, text );
	self->html = 1;
}
static void NODE_EachAtt( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 2 );
	GumboVector *attribs = & self->node->v.element.attributes;
	DaoString *name, *value;
	int i, entry;

	if( sect == NULL ) return;

	name = DaoString_New();
	value = DaoString_New();
	entry = proc->topFrame->entry;
	for (i=0; i< attribs->length; ++i) {
		GumboAttribute* at = (GumboAttribute*) attribs->data[i];
		if( sect->b > 0 ){
			DaoString_SetChars( name, at->name );
			DaoProcess_SetValue( proc, sect->a, (DaoValue*) name );
		}
		if( sect->b > 1 ){
			DaoString_SetChars( value, at->value );
			DaoProcess_SetValue( proc, sect->a+1, (DaoValue*) value );
		}
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_PROCESS_ABORTED ) break;
		if( proc->stackValues[0] && proc->stackValues[0]->type == DAO_STRING ){
			DaoxHtmlNode_SetAttValue( self, at->name, proc->stackValues[0]->xString.value->chars );
		}
	}
	DaoProcess_PopFrame( proc );
	DaoString_Delete( name );
	DaoString_Delete( value );
}
static void NODE_EachSub( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p[0];
	DaoVmCode *sect;
	GumboVector *children;
	int i, entry;

	if( self->node->type != GUMBO_NODE_DOCUMENT && self->node->type != GUMBO_NODE_ELEMENT ){
		DaoProcess_RaiseError( proc, "Param", "this node has no children node" );
		return;
	}

	sect = DaoProcess_InitCodeSection( proc, 2 );
	if( sect == NULL ) return;

	entry = proc->topFrame->entry;
	children = & self->node->v.element.children;
	for (i = 0; i < children->length; ++i) {
		GumboNode* child = (GumboNode*) children->data[i];
		if( sect->b > 0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) child->userdata );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_PROCESS_ABORTED ) break;
	}
	DaoProcess_PopFrame( proc );
}

static DaoFunctionEntry DaoxHtmlNodeMeths[]=
{
	{ NODE_ID,      ".( self: Node, id: string ) => Node|none" },
	{ NODE_SetID,   ".=( self: Node, id: string, node: Node|none )" },
	{ NODE_GetATT,  "[]( self: Node, name: string ) => string|none" },
	{ NODE_SetATT,  "[]=( self: Node, value: string, name: string )" },

	{ NODE_GetText, ".text( self: Node ) => string" },
	{ NODE_SetText, ".text=( self: Node, text: string )" },

	{ NODE_GetHtml, ".html( self: Node ) => string" },
	{ NODE_SetHtml, ".html=( self: Node, html: string )" },

	{ NODE_EachAtt, "EachAttribute( self: Node )"
		"[name: string, value: string => string|none]"
	},
	// TODO: filtering by node type;
	{ NODE_EachSub, "EachSubNode( self: Node )[node: Node]" },

	{ NULL, NULL }
};

static void DaoxHtmlNode_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int rm )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p;
	DList_Append( values, self->document );
	if( rm ) self->document = NULL;
}


DaoTypeCore daoHtmlNodeCore =
{
	"Node",                                            /* name */
	sizeof(DaoxHtmlNode),                              /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	DaoxHtmlNodeMeths,                                 /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	DaoCstruct_CheckSetField,  DaoCstruct_DoSetField,  /* SetField */
	DaoCstruct_CheckGetItem,   DaoCstruct_DoGetItem,   /* GetItem */
	DaoCstruct_CheckSetItem,   DaoCstruct_DoSetItem,   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxHtmlNode_Delete,           /* Delete */
	DaoxHtmlNode_HandleGC                              /* HandleGC */
};



static void DOC_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *retype = DaoProcess_GetReturnType( proc );
	DaoxHtmlDocument *self = DaoxHtmlDocument_New( retype );
	DString *source = p[0]->xString.value;
	DaoxHtmlDocument_Parse( self, source );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void DOC_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlDocument *self = (DaoxHtmlDocument*) p[0];
	DString *source = p[1]->xString.value;
	DaoxHtmlDocument_Parse( self, source );
	DaoProcess_PutValue( proc, (DaoValue*) self->output->root->userdata );
}
static void DOC_GetRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlDocument *self = (DaoxHtmlDocument*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) self->output->document->userdata );
}
static void DOC_GetContent( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlDocument *self = (DaoxHtmlDocument*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) self->output->root->userdata );
}

static DaoFunctionEntry DaoxHtmlDocumentMeths[]=
{
	{ DOC_New,          "Document( source = '' )" },
	{ DOC_Parse,        "Parse( self: Document, source: string ) => Node" },
	{ DOC_GetRoot,      ".root( self: Document ) => Node" },
	{ DOC_GetContent,   ".content( self: Document ) => Node" },

	{ NULL, NULL }
};

static void DaoxHtmlDocument_HandleGC( DaoValue *p, DList *vs, DList *lists, DList *maps, int rm )
{
	DaoxHtmlDocument *self = (DaoxHtmlDocument*) p;
	DList_Append( lists, self->allWrappers );
}


DaoTypeCore daoHtmlDocumentCore =
{
	"Document",                                        /* name */
	sizeof(DaoxHtmlDocument),                          /* size */
	{ NULL },                                          /* bases */
	{ NULL },                                          /* casts */
	NULL,                                              /* numbers */
	DaoxHtmlDocumentMeths,                             /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxHtmlDocument_Delete,       /* Delete */
	DaoxHtmlDocument_HandleGC                          /* HandleGC */
};



DAO_DLL int DaoHtml_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *htmlns = DaoNamespace_GetNamespace( ns, "html" );

	DaoNamespace_WrapType( htmlns, & daoHtmlNodeCore, DAO_CSTRUCT, 0 );
	DaoNamespace_WrapType( htmlns, & daoHtmlDocumentCore, DAO_CSTRUCT, 0 );

	DaoNamespace_DefineType( htmlns, html_global_attr, "GlobalAttr" );
	DaoNamespace_DefineType( htmlns, html_step_attr, "Step" );
	DaoNamespace_DefineType( htmlns, html_target_attr, "Target" );
	DaoNamespace_DefineType( htmlns, html_formtarget_attr, "FormTarget" );
	DaoNamespace_WrapFunctions( htmlns, htmlMeths );
	return 0;
}
