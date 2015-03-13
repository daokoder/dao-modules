## web.html -- generation of HTML5 web pages

This module provides DSL-like interface to generate web pages, assuming the role commonly carried out by template engines. It encompasses HTML5 syntax, allowing to produce HTML documents and fragments of documents without writing actual HTML code, while benefiting from static checks provided by the language.
### Index
namespace [html](#html)

[Attribute types](#attrs)

[Tag functions](#tags)

Other functions:
- [document](#document)(...: tuple&lt;enum&lt;manifest,xml_lang&gt;, string&gt; | GlobalAttr)[] => string
- [fragment](#fragment)()[] => string
- [text](#text)(text: string)
- [comment](#comment)(text: string)

<a name="html"></a>
<a name="attrs"></a>
###Attribute types
```ruby
type GlobalAttr = tuple<enum<accesskey,_class,contextmenu,id,lang,style,title>, string> | tuple<enum<dropzone>, enum<copy;move;link>|string> |
	tuple<enum<data_>, tuple<enum, string>> | tuple<enum<dir>, enum<ltr,rtl,auto>> | enum<contenteditable,hidden,spellcheck> |
	tuple<enum<contenteditable,draggable,spellcheck>, bool> | tuple<enum<tabindex>, int> | tuple<enum<translate>, enum<yes,no>> |
	tuple<enum<onabort,onblur,oncancel,oncanplay,oncanplaythrough,onchange,onclick,onclose,oncontextmenu,oncuechange,
			   ondblclick,ondrag,ondragend,ondragenter,ondragleave,ondragover,ondragstart,ondrop,ondurationchange,onemptied,
			   onended,onerror,onfocus,oninput,oninvalid,onkeydown,onkeypress,onkeyup,onload,onloadeddata,onloadedmetadata,
			   onloadstart,onmousedown,onmousemove,onmouseout,onmouseover,onmouseup,onmousewheel,onpause,onplay,onplaying,
			   onprogress,onratechange,onreset,onscroll,onseeked,onseeking,onselect,onshow,onstalled,onsubmit,onsuspend
			   ontimeupdate,onvolumechange,onwaiting>, string>
type Step = tuple<enum<step>, float|enum<any>>
type Target = tuple<enum<target>, enum<blank,self,parent,top>|string>
type FormTarget = tuple<enum<formtarget>, enum<blank,self,parent,top>|string>
```
These types represent corresponding HTML attributes and are provided to avoid unnecessary repetition of parameter types in function prototypes.
<a name="tags"></a>
###Tag functions
```ruby
anchor(...: tuple<enum<href,hreflang,media,_type>, string> | Target |
            tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;
								  prev;search;tag>> | GlobalAttr)[ => string|none ]
abbrev(...: GlobalAttr)[ => string|none ]
address(...: GlobalAttr)[ => string|none ]
area(...: tuple<enum<alt,href,media,hreflang,_type>, string> | Target |
		  tuple<enum<shape>, enum<rect,circle,poly,default>> | tuple<enum<coords>, tuple<...: int>> |
		  tuple<enum<rel>, enum<alternate;author;bookmark;help;license;next;nofollow;noreferrer;prefetch;
								prev;search;tag>> | GlobalAttr)
article(...: GlobalAttr)[ => string|none ]
aside(...: GlobalAttr)[ => string|none ]
audio(...: enum<autoplay,preload,controls,loop,muted> | tuple<enum<preload>, enum<none,metadata,auto>> |
		   tuple<enum<mediagroup,src>, string> | GlobalAttr)[ => string|none ]
base(...: tuple<enum<href>, string> | Target | GlobalAttr)
bdi(...: GlobalAttr)[ => string|none ]
bdo(...: GlobalAttr)[ => string|none ]
blockquote(...: tuple<enum<cite>, string> | GlobalAttr)[ => string|none ]
body(...: tuple<enum<onafterprint,onbeforeprint,onbeforeunload,onhashchange,onmessage,onoffline,ononline,
					 onpagehide,onpageshow,onpopstate,onresize,onstorage,onunload>, string> |
		  GlobalAttr)[ => string|none ]
bold(...: GlobalAttr)[ => string|none ]
br(...: GlobalAttr)
button(kind: enum<submit>, ...:
			   tuple<enum<name,form,value,formaction,formenctype>, string> | FormTarget |  
			   enum<disabled,autofocus,formnovalidate> | tuple<enum<formmethod>, enum<get,post>> | GlobalAttr)
	   [ => string|none ]
button(kind: enum<reset,button>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> |
									  GlobalAttr)[ => string|none ]
canvas(...: tuple<enum<height,width>, int> | GlobalAttr)[ => string|none ]
caption(...: GlobalAttr)[ => string|none ]
cite(...: GlobalAttr)[ => string|none ]
code(...: GlobalAttr)[ => string|none ]
col(...: tuple<enum<span>, int> | GlobalAttr)
colgroup(...: tuple<enum<span>, int> | GlobalAttr)[ => string|none ]
command(kind: enum<command>, ...: tuple<enum<label,icon>, string> | enum<disabled> | GlobalAttr)
command(kind: enum<radio>, ...: tuple<enum<radiogroup,icon,label>, string> | enum<checked,disabled> |
								GlobalAttr)
command(kind: enum<checkbox>, ...: tuple<enum<label,icon>, string> | enum<checked,disabled> |
								   GlobalAttr)
datalist(...: GlobalAttr)[ => string|none ]
dd(...: GlobalAttr)[ => string|none ]
del(...: tuple<enum<cite,datetime>, string> | GlobalAttr)[ => string|none ]
details(...: enum<open> | GlobalAttr)[ => string|none ]
dfn(...: GlobalAttr)[ => string|none ]
div(...: GlobalAttr)[ => string|none ]
dl(...: GlobalAttr)[ => string|none ]
dt(...: GlobalAttr)[ => string|none ]
em(...: GlobalAttr)[ => string|none ]
embed(...: tuple<enum<src,_type>, string> | tuple<enum<height,width>, int> | GlobalAttr)
fieldset(...: tuple<enum<name,form>, string> | enum<disabled> | GlobalAttr)[ => string|none ]
figcaption(...: GlobalAttr)[ => string|none ]
figure(...: GlobalAttr)[ => string|none ]
footer(...: GlobalAttr)[ => string|none ]
form(...: tuple<enum<action,enctype,name,accept_charset>, string> | Target | enum<novalidate> |
		  tuple<enum<method>, enum<get,post>> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
	[ => string|none ]
h1(...: GlobalAttr)[ => string|none ]
h2(...: GlobalAttr)[ => string|none ]
h3(...: GlobalAttr)[ => string|none ]
h4(...: GlobalAttr)[ => string|none ]
h5(...: GlobalAttr)[ => string|none ]
h6(...: GlobalAttr)[ => string|none ]
head(...: GlobalAttr)[ => string|none ]
header(...: GlobalAttr)[ => string|none ]
hgroup(...: GlobalAttr)[ => string|none ]
hr(...: GlobalAttr)
iframe(...: tuple<enum<src,srcdoc,name>, string> | tuple<enum<height,width>, int> |
			tuple<enum<sandbox>, enum<allow_forms;allow_scripts;allow_top_navigation;allow_same_origin>> |
			enum<sandbox,seamless> | GlobalAttr)[ => string|none ]
img(...: tuple<enum<src,alt,usemap>, string> | tuple<enum<height,width>, int> | enum<ismap> | GlobalAttr)
input(kind: enum<text,search>, ...:
						  tuple<enum<name,form,value,list,pattern,placeholder,dirname>, string> |
						  tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> |
						  tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<password>, ...:
		  tuple<enum<name,form,value,placeholder>, string> | tuple<enum<maxlength,size>, int> |
		  enum<disabled,readonly,autofocus,required> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<checkbox,radio>, ...: tuple<enum<name,form,value>, string> |
									   enum<checked,disabled,autofocus,required> | GlobalAttr)
input(kind: enum<button,reset>, ...: tuple<enum<name,form,value>, string> | enum<disabled,autofocus> |
									 GlobalAttr)
input(kind: enum<submit>, ...:
						  tuple<enum<name,form,value>, string> | enum<disabled,autofocus> |
						  tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | FormTarget |
						  tuple<enum<formmethod>, enum<get,post>> | GlobalAttr)
input(kind: enum<file>, ...: tuple<enum<name,form,accept>, string> |
							 enum<disabled,autofocus,required,multiple> | GlobalAttr)
input(kind: enum<hidden>, ...: tuple<enum<name,form,value>, string> | enum<disabled> | GlobalAttr)
input(kind: enum<image>, ...:
					  tuple<enum<name,form,alt,src>, string> | enum<disabled,autofocus> |
					  tuple<enum<formaction,formenctype>, string> | enum<formnovalidate> | FormTarget |
					  tuple<enum<formmethod>, enum<get,post>> | tuple<enum<height,width>, int> | GlobalAttr)
input(kind: enum<datetime,datetime_local,date,month,time,week>, ...:
			  tuple<enum<name,form,value,list,min,max>, string> | enum<disabled,autofocus,readonly,required> |
			  tuple<enum<autocomplete>, enum<on,off>> | Step | GlobalAttr)
input(kind: enum<number>, ...:
			  tuple<enum<name,form,value,list,placeholder>, string> | tuple<enum<min,max>, float> | Step |
			  enum<disabled,autofocus,readonly,required> | tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<range>, ...:
		  tuple<enum<name,form,value,list>, string> | tuple<enum<min,max>, float> | enum<disabled,autofocus> |
		  tuple<enum<autocomplete>, enum<on,off>> | Step | GlobalAttr)
input(kind: enum<email>, ...:
					  tuple<enum<name,form,value,list,pattern,placeholder>, string> |
					  tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> |
					  tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<url,tel>, ...:
							  tuple<enum<name,form,value,list,pattern,placeholder>, string> |
							  tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required> |
							  tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<color>, ...:
					  tuple<enum<name,form,value,list,pattern,placeholder,dir>, string> |
					  tuple<enum<maxlength,size>, int> | enum<disabled,readonly,autofocus,required,multiple> |
					  tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
input(kind: enum<color>, ...: tuple<enum<name,form,list,value>, string> | enum<disabled,autofocus> |
							  tuple<enum<autocomplete>, enum<on,off>> | GlobalAttr)
ins(...: tuple<enum<cite,datetime>, string> | GlobalAttr)[ => string|none ]
italic(...: GlobalAttr)[ => string|none ]
kbd(...: GlobalAttr)[ => string|none ]
keygen(...: tuple<enum<challenge,name,form>, string> | tuple<enum<keytype>, enum<rsa>> |
			enum<autofocus,disabled> | GlobalAttr)[ => string|none ]
label(...: tuple<enum<id,_for,form>, string> | GlobalAttr)[ => string|none ]
legend(...: tuple<enum<value>, int> | GlobalAttr)[ => string|none ]
li(...: tuple<enum<value>, int> | GlobalAttr)[ => string|none ]
link(...: tuple<enum<href,hreflang,media,_type,sizes>, string> |
		  tuple<enum<rel>, enum<alternate;author;help;icon;license;next;prefetch;prev;search;stylesheet>> |
		  GlobalAttr)
map(...: tuple<enum<name>, string> | GlobalAttr)[ => string|none ]
mark(...: GlobalAttr)[ => string|none ]
menu(kind: enum<toolbar,context>, ...: tuple<enum<label>, string> | GlobalAttr)[ => string|none ]
meta(...: tuple<enum<name,content>, string> | GlobalAttr)
meta(...: tuple<enum<http_equiv>, enum<refresh,default_style,content_type>> | tuple<enum<content>, string> |
		  GlobalAttr)
meta(...: tuple<enum<charset>, string> | GlobalAttr)
meter(...: tuple<enum<value,min,low,high,max,optimum>, float> | GlobalAttr)[ => string|none ]
nav(...: GlobalAttr)[ => string|none ]
noscript(...: GlobalAttr)[ => string|none ]
object(...: tuple<enum<data,_type,usemap,name,form>, string> | tuple<enum<height,width>, int> |
			GlobalAttr)[ => string|none ]
ol(...: tuple<enum<_type>, string> | tuple<enum<start>, int> | enum<reversed> | GlobalAttr)
  [ => string|none ]
optgroup(...: tuple<enum<label>, string> | enum<disabled> | GlobalAttr)[ => string|none ]
option(...: tuple<enum<label,value>, string> | enum<disabled,selected> | GlobalAttr)[ => string|none ]
output(...: tuple<enum<name,form,_for>, string> | GlobalAttr)[ => string|none ]
paragraph(...: GlobalAttr)[ => string|none ]
param(...: tuple<enum<name,value>, string> | GlobalAttr)
pre(...: GlobalAttr)[ => string|none ]
progress(...: tuple<enum<value,max>, float> | GlobalAttr)[ => string|none ]
quoted(...: tuple<enum<cite>, string> | GlobalAttr)[ => string|none ]
rp(...: GlobalAttr)[ => string|none ]
ruby(...: GlobalAttr)[ => string|none ]
struck(...: GlobalAttr)[ => string|none ]
samp(...: GlobalAttr)[ => string|none ]
script(...: tuple<enum<_type,src,charset>, string> | enum<defer,async> | GlobalAttr)[ => string|none ]
section(...: GlobalAttr)[ => string|none ]
select(...: tuple<enum<name,form>, string> | tuple<enum<size>, int> |
			enum<disabled,multiple,autofocus,required> | GlobalAttr)[ => string|none ]
small(...: GlobalAttr)[ => string|none ]
source(...: tuple<enum<_type,src,media>, string> | GlobalAttr)
span(...: GlobalAttr)[ => string|none ]
strong(...: GlobalAttr)[ => string|none ]
style(...: tuple<enum<_type,media>, string> | enum<scoped> | GlobalAttr)[ => string|none ]
sub(...: GlobalAttr)[ => string|none ]
summary(...: GlobalAttr)[ => string|none ]
sup(...: GlobalAttr)[ => string|none ]
table(...: tuple<enum<border>, string> | GlobalAttr)[ => string|none ]
tbody(...: GlobalAttr)[ => string|none ]
td(...: tuple<enum<colspan,rowspan>, int> | tuple<enum<headers>, invar<list<string>>> | GlobalAttr)
  [ => string|none ]
textarea(...: tuple<enum<name,form,placeholder,dirname>, string> |
			  enum<disabled,readonly,autofocus,required> | tuple<enum<maxlength,rows,cols>, int> |
			  tuple<enum<wrap>, enum<hard,soft>> | GlobalAttr)[ => string|none ]
tfoot(...: GlobalAttr)[ => string|none ]
th(...: tuple<enum<scope>, enum<row,col,rowgroup,colgroup>> | tuple<enum<colspan,rowspan>, int> |
		tuple<enum<headers>, invar<list<string>>> | GlobalAttr)[ => string|none ]
thead(...: GlobalAttr)[ => string|none ]
time(...: tuple<enum<datetime>, string> | GlobalAttr)[ => string|none ]
title(...: GlobalAttr)[ => string|none ]
tr(...: GlobalAttr)[ => string|none ]
track(...: tuple<enum<kind>, enum<subtitles,captions,descriptions,chapters,metadata>> |
		   tuple<enum<src,srclang,label>, string> | enum<default> | GlobalAttr)
underlined(...: GlobalAttr)[ => string|none ]
ul(...: GlobalAttr)[ => string|none ]
variable(...: GlobalAttr)[ => string|none ]
video(...: tuple<enum<preload>, enum<none,metadata,auto>> | enum<autoplay,controls,loop,muted> |
		   tuple<enum<poster,mediagroup,src>, string> | tuple<enum<height,width>, int> | GlobalAttr)
	 [ => string|none ]
wbr(...: GlobalAttr)
```
Each tag routine corresponds to individual HTML element. Code section routine indicates element which may have content, i.e. nested tags, text or comments. Tag routines, [text()](#text) and [comment()](#comment) called within such code sections are automatically nested inside the corresponding parent tag in the resulting HTML code. To avoid unnecessary text() calls, all code sections support `string` as returned value to be used for elements whose content can be specified as a single `string` value.

 __Note:__ For elements which wrap short text fragments ('em', 'q', 'span', etc.) it is often better to directly use HTML markup within strings -- it may appear more concise and clear in certain cases. Strings which are code section results and [text()](#text) arguments may include arbitrary HTML code.

Element attributes may be specified via variadic parameters, each of which is either a named value (attribute with a value) or `enum` (void attribute). All tag routines support parameters of type `GlobalAttr`, which covers all global HTML attributes.

Routine names mostly match those of HTML tags; however, there are few exceptions made to avoid naming conflicts:
- 'a' -> 'anchor';
- 'b' -> 'bold';
- 'i' -> 'italic';
- 'p' -> 'paragraph';
- 'q' -> 'qouted';
- 's' -> 'struck';
- 'u' -> 'underlined';
- 'var' -> 'variable'.

That is, all single-letter tags (plus 'var') are replaced with the corresponding full words.

Attribute names also mostly match HTML specification; the exceptions are:
- 'type', 'class' and 'for' start with '_' as they are Dao keywords;
- the names originally containing '-' and ':' ('xml:lang') use '_' instead;
- 'data-*' attributes are provided via `data_` parameter accepting name-value tuple.

__Note:__ For `data_` parameter, any '_' in its name suffix is replaced with '-'.

Additionally, `button()`, `command()` and `input()` do not accept 'type' attribute as named value, instead expecting single `enum` value as the first, non-variadic routine argument. This allows to provide overloaded version of these routines with different parameter sets similar to HTML specification for these tags.
<a name="functions"></a>
###Other functions
<a name="document"></a>
```ruby
document(...: tuple<enum<manifest,xml_lang>, string> | GlobalAttr)[] => string
```
Returns HTML 5 document composed from content specified in the code section. Includes '<!DOCTYPE html>' and 'html' tag with the specified attributes
<a name="fragment"></a>
```ruby
fragment()[] => string
```
Returns HTML 5 code fragment composed from content specified in the code section
<a name="text"></a>
```ruby
text(text: string)
```
Specifies plain *text*, but may as well contain arbitrary HTML markup (*text* is included 'as-is' into the resulting HTML code)
<a name="comment"></a>
```ruby
comment(text: string)
```
Specifies comment with the given *text*
