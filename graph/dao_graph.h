/*=========================================================================================
  This file is a part of the Dao standard modules.
  Copyright (C) 2011-2012, Fu Limin. Email: fu@daovm.net, limin.fu@yahoo.com

  This software is free software; you can redistribute it and/or modify it under the terms 
  of the GNU Lesser General Public License as published by the Free Software Foundation; 
  either version 2.1 of the License, or (at your option) any later version.

  This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
  See the GNU Lesser General Public License for more details.
  =========================================================================================*/

#ifndef __DAO_GRAPH_H__
#define __DAO_GRAPH_H__

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


DAO_DLL extern DaoTypeBase DaoxNode_Typer;
DAO_DLL extern DaoTypeBase DaoxEdge_Typer;
DAO_DLL extern DaoTypeBase DaoxGraph_Typer;

struct DaoxNode
{
	DAO_CDATA_COMMON;

	DaoxGraph  *graph; /* Without reference counting; */
	DArray     *ins;   /* in edges:  <DaoxEdge*>; Without reference counting; */
	DArray     *outs;  /* out edges: <DaoxEdge*>; Without reference counting; */
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
	DAO_CDATA_COMMON;

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
	DAO_CDATA_COMMON;

	DArray  *nodes; /* <DaoxNode*>; With reference counting; */
	DArray  *edges; /* <DaoxEdge*>; With reference counting; */
	short    directed; /* directed graph; */

	DaoType  *nodeType; /* With reference counting; */
	DaoType  *edgeType; /* With reference counting; */
};
DAO_DLL extern DaoType *daox_node_template_type;
DAO_DLL extern DaoType *daox_edge_template_type;
DAO_DLL extern DaoType *daox_graph_template_type;

DAO_DLL DaoxGraph* DaoxGraph_New( DaoType *type, int directed );
DAO_DLL void DaoxGraph_Delete( DaoxGraph *self );

DAO_DLL DaoxNode* DaoxGraph_AddNode( DaoxGraph *self );
DAO_DLL DaoxEdge* DaoxGraph_AddEdge( DaoxGraph *self, DaoxNode *first, DaoxNode *second );

DAO_DLL daoint DaoxGraph_RandomInit( DaoxGraph *self, daoint N, double prob );

DAO_DLL void DaoxNode_BreadthFirstSearch( DaoxNode *self, DArray *nodes );
DAO_DLL void DaoxNode_DepthFirstSearch( DaoxNode *self, DArray *nodes );
DAO_DLL void DaoxGraph_ConnectedComponents( DaoxGraph *self, DArray *cclist );




#define DAOX_GRAPH_DATA DaoxGraph *graph; DString *nodeData; DString *edgeData

typedef struct DaoxGraphData  DaoxGraphData;

struct DaoxGraphData
{
	DAO_CDATA_COMMON;
	DAOX_GRAPH_DATA;
};
DAO_DLL extern DaoTypeBase DaoxGraphData_Typer;
DAO_DLL extern DaoType *daox_graph_data_type;

DAO_DLL void DaoxGraphData_Init( DaoxGraphData *self, DaoType *type );
DAO_DLL void DaoxGraphData_Clear( DaoxGraphData *self );

DAO_DLL void DaoxGraphData_Reset( DaoxGraphData *self, DaoxGraph *graph, int nodeSize, int edgeSize );

DAO_DLL void DaoxGraphData_GetGCFields( void *p, DArray *vs, DArray *as, DArray *ms, int rm );

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
	DAO_CDATA_COMMON;
	DAOX_GRAPH_DATA;

	double  maxflow;
};
DAO_DLL extern DaoType *daox_graph_maxflow_type;

DAO_DLL DaoxGraphMaxFlow* DaoxGraphMaxFlow_New();
DAO_DLL void DaoxGraphMaxFlow_Delete( DaoxGraphMaxFlow *self );

DAO_DLL void DaoxGraphMaxFlow_Init( DaoxGraphMaxFlow *self, DaoxGraph *graph );
DAO_DLL int DaoxGraphMaxFlow_Compute( DaoxGraphMaxFlow *self, DaoxNode *source, DaoxNode *sink );

#endif
