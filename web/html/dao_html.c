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
#include "dao.h"
#include "daoString.h"
#include "daoGC.h"
#include "daoValue.h"
#include "daoStdtype.h"
#include "daoProcess.h"

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

static DaoFuncItem htmlMeths[] =
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

typedef struct DaoxHtmlNode    DaoxHtmlNode;
typedef struct DaoxHtmlParser  DaoxHtmlParser;


struct DaoxHtmlNode
{
	DAO_CSTRUCT_COMMON;

	DaoxHtmlParser  *parser;  /* For GC, to ensure GumboOutput is never freed before this; */
	GumboNode       *node;
	DString         *text;
	short            html;
};
DaoType *daox_type_html_node;
DaoType *daox_type_html_document_node;
DaoType *daox_type_html_element_node;
DaoType *daox_type_html_text_node;



DaoxHtmlNode* DaoxHtmlNode_New( DaoxHtmlParser *parser )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) dao_calloc(1, sizeof(DaoxHtmlNode));
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_html_node );
	self->text = DString_New();
	self->parser = parser;
	GC_IncRC( parser );
	return self;
}

void DaoxHtmlNode_Delete( DaoxHtmlNode *self )
{
	DString_Delete( self->text );
	GC_IncRC( self->parser );
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
	strncpy( (char*)att->value, value, len );
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



struct DaoxHtmlParser
{
	DAO_CSTRUCT_COMMON;

	GumboOutput  *output;
	DList        *allWrappers;
	DList        *freeWrappers;
	DList        *strings;
	daoint        useCount;
};
DaoType *daox_type_html_parser;



DaoxHtmlParser* DaoxHtmlParser_New()
{
	DaoxHtmlParser *self = (DaoxHtmlParser*) dao_calloc(1, sizeof(DaoxHtmlParser));
	DaoCstruct_Init( (DaoCstruct*)self, daox_type_html_parser );
	self->strings = DList_New( DAO_DATA_STRING );
	self->allWrappers = DList_New( DAO_DATA_VALUE );
	self->freeWrappers = DList_New(0);
	self->useCount = 0;
	return self;
}

void DaoxHtmlParser_Reset( DaoxHtmlParser *self );

void DaoxHtmlParser_Delete( DaoxHtmlParser *self )
{
	DaoxHtmlParser_Reset( self );
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->allWrappers );
	DList_Delete( self->freeWrappers );
	dao_free( self );
}

void DaoxHtmlParser_Reset( DaoxHtmlParser *self )
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
static DaoxHtmlNode* DaoxHtmlParser_MakeWrapper( DaoxHtmlParser *self, GumboNode *node )
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
static void DaoxHtmlParser_WrapNode( DaoxHtmlParser *self, GumboNode *node )
{
	int i;

	node->userdata = DaoxHtmlParser_MakeWrapper( self, node );

	if( node->type > GUMBO_NODE_TEXT ) return;

	//if( node->type == GUMBO_NODE_TEXT ){ }
	if( node->type != GUMBO_NODE_DOCUMENT && node->type != GUMBO_NODE_ELEMENT ){
		return;
	}
	for(i=0; i<node->v.element.children.length; ++i){
		GumboNode* child = (GumboNode*) node->v.element.children.data[i];
		DaoxHtmlParser_WrapNode( self, child );
	}
}
static DaoxHtmlNode* DaoxHtmlParser_FindRealRoot( DaoxHtmlParser *self, GumboNode *node )
{
	int i;

	if( node->type != GUMBO_NODE_ELEMENT ) return NULL;
	if( node->v.element.original_tag.length > 0 ) return (DaoxHtmlNode*) node->userdata;

	for(i=0; i<node->v.element.children.length; ++i){
		GumboNode *child = (GumboNode*) node->v.element.children.data[i];
		DaoxHtmlNode *wrapper = DaoxHtmlParser_FindRealRoot( self, child );
		if( wrapper ) return wrapper;
	}
	return NULL;
}
void DaoxHtmlParser_Parse( DaoxHtmlParser *self, DString *source )
{
	GumboOptions options = kGumboDefaultOptions;

	DaoxHtmlParser_Reset( self );
	self->output = gumbo_parse_with_options( & options, source->chars, source->size );

	DaoxHtmlParser_WrapNode( self, self->output->document );
}
DString* DaoxHtmlParser_AcquireString( DaoxHtmlParser *self )
{
	DString *res;
	DString temp = DString_WrapChars( "" );
	if( self->strings->size <= self->useCount ) DList_Append( self->strings, & temp );
	res = self->strings->items.pString[ self->useCount ++ ];
	DString_Reset( res, 0 );
	return res;
}



/*
// Adapted from gumbo-parser/examples/serialize.cc!
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
	DString *value = DaoxHtmlParser_AcquireString( self->parser );

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
	self->parser->useCount --;
}


static void DaoNode_Serialize( DaoxHtmlNode *self, int level, DString *indent, DString *output );

static void DaoNode_SerializeContents( DaoxHtmlNode *self, int level, DString *indent, DString *contents)
{
	int useCount = self->parser->useCount;
	int i, no_entity_substitution;
	GumboNode *node = self->node;
	GumboVector* children;
	DString *tagname = DaoxHtmlParser_AcquireString( self->parser );
	DString *key = DaoxHtmlParser_AcquireString( self->parser );
	DString *val = DaoxHtmlParser_AcquireString( self->parser );

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
	self->parser->useCount = useCount;
}


static void DaoNode_Serialize( DaoxHtmlNode *self, int level, DString *indent, DString *output )
{
	int useCount = self->parser->useCount;
	DString *close    = DaoxHtmlParser_AcquireString( self->parser );
	DString *closeTag = DaoxHtmlParser_AcquireString( self->parser );
	DString *atts     = DaoxHtmlParser_AcquireString( self->parser );
	DString *tagname  = DaoxHtmlParser_AcquireString( self->parser );
	DString *key      = DaoxHtmlParser_AcquireString( self->parser );
	DString *contents = DaoxHtmlParser_AcquireString( self->parser );
	GumboNode *node = self->node;
	const GumboVector *attribs;
	int need_special_handling;
	int no_entity_substitution;
	int is_empty_tag;
	int i; 

	if( node->v.element.original_tag.length == 0 ){
		DaoNode_SerializeContents( self, level+1, indent, output );
		self->parser->useCount = useCount;
		return;
	}

	if (node->type == GUMBO_NODE_DOCUMENT) {
		build_doctype(node, output);
		DaoNode_SerializeContents( self, level+1, indent, output );
		self->parser->useCount = useCount;
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

	contents = DString_New();
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

	self->parser->useCount = useCount;
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
	strncpy( (char*)node->v.text.text, text, len );
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

static DaoFuncItem DaoxHtmlNodeMeths[]=
{
	{ NODE_ID,      ".( self: Node, id: string ) => Node|none" },
	{ NODE_GetATT,  "[]( self: Node, name: string ) => string|none" },
	{ NODE_SetATT,  "[]=( self: Node, name: string, value: string )" },

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

static void DaoxHtmlNode_GetGCFields( void *p, DList *values, DList *lists, DList *maps, int rm )
{
	DaoxHtmlNode *self = (DaoxHtmlNode*) p;
	DList_Append( values, self->parser );
	if( rm ) self->parser = NULL;
}

DaoTypeBase DaoxHtmlNode_Typer =
{
	"Node", NULL, NULL, (DaoFuncItem*) DaoxHtmlNodeMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxHtmlNode_Delete, DaoxHtmlNode_GetGCFields
};



static void PARS_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlParser *self = DaoxHtmlParser_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PARS_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlParser *self = (DaoxHtmlParser*) p[0];
	DString *source = p[1]->xString.value;
	DaoxHtmlParser_Parse( self, source );
	DaoProcess_PutValue( proc, (DaoValue*) self->output->root->userdata );
}
static void PARS_GetDocument( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlParser *self = (DaoxHtmlParser*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) self->output->document->userdata );
}
static void PARS_GetRoot( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxHtmlParser *self = (DaoxHtmlParser*) p[0];
	DaoProcess_PutValue( proc, (DaoValue*) self->output->root->userdata );
}

static DaoFuncItem DaoxHtmlParserMeths[]=
{
	{ PARS_New,          "Parser()" },
	{ PARS_Parse,        "Parse( self: Parser, source: string ) => Node" },
	{ PARS_GetDocument,  ".document( self: Parser ) => Node" },
	{ PARS_GetRoot,      ".root( self: Parser ) => Node" },

	{ NULL, NULL }
};

static void DaoxHtmlParser_GetGCFields( void *p, DList *vs, DList *lists, DList *maps, int rm )
{
	DaoxHtmlParser *self = (DaoxHtmlParser*) p;
	DList_Append( lists, self->allWrappers );
}

DaoTypeBase DaoxHtmlParser_Typer =
{
	"Parser", NULL, NULL, (DaoFuncItem*) DaoxHtmlParserMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxHtmlParser_Delete, DaoxHtmlParser_GetGCFields
};



DAO_DLL int DaoHtml_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *htmlns = DaoNamespace_GetNamespace( ns, "html" );

	daox_type_html_node = DaoNamespace_WrapType( htmlns, & DaoxHtmlNode_Typer, 0 );
	daox_type_html_parser = DaoNamespace_WrapType( htmlns, & DaoxHtmlParser_Typer, 0 );

	DaoNamespace_DefineType( htmlns, html_global_attr, "GlobalAttr" );
	DaoNamespace_DefineType( htmlns, html_step_attr, "Step" );
	DaoNamespace_DefineType( htmlns, html_target_attr, "Target" );
	DaoNamespace_DefineType( htmlns, html_formtarget_attr, "FormTarget" );
	DaoNamespace_WrapFunctions( htmlns, htmlMeths );
	return 0;
}
