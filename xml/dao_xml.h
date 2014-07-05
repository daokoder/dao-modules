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

// 2014-05: Danilov Aleksey, initial implementation

#include"dao.h"
#include"daoValue.h"
#include"daoStdtype.h"

#include<errno.h>

#ifndef __DAO_XML_H__
#define __DAO_XML_H__

typedef int xml_item;

enum {
	XMLElement,
	XMLEmptyElement,
	XMLTextElement,
	XMLInstruction,
	XMLComment,
	XMLText,
	XMLCdata,
};

typedef struct XMLContext XMLContext;
typedef struct DaoXMLNode DaoXMLNode;
typedef struct DaoXMLElement DaoXMLElement;
typedef struct DaoXMLCharData DaoXMLCharData;
typedef struct DaoXMLInstruction DaoXMLInstruction;

struct XMLContext {
	char *pos;
	int len;
	mbstate_t state;
};

#define DAO_XML_NODE xml_item kind; daoint refs; DaoXMLElement *parent

struct DaoXMLNode {
	DAO_XML_NODE;
};

struct DaoXMLElement {
	DAO_XML_NODE;
	DString tag;
	DMap *attribs;
	union {
		DString *text;
		DArray *children;
	} c;
};

struct DaoXMLCharData {
	DAO_XML_NODE;
	DString data;
};

struct DaoXMLInstruction {
	DAO_XML_NODE;
	DString name, data;
};

struct DaoXMLDocument {
	DString *version, *doctype, *encoding;
	int standalone;
	DArray *instructions;
	DaoXMLElement *root;
	DMap *namepool;
};

typedef struct DaoXMLDocument DaoXMLDocument;

struct DaoXMLWriter {
	DaoStream *stream;
	DaoList *tagstack;
	int indent;
	uchar_t start, end, closed, decl, dtd;
	uchar_t del;
};

typedef struct DaoXMLWriter DaoXMLWriter;

#endif
