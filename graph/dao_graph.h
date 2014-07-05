/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011,2012, Limin Fu
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

#ifndef __DAO_GRAPH_H__
#define __DAO_GRAPH_H__

#define DAO_LIST_ITEM_TYPES  struct DaoxNode **pgNode;  struct DaoxEdge **pgEdge;

#include"daoStdtype.h"

typedef struct DaoxGraph  DaoxGraph;
typedef struct DaoxNode   DaoxNode;
typedef struct DaoxEdge   DaoxEdge;

/*
// DaoxGraph, DaoxNode and DaoxEdge only provide backbone data structures for graphs.
// If a graph algorithm requires additional data other than the node and edge weights,
// such data can be stored in the "value" field, if the algorithm is to be implemented
// in Dao; or they can be stored in the "X" field, if the algorithm is to be implemented
// in C/C++.
//
// See the implementation of the maximum flow algorithm for an example.
// 
// Note:
// No reference counting is used for some of the fields in the node and edge data
// structures, because, doing so, there will be a huge burden on the Dao GC if the
// graph is big (if any node or edge has refCount decrease, the GC would have to
// scan the entire graph to determine if the node and its graph is dead).
*/

/* Data type used by various graph algorithms: */

/* Maximum Flow: */
typedef struct DaoxNodeMF  DaoxNodeMF;
typedef struct DaoxEdgeMF  DaoxEdgeMF;

/* Affinity Propagation Clustering: */
typedef struct DaoxNodeAP  DaoxNodeAP;
typedef struct DaoxEdgeAP  DaoxEdgeAP;


DAO_DLL DaoTypeBase DaoxNode_Typer;
DAO_DLL DaoTypeBase DaoxEdge_Typer;
DAO_DLL DaoTypeBase DaoxGraph_Typer;

struct DaoxNode
{
	DAO_CSTRUCT_COMMON;

	DaoxGraph  *graph; /* Without reference counting; */
	DList      *ins;   /* in edges:  <DaoxEdge*>; Without reference counting; */
	DList      *outs;  /* out edges: <DaoxEdge*>; Without reference counting; */
	DaoValue   *value; /* Dao user data; With reference counting; */
	double      weight;
	daoint      state;

	union {
		void        *Void;
		DaoxNodeMF  *MF;
		DaoxNodeAP  *AP;
	} X; /* C user data; */
};

DAO_DLL DaoxNode* DaoxNode_New( DaoxGraph *graph );
DAO_DLL void DaoxNode_Delete( DaoxNode *self );

struct DaoxEdge
{
	DAO_CSTRUCT_COMMON;

	DaoxGraph  *graph;  /* Without reference counting; */
	DaoxNode   *first;  /* Without reference counting; */
	DaoxNode   *second; /* Without reference counting; */
	DaoValue   *value;  /* With reference counting; */
	double      weight;

	union {
		void        *Void;
		DaoxEdgeMF  *MF;
		DaoxEdgeAP  *AP;
	} X;
};

DAO_DLL DaoxEdge* DaoxEdge_New( DaoxGraph *graph );
DAO_DLL void DaoxEdge_Delete( DaoxEdge *self );

struct DaoxGraph
{
	DAO_CSTRUCT_COMMON;

	DList   *nodes; /* <DaoxNode*>; With reference counting; */
	DList   *edges; /* <DaoxEdge*>; With reference counting; */
	short    directed; /* directed graph; */

	DaoType  *nodeType; /* With reference counting; */
	DaoType  *edgeType; /* With reference counting; */
};
DAO_DLL DaoType *daox_node_template_type;
DAO_DLL DaoType *daox_edge_template_type;
DAO_DLL DaoType *daox_graph_template_type;

DAO_DLL DaoxGraph* DaoxGraph_New( DaoType *type, int directed );
DAO_DLL void DaoxGraph_Delete( DaoxGraph *self );

DAO_DLL DaoxNode* DaoxGraph_AddNode( DaoxGraph *self );
DAO_DLL DaoxEdge* DaoxGraph_AddEdge( DaoxGraph *self, DaoxNode *first, DaoxNode *second );

DAO_DLL daoint DaoxGraph_RandomInit( DaoxGraph *self, daoint N, double prob );

DAO_DLL void DaoxNode_BreadthFirstSearch( DaoxNode *self, DList *nodes );
DAO_DLL void DaoxNode_DepthFirstSearch( DaoxNode *self, DList *nodes );
DAO_DLL void DaoxGraph_ConnectedComponents( DaoxGraph *self, DList *cclist );




#define DAOX_GRAPH_DATA DaoxGraph *graph; DString *nodeData; DString *edgeData

typedef struct DaoxGraphData  DaoxGraphData;

struct DaoxGraphData
{
	DAO_CSTRUCT_COMMON;
	DAOX_GRAPH_DATA;
};
DAO_DLL DaoTypeBase DaoxGraphData_Typer;
DAO_DLL DaoType *daox_graph_data_type;

DAO_DLL void DaoxGraphData_Init( DaoxGraphData *self, DaoType *type );
DAO_DLL void DaoxGraphData_Clear( DaoxGraphData *self );

DAO_DLL void DaoxGraphData_Reset( DaoxGraphData *self, DaoxGraph *graph, int nodeSize, int edgeSize );

DAO_DLL void DaoxGraphData_GetGCFields( void *p, DList *vs, DList *as, DList *ms, int rm );

DAO_DLL int DaoxGraphData_IsAssociated( DaoxGraphData *self, DaoxGraph *graph, DaoProcess *proc );



typedef struct DaoxGraphMaxFlow  DaoxGraphMaxFlow;

struct DaoxNodeMF
{
	daoint  nextpush;
	daoint  height;
	double  excess;
};
struct DaoxEdgeMF
{
	double  capacity;
	double  flow_fw; /* forward flow; */
	double  flow_bw; /* backward flow; */
};
struct DaoxGraphMaxFlow
{
	DAO_CSTRUCT_COMMON;
	DAOX_GRAPH_DATA;

	double  maxflow;
};
DAO_DLL DaoType *daox_graph_maxflow_type;

DAO_DLL DaoxGraphMaxFlow* DaoxGraphMaxFlow_New();
DAO_DLL void DaoxGraphMaxFlow_Delete( DaoxGraphMaxFlow *self );

DAO_DLL void DaoxGraphMaxFlow_Init( DaoxGraphMaxFlow *self, DaoxGraph *graph );
DAO_DLL int DaoxGraphMaxFlow_Compute( DaoxGraphMaxFlow *self, DaoxNode *source, DaoxNode *sink );

#endif
