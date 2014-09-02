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
	int i;
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
	case DAO_DOUBLE:
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
		DString_AppendChar( line, '\t' );
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
	"tuple<enum<data_>, tuple<enum, string>> | tuple<enum<dir>, enum<ltr,rtl,auto>> | enum<contenteditable,hidden,spellcheck> | "
	"tuple<enum<contenteditable,draggable,spellcheck>, enum<true,false>> | tuple<enum<tabindex>, int> | "
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
	{ Html_Document,	"document(...: tuple<enum<manifest,xml_lang>, string> | attr::global)[] => string" },

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
	 * or \c enum (void attribute). All tag routines support parameters of type `attr::global`, which covers all global HTML attributes.
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
	{ Html_Tag,			"anchor(...: tuple<enum<href,hreflang,media,_type>, string> | attr::target | "
									"tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;"
														  "prev;search;tag>> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"abbrev(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"address(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"area(...: tuple<enum<alt,href,media,hreflang,_type>, string> | attr::target | "
								  "tuple<enum<shape>, enum<rect,circle,poly,default>> | tuple<enum<coords>, tuple<...: int>> | "
								  "tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;"
														"prev;search;tag>> | attr::global)" },
	{ Html_Tag,			"article(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"aside(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"audio(...: enum<autoplay,preload,controls,loop,muted> | tuple<enum<preload>, enum<none,metadata,auto>> | "
								   "tuple<enum<mediagroup,src>, string> | attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"base(...: tuple<enum<href>, string> | attr::target | attr::global)" },
	{ Html_Tag,			"bdi(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"bdo(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"blockquote(...: tuple<enum<cite>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"body(...: tuple<enum<onafterprint,onbeforeprint,onbeforeunload,onhashchange,onmessage,onoffline,ononline,"
											 "onpagehide,onpageshow,onpopstate,onresize,onstorage,onunload>, string> | "
								  "attr::global)[ => string|none ]" },
	{ Html_Tag,			"bold(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"br(...: attr::global)" },
	{ Html_Tag,			"button(kind: enum<submit>, ...: "
							   "tuple<enum<name,form,value,formaction,formenctype>, string> | attr::formtarget |  "
							   "enum<disabled,autofocus,formnovalidate> | tuple<enum<formmethod>, enum<get,post>> | attr::global)"
							   "[ => string|none ]" },
	{ Html_Tag,			"button(kind: enum<reset,button>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
															  "attr::global)[ => string|none ]" },
	{ Html_Tag,			"canvas(...: tuple<enum<height,width>, int> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"caption(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"cite(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"code(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"col(...: tuple<enum<span>, int> | attr::global)" },
	{ Html_Tag,			"colgroup(...: tuple<enum<span>, int> | attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"command(kind: enum<command>, ...: tuple<enum<label,icon>, string> | enum<disabled> | attr::global)" },
	{ Html_EmptyTag,	"command(kind: enum<radio>, ...: tuple<enum<radiogroup,icon,label>, string> | enum<checked,disabled> | "
														"attr::global)" },
	{ Html_EmptyTag,	"command(kind: enum<checkbox>, ...: tuple<enum<label,icon>, string> | enum<checked,disabled> | "
														   "attr::global)" },
	{ Html_Tag,			"datalist(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"dd(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"del(...: tuple<enum<cite,datetime>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"details(...: enum<open> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"dfn(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"div(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"dl(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"dt(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"em(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"embed(...: tuple<enum<src,_type>, string> | tuple<enum<height,width>, int> | attr::global)" },
	{ Html_Tag,			"fieldset(...: tuple<enum<name,form>, string> | enum<disabled> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"figcaption(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"figure(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"footer(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"form(...: tuple<enum<action,enctype,name,accept_charset>, string> | attr::target | enum<novalidate> | "
								  "tuple<enum<method>, enum<get,post>> | tuple<enum<autocomplete>, enum<on,off>> | attr::global)"
							"[ => string|none ]" },
	{ Html_Tag,			"h1(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"h2(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"h3(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"h4(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"h5(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"h6(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"head(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"header(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"hgroup(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"hr(...: attr::global)" },
	{ Html_Tag,			"iframe(...: tuple<enum<src,srcdoc,name>, string> | tuple<enum<height,width>, int> | "
									"tuple<enum<sandbox>, enum<allow_forms;allow_scripts;allow_top_navigation;allow_same_origin>> | "
									"enum<sandbox,seamless> | attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"img(...: tuple<enum<src,alt,usemap>, string> | tuple<enum<height,width>, int> | enum<ismap> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<text,search>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder,dirname>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<password>, ...: "
							  "tuple<enum<name,form,value,placeholder>, string> | tuple<enum<maxlength,size>, int> | "
							  "enum<disabled,readonly,autofocus,required> | tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<checkbox,radio>, ...: tuple<enum<name,form,value>, string> | "
															   "enum<checked,disabled,autofocus,required> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<button,reset>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
															 "attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<submit>, ...: "
							  "tuple<enum<name,form,value>, string> | enum<disabled,autofocus> | "
							  "tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | attr::formtarget | "
							  "tuple<enum<formmethod>, enum<get,post>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<file>, ...: tuple<enum<name,form,accept>, string> | "
													 "enum<disabled,autofocus,required,multiple> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<hidden>, ...: tuple<enum<name,form,value>, string> | enum<disabled> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<image>, ...: "
							  "tuple<enum<name,form,alt,src>, string> | enum<disabled,autofocus> | "
							  "tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | attr::formtarget | "
							  "tuple<enum<formmethod>, enum<get,post>> | tuple<enum<height,width>, int> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<datetime,datetime_local,date,month,time,week>, ...: "
							  "tuple<enum<name,form,value,list,min,max>, string> | enum<disabled,autofocus,readonly,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::step | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<number>, ...: "
							  "tuple<enum<name,form,value,list,placeholder>, string> | tuple<enum<min,max>, float> | attr::step | "
							  "enum<disabled,autofocus,readonly,required> | tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<range>, ...: "
							  "tuple<enum<name,form,value,list>, string> | tuple<enum<min,max>, float> | enum<disabled,autofocus> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::step | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<email>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<url,tel>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<color>, ...: "
							  "tuple<enum<name,form,value,list,pattern,placeholder,dir>, string> | "
							  "tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> | "
							  "tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_EmptyTag,	"input(kind: enum<color>, ...: tuple<enum<name,form,list,value>, string> | enum<disabled,autofocus> | "
													  "tuple<enum<autocomplete>, enum<on,off>> | attr::global)" },
	{ Html_Tag,			"ins(...: tuple<enum<cite,datetime>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"italic(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"kbd(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"keygen(...: tuple<enum<challenge,name,form>, string> | tuple<enum<keytype>, enum<rsa>> | "
									"enum<autofocus,disabled> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"label(...: tuple<enum<id,_for,form>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"legend(...: tuple<enum<value>, int> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"li(...: tuple<enum<value>, int> | attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"link(...: tuple<enum<href,hreflang,media,_type,sizes>, string> | "
								  "tuple<enum<rel>, enum<alternate;author;help;icon;license;next;prefetch;prev;search;stylesheet>> | "
								  "attr::global)" },
	{ Html_Tag,			"map(...: tuple<enum<name>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"mark(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"menu(kind: enum<toolbar,context>, ...: tuple<enum<label>, string> | attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<name,content>, string> | attr::global)" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<http_eqiv>, enum<refresh,default_style,content_type>> | tuple<enum<content>, string> | "
								  "attr::global)" },
	{ Html_EmptyTag,	"meta(...: tuple<enum<charset>, string> | attr::global)" },
	{ Html_Tag, 		"meter(...: tuple<enum<value,min,low,high,max,optimum>, float> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"nav(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"noscript(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"object(...: tuple<enum<data,_type,usemap,name,form>, string> | tuple<enum<height,width>, int> | "
									"attr::global)[ => string|none ]" },
	{ Html_Tag,			"ol(...: tuple<enum<_type>, string> | tuple<enum<start>, int> | enum<reversed> | attr::global)"
						  "[ => string|none ]" },
	{ Html_Tag,			"optgroup(...: tuple<enum<label>, string> | enum<disabled> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"option(...: tuple<enum<label,value>, string> | enum<disabled,selected> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"output(...: tuple<enum<name,form,_for>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"paragraph(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"param(...: tuple<enum<name,value>, string> | attr::global)" },
	{ Html_Tag,			"pre(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"progress(...: tuple<enum<value,max>, float> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"quoted(...: tuple<enum<cite>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"rp(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"ruby(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"struck(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"samp(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"script(...: tuple<enum<_type,src,charset>, string> | enum<defer,async> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"section(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"select(...: tuple<enum<name,form>, string> | tuple<enum<size>, int> | "
									"enum<disabled,multiple,autofocus,required> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"small(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"source(...: tuple<enum<_type,src,media>, string> | attr::global)" },
	{ Html_Tag,			"span(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"strong(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"style(...: tuple<enum<_type,media>, string> | enum<scoped> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"sub(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"summary(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"sup(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"table(...: tuple<enum<border>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"tbody(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"td(...: tuple<enum<colspan,rowspan>, int> | tuple<enum<headers>, list<string>> | attr::global)"
						  "[ => string|none ]" },
	{ Html_Tag,			"textarea(...: tuple<enum<name,form,placeholder,dirname>, string> | "
									  "enum<disabled,readonly,autofocus,required> | tuple<enum<maxlength,rows,cols>, int> | "
									  "tuple<enum<wrap>, enum<hard,soft>> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"tfoot(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"th(...: tuple<enum<scope>, enum<row,col,rowgroup,colgroup>> | tuple<enum<colspan,rowspan>, int> | "
								"tuple<enum<headers>, list<string>> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"thead(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"time(...: tuple<enum<datetime>, string> | attr::global)[ => string|none ]" },
	{ Html_Tag,			"title(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"tr(...: attr::global)[ => string|none ]" },
	{ Html_EmptyTag,	"track(...: tuple<enum<kind>, enum<subtitles,captions,descriptions,chapters,metadata>> | "
								   "tuple<enum<src,srclang,label>, string> | enum<default> | attr::global)" },
	{ Html_Tag,			"underlined(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"ul(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"variable(...: attr::global)[ => string|none ]" },
	{ Html_Tag,			"video(...: tuple<enum<preload>, enum<none,metadata,auto>> | enum<autoplay,controls,loop,muted> | "
								   "tuple<enum<poster,mediagroup,src>, string> | tuple<enum<height,width>, int> | attr::global)"
							 "[ => string|none ]" },
	{ Html_EmptyTag,	"wbr(...: attr::global)" },
	{ NULL, NULL }
};

DAO_DLL int DaoHtml_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *htmlns, *attrns;
	htmlns = DaoNamespace_GetNamespace( ns, "html" );
	attrns = DaoNamespace_GetNamespace( htmlns, "attr" );
	DaoNamespace_AddConstValue( ns, "html", (DaoValue*)htmlns );
	DaoNamespace_AddConstValue( htmlns, "attr", (DaoValue*)attrns );
	DaoNamespace_TypeDefine( attrns, html_global_attr, "global" );
	DaoNamespace_TypeDefine( attrns, html_step_attr, "step" );
	DaoNamespace_TypeDefine( attrns, html_target_attr, "target" );
	DaoNamespace_TypeDefine( attrns, html_formtarget_attr, "formtarget" );
	DaoNamespace_WrapFunctions( htmlns, htmlMeths );
	return 0;
}
