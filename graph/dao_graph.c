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

#include"math.h"
#include"daoGC.h"
#include"daoValue.h"
#include"dao_graph.h"


/* @W: type of weights (U1, U2) in nodes, (W1, W2) in edges; */
/* @N: type of user data for nodes; */
/* @E: type of user data for edges; */

#define TYPE_PARAMS "<@W<none|int|float|double>=none,@N=none,@E=none>"

DaoxNode* DaoxNode_New( DaoxGraph *graph )
{
	DaoxNode *self = (DaoxNode*) dao_calloc( 1, sizeof(DaoxNode) );
	DaoCdata_InitCommon( (DaoCdata*) self, graph->nodeType );
	self->edges = DArray_New(D_VALUE);
	self->graph = graph;
	GC_IncRC( graph );
	return self;
}
void DaoxNode_Delete( DaoxNode *self )
{
	DaoCdata_FreeCommon( (DaoCdata*) self );
	DArray_Delete( self->edges );
	GC_DecRC( self->graph );
	dao_free( self );
}
void DaoxNode_SetValue( DaoxNode *self, DaoValue *value )
{
	DaoValue_Move( value, & self->value, self->ctype->nested->items.pType[1] );
}

DaoxEdge* DaoxEdge_New( DaoxGraph *graph )
{
	DaoxEdge *self = (DaoxEdge*) dao_calloc( 1, sizeof(DaoxEdge) );
	DaoCdata_InitCommon( (DaoCdata*) self, graph->edgeType );
	self->first = self->second = NULL;
	self->graph = graph;
	GC_IncRC( graph );
	return self;
}
void DaoxEdge_Delete( DaoxEdge *self )
{
	DaoCdata_FreeCommon( (DaoCdata*) self );
	GC_DecRC( self->graph );
	GC_DecRC( self->first );
	GC_DecRC( self->second );
	dao_free( self );
}
void DaoxEdge_SetValue( DaoxEdge *self, DaoValue *value )
{
	DaoValue_Move( value, & self->value, self->ctype->nested->items.pType[1] );
}

DaoxGraph* DaoxGraph_New( DaoType *type, int directed )
{
	DaoxGraph *self = (DaoxGraph*) dao_calloc( 1, sizeof(DaoxGraph) );
	DaoCdata_InitCommon( (DaoCdata*) self, type );
	self->nodes = DArray_New(D_VALUE);
	self->edges = DArray_New(D_VALUE);
	self->directed = directed;
	self->wtype = 0;
	self->nodeType = NULL;
	self->edgeType = NULL;
	self->nodeData = NULL;
	self->edgeData = NULL;
	if( type ){
		if( type->nested->size ) self->wtype = type->nested->items.pType[0]->tid;
		self->nodeType = DaoCdataType_Specialize( daox_node_template_type, type->nested );
		self->edgeType = DaoCdataType_Specialize( daox_edge_template_type, type->nested );
		GC_IncRC( self->nodeType );
		GC_IncRC( self->edgeType );
	}
	if( self->wtype > DAO_DOUBLE ) self->wtype = DAO_DOUBLE;
	return self;
}
void DaoxGraph_Delete( DaoxGraph *self )
{
	DaoCdata_FreeCommon( (DaoCdata*) self );
	DArray_Delete( self->nodes );
	DArray_Delete( self->edges );
	GC_DecRC( self->nodeType );
	GC_DecRC( self->edgeType );
	if( self->nodeData ) DString_Delete( self->nodeData );
	if( self->edgeData ) DString_Delete( self->edgeData );
	dao_free( self );
}

DaoxNode* DaoxGraph_AddNode( DaoxGraph *self )
{
	DaoxNode *node = DaoxNode_New( self );
	DArray_Append( self->nodes, node );
	return node;
}
DaoxEdge* DaoxGraph_AddEdge( DaoxGraph *self, DaoxNode *first, DaoxNode *second )
{
	DaoxEdge *edge = DaoxEdge_New( self );
	if( self->directed ){
		DArray_PushFront( first->edges, edge );
	}else{
		DArray_PushBack( first->edges, edge );
	}
	DArray_PushBack( second->edges, edge );

	DArray_Append( self->edges, edge );
	GC_ShiftRC( first, edge->first );
	GC_ShiftRC( second, edge->second );
	edge->first = first;
	edge->second = second;
	return edge;
}

static void DaoxNode_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoxNode *self = (DaoxNode*) p;
	if( self->graph ) DArray_Append( values, self->graph );
	if( self->value ) DArray_Append( values, self->value );
	DArray_Append( arrays, self->edges );
	if( remove ) self->graph = NULL;
	if( remove ) self->value = NULL;
}
static void DaoxEdge_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoxEdge *self = (DaoxEdge*) p;
	if( self->graph ) DArray_Append( values, self->graph );
	if( self->first ) DArray_Append( values, self->first );
	if( self->second ) DArray_Append( values, self->second );
	if( self->value ) DArray_Append( values, self->value );
	if( remove ){
		self->graph = NULL;
		self->first = NULL;
		self->second = NULL;
		self->value = NULL;
	}
}
static void DaoxGraph_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoxGraph *self = (DaoxGraph*) p;
	DArray_Append( arrays, self->nodes );
	DArray_Append( arrays, self->edges );
	if( self->nodeType ) DArray_Append( values, self->nodeType );
	if( self->edgeType ) DArray_Append( values, self->edgeType );
	if( remove ){
		self->nodeType = NULL;
		self->edgeType = NULL;
	}
}


void DaoxGraph_InitUserData( DaoxGraph *self, int nodeDataSize, int edgeDataSize )
{
	daoint i, N = self->nodes->size;
	daoint M = self->edges->size;
	char *data;
	if( self->nodeData == NULL ){
		self->nodeData = DString_New(1);
		DString_SetSharing( self->nodeData, 0 );
	}
	if( self->edgeData == NULL ){
		self->edgeData = DString_New(1);
		DString_SetSharing( self->edgeData, 0 );
	}
	DString_Reserve( self->nodeData, N * nodeDataSize );
	DString_Reserve( self->edgeData, M * edgeDataSize );
	for(i=0, data=self->nodeData->mbs;  i<N;  i++, data+=nodeDataSize){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pValue[i];
		node->X.Void = data;
	}
	for(i=0, data=self->edgeData->mbs;  i<N;  i++, data+=edgeDataSize){
		DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
		edge->X.Void = data;
	}
}

/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Random Graph                                                  */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

daoint DaoxGraph_RandomInit( DaoxGraph *self, daoint N, double prob )
{
	daoint i, j, E = 0;
	if( self->nodes->size ) return 0;
	for(i=0; i<N; i++) DaoxGraph_AddNode( self );
	for(i=0; i<N; i++){
		DaoxNode *inode = (DaoxNode*) self->nodes->items.pVoid[i];
		for(j=self->directed?0:(i+1); j<N; j++){
			DaoxNode *jnode = (DaoxNode*) self->nodes->items.pVoid[j];
			double p = rand() / (RAND_MAX + 1.0);
			if( p < prob ) E += DaoxGraph_AddEdge( self, inode, jnode ) != NULL;
		}
	}
	return E;
}


/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Searching                                                     */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

/*
// DaoxNode: state, visit tag;
// They must have been set to zero before calling these methods.
*/

void DaoxNode_BreadthFirstSearch( DaoxNode *self, DArray *nodes )
{
	daoint i, j;
	DArray_Clear( nodes );
	DArray_PushBack( nodes, self );
	self->state = 1;
	for(i=0; i<nodes->size; i++){
		DaoxNode *node = (DaoxNode*) nodes->items.pVoid[i];
		for(j=0; j<node->edges->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->edges->items.pVoid[j];
			DaoxNode *node2 = edge->second;
			if( node2 == node ){ /* in-edges: */
				if( self->graph->directed ) break;
				node2 = edge->first;
			}
			if( node2->state ) continue;
			node2->state = 1;
			DArray_PushBack( nodes, node2 );
		}
	}
}

void DaoxNode_DepthFirstSearch( DaoxNode *self, DArray *nodes )
{
	DArray *stack = DArray_New(0);
	daoint j;
	DArray_Clear( nodes );
	DArray_PushBack( stack, self );
	while( stack->size ){
		DaoxNode *node = (DaoxNode*) DArray_Back( stack );
		DArray_PopBack( stack );
		if( node->state ) continue;
		node->state = 1;
		DArray_PushBack( nodes, node );
		for(j=0; j<node->edges->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->edges->items.pVoid[j];
			DaoxNode *node2 = edge->second;
			if( node2 == node ){ /* in-edges: */
				if( self->graph->directed ) break;
				node2 = edge->first;
			}
			DArray_PushBack( stack, node2 );
		}
	}
	DArray_Delete( stack );
}


/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Maximum Flow: Relabel-to-front algorithm, with FIFO heuristic */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

struct DaoxNodeMFxI
{
	daoint  height;
	daoint  excess;
};
struct DaoxEdgeMFxI
{
	daoint  capacity;
	daoint  flow_fw; /* forward flow; */
	daoint  flow_bw; /* backward flow; */
};

struct DaoxNodeMFxD
{
	daoint  height;
	double  excess;
};
struct DaoxEdgeMFxD
{
	double  capacity;
	double  flow_fw; /* forward flow; */
	double  flow_bw; /* backward flow; */
};

/* For integer type weighted network: */
static void MaxFlow_PushInt( DaoxNode *node, DaoxEdge *edge )
{
	DaoxEdgeMFxI *E = edge->X.MFxI;
	DaoxNodeMFxI *U = node->X.MFxI;
	DaoxNodeMFxI *V = edge->second->X.MFxI;
	daoint  CUV =   E->capacity;
	daoint *FUV = & E->flow_fw;
	daoint *FVU = & E->flow_bw;
	daoint send;
	if( node == edge->second ){
		V = edge->first->X.MFxI;
		CUV = 0;
		FUV = & E->flow_bw;
		FVU = & E->flow_fw;
	}
	send = CUV - (*FUV);
	if( U->excess < send ) send = U->excess;
	*FUV += send;
	*FVU -= send;
	U->excess -= send;
	V->excess += send;
}
static void MaxFlow_RelabelInt( DaoxNode *U )
{
	daoint min_height = 100 * U->graph->nodes->size;
	daoint i, n;
	for(i=0,n=U->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) U->edges->items.pValue[i];
		DaoxEdgeMFxI *EMF = edge->X.MFxI;
		if( U == edge->first ){ /* out edges */
			DaoxNode *V = edge->second;
			DaoxNodeMFxI *VMF = V->X.MFxI;
			if( (EMF->capacity > EMF->flow_fw) && (VMF->height < min_height) )
				min_height = VMF->height;
		}else{ /* in-edges */
			DaoxNode *V = edge->first;
			DaoxNodeMFxI *VMF = V->X.MFxI;
			if( (0 > EMF->flow_bw) && (VMF->height < min_height) ) min_height = VMF->height;
		}
	}
	U->X.MFxI->height = min_height + 1;
}
static void MaxFlow_DischargeInt( DaoxNode *U )
{
	daoint i, n;
	DaoxNodeMFxI *UMF = U->X.MFxI;
	while( UMF->excess > 0 ){
		for(i=0,n=U->edges->size; i<n; i++){
			DaoxEdge *edge = (DaoxEdge*) U->edges->items.pValue[i];
			DaoxEdgeMFxI *EMF = edge->X.MFxI;
			if( U == edge->first ){ /* out edges */
				DaoxNode *V = edge->second;
				DaoxNodeMFxI *VMF = V->X.MFxI;
				if( (EMF->capacity > EMF->flow_fw) && (UMF->height > VMF->height) )
					MaxFlow_PushInt( U, edge );
			}else{ /* in-edges */
				DaoxNode *V = edge->first;
				DaoxNodeMFxI *VMF = V->X.MFxI;
				if( (0 > EMF->flow_bw) && (UMF->height > VMF->height) )
					MaxFlow_PushInt( U, edge );
			}
		}
		MaxFlow_RelabelInt( U );
	}
}
daoint DaoxGraph_MaxFlow_PRTF_Int( DaoxGraph *self, DaoxNode *source, DaoxNode *sink )
{
	daoint i, n;
	daoint inf = 0;
	DArray *list = DArray_New(0);

	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) inf += edge->X.MFxI->capacity;
	}
	for(i=0,n=self->nodes->size; i<n; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pValue[i];
		node->X.MFxI->height = n;
		node->X.MFxI->excess = inf;
	}
	for(i=0,n=self->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
		edge->X.MFxI->flow_fw = 0;
		edge->X.MFxI->flow_bw = 0;
	}
	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) MaxFlow_PushInt( source, edge );
	}
	i = 0;
	while( i < list->size ){
		DaoxNode *U = (DaoxNode*) list->items.pValue[i];
		daoint old_height = U->X.MFxI->height;
		MaxFlow_DischargeInt( U );
		if( U->X.MFxI->height > old_height ){
			DArray_Erase( list, i, 1 );
			DArray_PushFront( list, U );
			i = 0;
		}else{
			i += 1;
		}
	}
	DArray_Delete( list );
	inf = 0;
	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) inf += edge->X.MFxI->flow_fw;
	}
	return inf;
}
/* For double type weighted network: */
static void MaxFlow_PushDouble( DaoxNode *node, DaoxEdge *edge )
{
	DaoxEdgeMFxD *E = edge->X.MFxD;
	DaoxNodeMFxD *U = node->X.MFxD;
	DaoxNodeMFxD *V = edge->second->X.MFxD;
	double  CUV =   E->capacity;
	double *FUV = & E->flow_fw;
	double *FVU = & E->flow_bw;
	double send;
	if( node == edge->second ){
		V = edge->first->X.MFxD;
		CUV = 0;
		FUV = & E->flow_bw;
		FVU = & E->flow_fw;
	}
	send = CUV - (*FUV);
	if( U->excess < send ) send = U->excess;
	*FUV += send;
	*FVU -= send;
	U->excess -= send;
	V->excess += send;
}
static void MaxFlow_RelabelDouble( DaoxNode *U )
{
	daoint min_height = 100 * U->graph->nodes->size;
	daoint i, n;
	for(i=0,n=U->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) U->edges->items.pValue[i];
		DaoxEdgeMFxD *EMF = edge->X.MFxD;
		if( U == edge->first ){ /* out edges */
			DaoxNode *V = edge->second;
			DaoxNodeMFxD *VMF = V->X.MFxD;
			if( (EMF->capacity > EMF->flow_fw) && (VMF->height < min_height) )
				min_height = VMF->height;
		}else{ /* in-edges */
			DaoxNode *V = edge->first;
			DaoxNodeMFxD *VMF = V->X.MFxD;
			if( (0 > EMF->flow_bw) && (VMF->height < min_height) ) min_height = VMF->height;
		}
	}
	U->X.MFxD->height = min_height + 1;
}
static void MaxFlow_DischargeDouble( DaoxNode *U )
{
	daoint i, n;
	DaoxNodeMFxD *UMF = U->X.MFxD;
	while( UMF->excess > 0 ){
		for(i=0,n=U->edges->size; i<n; i++){
			DaoxEdge *edge = (DaoxEdge*) U->edges->items.pValue[i];
			DaoxEdgeMFxD *EMF = edge->X.MFxD;
			if( U == edge->first ){ /* out edges */
				DaoxNode *V = edge->second;
				DaoxNodeMFxD *VMF = V->X.MFxD;
				if( (EMF->capacity > EMF->flow_fw) && (UMF->height > VMF->height) )
					MaxFlow_PushDouble( U, edge );
			}else{ /* in-edges */
				DaoxNode *V = edge->first;
				DaoxNodeMFxD *VMF = V->X.MFxD;
				if( (0 > EMF->flow_bw) && (UMF->height > VMF->height) )
					MaxFlow_PushDouble( U, edge );
			}
		}
		MaxFlow_RelabelDouble( U );
	}
}
double DaoxGraph_MaxFlow_PRTF_Double( DaoxGraph *self, DaoxNode *source, DaoxNode *sink )
{
	daoint i, n;
	double inf = 0.0;
	DArray *list = DArray_New(0);

	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) inf += edge->X.MFxD->capacity;
	}
	for(i=0,n=self->nodes->size; i<n; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pValue[i];
		node->X.MFxD->height = n;
		node->X.MFxD->excess = inf;
	}
	for(i=0,n=self->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
		edge->X.MFxD->flow_fw = 0.0;
		edge->X.MFxD->flow_bw = 0.0;
	}
	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) MaxFlow_PushDouble( source, edge );
	}
	i = 0;
	while( i < list->size ){
		DaoxNode *U = (DaoxNode*) list->items.pValue[i];
		daoint old_height = U->X.MFxD->height;
		MaxFlow_DischargeDouble( U );
		if( U->X.MFxD->height > old_height ){
			DArray_Erase( list, i, 1 );
			DArray_PushFront( list, U );
			i = 0;
		}else{
			i += 1;
		}
	}
	DArray_Delete( list );
	inf = 0.0;
	for(i=0,n=source->edges->size; i<n; i++){
		DaoxEdge *edge = (DaoxEdge*) source->edges->items.pValue[i];
		if( source == edge->first ) inf += edge->X.MFxD->flow_fw;
	}
	return inf;
}
double DaoxGraph_MaxFlow_PushRelabelToFront( DaoxGraph *self, DaoxNode *source, DaoxNode *sink )
{
	daoint i, n;
	switch( self->wtype ){
	case DAO_INTEGER :
		DaoxGraph_InitUserData( self, sizeof(DaoxNodeMFxI), sizeof(DaoxEdgeMFxI) );
		for(i=0, n=self->edges->size; i<n; i++){
			DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
			edge->X.MFxI->capacity = edge->W1.I;
		}
		return DaoxGraph_MaxFlow_PRTF_Int( self, source, sink );
	case DAO_FLOAT   :
	case DAO_DOUBLE  :
		DaoxGraph_InitUserData( self, sizeof(DaoxNodeMFxD), sizeof(DaoxEdgeMFxD) );
		for(i=0, n=self->edges->size; i<n; i++){
			DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
			edge->X.MFxD->capacity = edge->W1.D;
		}
		return DaoxGraph_MaxFlow_PRTF_Double( self, source, sink );
	}
	return 0;
}



/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Connected Components                                          */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

void DaoxGraph_ConnectedComponents( DaoxGraph *self, DArray *cclist )
{
	DArray *nodes;
	DaoxGraph *subgraph;
	daoint i, j, k, n;
	if( self->nodes->size == 0 ){
		DArray_PushBack( cclist, self );
		return;
	}
	for(i=0; i<self->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		node->state = 0;
	}
	nodes = DArray_New(0);
	while( self->nodes->size ){
		DaoxNode_BreadthFirstSearch( (DaoxNode*) self->nodes->items.pVoid[0], nodes );
#if 0
		printf( "self->nodes->size = %i, %i\n", self->nodes->size, nodes->size );
#endif
		if( nodes->size == self->nodes->size ){
			DArray_PushBack( cclist, self );
			break;
		}
		subgraph = DaoxGraph_New( self->ctype, self->directed );
		DArray_PushBack( cclist, subgraph );
		for(i=0,n=nodes->size; i<n; i++){
			DaoxNode *node = (DaoxNode*) nodes->items.pVoid[i];
			GC_ShiftRC( subgraph, node->graph );
			node->graph = subgraph;
			DArray_PushBack( subgraph->nodes, node );
			for(j=0; j<node->edges->size; j++){
				DaoxEdge *edge = (DaoxEdge*) node->edges->items.pVoid[j];
				if( edge->graph == subgraph ) continue;
				GC_ShiftRC( subgraph, edge->graph );
				edge->graph = subgraph;
				DArray_PushBack( subgraph->edges, edge );
			}
		}
		for(i=0,k=0,n=self->nodes->size; i<n; i++){
			DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
			/* Ensure no duplication of the reference (for the Concurrent GC): */
			self->nodes->items.pVoid[i] = NULL;
			if( node->graph != self ){
				GC_DecRC( node );
				continue;
			}
			self->nodes->items.pVoid[k++] = node;
		}
		self->nodes->size = k;
		for(i=0,k=0,n=self->edges->size; i<n; i++){
			DaoxNode *edge = (DaoxNode*) self->edges->items.pVoid[i];
			/* Ensure no duplication of the reference (for the Concurrent GC): */
			self->edges->items.pVoid[i] = NULL;
			if( edge->graph != self ){
				GC_DecRC( edge );
				continue;
			}
			self->edges->items.pVoid[k++] = edge;
		}
		self->edges->size = k;
	}
	DArray_Delete( nodes );
}


/***************************/
/***************************/
/*                         */
/* Interfaces to Dao       */
/*                         */
/***************************/
/***************************/

static void NODE_GetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER : DaoProcess_PutInteger( proc, self->U1.I ); break;
	case DAO_FLOAT   : DaoProcess_PutFloat( proc, self->U1.D ); break;
	case DAO_DOUBLE  : DaoProcess_PutDouble( proc, self->U1.D ); break;
	}
}
static void NODE_SetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER : self->U1.I = p[1]->xInteger.value; break;
	case DAO_FLOAT   : self->U1.D = p[1]->xFloat.value; break;
	case DAO_DOUBLE  : self->U1.D = p[1]->xDouble.value; break;
	}
}
static void NODE_GetWeights( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 0 );
	DaoxNode *self = (DaoxNode*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER :
		tuple->items[0]->xInteger.value = self->U1.I;
		tuple->items[1]->xInteger.value = self->U2.I;
		break;
	case DAO_FLOAT   :
		tuple->items[0]->xFloat.value = self->U1.D;
		tuple->items[1]->xFloat.value = self->U2.D;
		break;
	case DAO_DOUBLE  :
		tuple->items[0]->xDouble.value = self->U1.D;
		tuple->items[1]->xDouble.value = self->U2.D;
		break;
	}
}
static void NODE_SetWeights( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	DaoTuple *tuple = (DaoTuple*) p[1];
	switch( self->graph->wtype ){
	case DAO_INTEGER :
		self->U1.I = tuple->items[0]->xInteger.value;
		self->U2.I = tuple->items[1]->xInteger.value;
		break;
	case DAO_FLOAT   :
		self->U1.D = tuple->items[0]->xFloat.value;
		self->U2.D = tuple->items[1]->xFloat.value;
		break;
	case DAO_DOUBLE  :
		self->U1.D = tuple->items[0]->xDouble.value;
		self->U2.D = tuple->items[1]->xDouble.value;
		break;
	}
}
static void NODE_GetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	DaoProcess_PutValue( proc, self->value );
}
static void NODE_SetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode_SetValue( (DaoxNode*) p[0], p[1] );
}
static void NODE_GetEdges( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	DaoList *res = DaoProcess_PutList( proc );
	daoint i, n;
	if( p[1]->xEnum.value == 0 ){
		for(i=0,n=self->edges->size; i>0; i--){
			DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i-1];
			if( self != edge->second ) break;
			DaoList_PushBack( res, (DaoValue*) edge );
		}
	}else{
		for(i=0; i<self->edges->size; i++){
			DaoxEdge *edge = (DaoxEdge*) self->edges->items.pValue[i];
			if( self != edge->first ) break;
			DaoList_PushBack( res, (DaoValue*) edge );
		}
	}
}
static void EDGE_GetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER : DaoProcess_PutInteger( proc, self->W1.I ); break;
	case DAO_FLOAT   : DaoProcess_PutFloat( proc, self->W1.D ); break;
	case DAO_DOUBLE  : DaoProcess_PutDouble( proc, self->W1.D ); break;
	}
}
static void EDGE_SetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER : self->W1.I = p[1]->xInteger.value; break;
	case DAO_FLOAT   : self->W1.D = p[1]->xFloat.value; break;
	case DAO_DOUBLE  : self->W1.D = p[1]->xDouble.value; break;
	}
}
static void EDGE_GetWeights( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 0 );
	DaoxEdge *self = (DaoxEdge*) p[0];
	switch( self->graph->wtype ){
	case DAO_INTEGER :
		tuple->items[0]->xInteger.value = self->W1.I;
		tuple->items[1]->xInteger.value = self->W2.I;
		break;
	case DAO_FLOAT   :
		tuple->items[0]->xFloat.value = self->W1.D;
		tuple->items[1]->xFloat.value = self->W2.D;
		break;
	case DAO_DOUBLE  :
		tuple->items[0]->xDouble.value = self->W1.D;
		tuple->items[1]->xDouble.value = self->W2.D;
		break;
	}
}
static void EDGE_SetWeights( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	DaoTuple *tuple = (DaoTuple*) p[1];
	switch( self->graph->wtype ){
	case DAO_INTEGER :
		self->W1.I = tuple->items[0]->xInteger.value;
		self->W2.I = tuple->items[1]->xInteger.value;
		break;
	case DAO_FLOAT   :
		self->W1.D = tuple->items[0]->xFloat.value;
		self->W2.D = tuple->items[1]->xFloat.value;
		break;
	case DAO_DOUBLE  :
		self->W1.D = tuple->items[0]->xDouble.value;
		self->W2.D = tuple->items[1]->xDouble.value;
		break;
	}
}
static void EDGE_GetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	DaoProcess_PutValue( proc, self->value );
}
static void EDGE_SetValue( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge_SetValue( (DaoxEdge*) p[0], p[1] );
}
static void EDGE_GetNodes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	DaoTuple *res = DaoProcess_PutTuple( proc, 0 );
	DaoTuple_SetItem( res, (DaoValue*)self->first, 0 );
	DaoTuple_SetItem( res, (DaoValue*)self->second, 1 );
}
static void GRAPH_Graph( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoType *retype = DaoProcess_GetReturnType( proc );
	DaoxGraph *graph = DaoxGraph_New( retype, p[0]->xEnum.value );
	DaoValue *res = DaoProcess_PutValue( proc, (DaoValue*) graph );
}
static void GRAPH_GetNodes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoList *res = DaoProcess_PutList( proc );
	daoint i;
	for(i=0; i<self->nodes->size; i++) DaoList_PushBack( res, self->nodes->items.pValue[i] );
}
static void GRAPH_GetEdges( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoList *res = DaoProcess_PutList( proc );
	daoint i;
	for(i=0; i<self->edges->size; i++) DaoList_PushBack( res, self->edges->items.pValue[i] );
}
static void GRAPH_AddNode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *node = DaoxGraph_AddNode( (DaoxGraph*) p[0] );
	DaoProcess_PutValue( proc, (DaoValue*) node );
}
static void GRAPH_AddEdge( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *edge = DaoxGraph_AddEdge( (DaoxGraph*) p[0], (DaoxNode*) p[1], (DaoxNode*) p[2] );
	DaoProcess_PutValue( proc, (DaoValue*) edge );
}

static void GRAPH_NodeCount( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoProcess_PutInteger( proc, self->nodes->size );
}
static void GRAPH_EdgeCount( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoProcess_PutInteger( proc, self->edges->size );
}

static void GRAPH_RandomInit( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	daoint added = DaoxGraph_RandomInit( self, p[1]->xInteger.value, p[2]->xDouble.value );
	DaoProcess_PutInteger( proc, added );
}

static void GRAPH_RemoveSingletonNodes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoxGraph *save = (DaoxGraph*) DaoCdata_Cast( DaoValue_CastCdata( p[1] ), self->ctype );
	daoint i, k, n, removed = 0;
	for(i=0,k=0,n=self->nodes->size; i<n; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		/* Ensure no duplication of the reference (for the Concurrent GC): */
		self->nodes->items.pVoid[i] = NULL;
		if( node->edges->size == 0 ){
			if( save ) DArray_Append( save->nodes, node );
			GC_DecRC( node );
			continue;
		}
		self->nodes->items.pVoid[k++] = node;
	}
	DaoProcess_PutInteger( proc, self->nodes->size - k );
	self->nodes->size = k;
}

static void GRAPH_ConnectedComponents( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoList *graphs = DaoProcess_PutList( proc );
	DArray *cclist;
	daoint i, n;
	if( self->nodes->size == 0 ){
		DaoList_PushBack( graphs, (DaoValue*)self );
		return;
	}
	cclist = DArray_New(0);
	DaoxGraph_ConnectedComponents( self, cclist );
	for(i=0,n=cclist->size; i<n; i++) DaoList_PushBack( graphs, cclist->items.pValue[i] );
	DArray_Delete( cclist );
}
static void GRAPH_MaxFlow( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoxNode *source = (DaoxNode*) p[1];
	DaoxNode *sink = (DaoxNode*) p[2];
	if( self->wtype == DAO_INTEGER ){
		daoint maxflow = DaoxGraph_MaxFlow_PushRelabelToFront( self, source, sink );
		DaoProcess_PutInteger( proc, maxflow );
	}else if( self->wtype == DAO_FLOAT ){
		float maxflow = DaoxGraph_MaxFlow_PushRelabelToFront( self, source, sink );
		DaoProcess_PutFloat( proc, maxflow );
	}else{
		double maxflow = DaoxGraph_MaxFlow_PushRelabelToFront( self, source, sink );
		DaoProcess_PutDouble( proc, maxflow );
	}
}


/***************************************/
/***************************************/
/*                                     */
/* Functional or code section methods: */
/*                                     */
/***************************************/
/***************************************/

static void NODE_Search( DaoProcess *proc, DaoValue *p[], int N )
{
	DArray *nodes;
	DaoVmCode *sect = DaoGetSectionCode( proc->activeCode );
	DaoList *list = DaoProcess_PutList( proc );
	DaoxNode *self = (DaoxNode*) p[0];
	daoint method = p[1]->xEnum.value;
	daoint which = p[2]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	if( DaoProcess_PushSectionFrame( proc ) == NULL ) return;
	for(i=0; i<self->graph->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->graph->nodes->items.pVoid[i];
		node->state = 0;
	}
	nodes = DArray_New(0);
	DArray_PushBack( nodes, self );

	entry = proc->topFrame->entry;
	DaoProcess_AcquireCV( proc );
	while( nodes->size ){
		DaoxNode *node = NULL;
		if( method ){
			node = (DaoxNode*) DArray_Front( nodes );
			DArray_PopFront( nodes );
		}else{
			node = (DaoxNode*) DArray_Back( nodes );
			DArray_PopBack( nodes );
		}
		if( node->state ) continue;
		node->state = 1;

		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) node );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_VMPROC_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) node );
			if( which == 0 ) break;
		}

		for(j=0; j<node->edges->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->edges->items.pVoid[j];
			DaoxNode *node2 = edge->second;
			if( node2 == node ){ /* in-edges: */
				if( self->graph->directed ) break;
				node2 = edge->first;
			}
			DArray_PushBack( nodes, node2 );
		}
	}
	DaoProcess_PopFrame( proc );
	DaoProcess_ReleaseCV( proc );
	DArray_Delete( nodes );
}

static void GRAPH_FindNodes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoVmCode *sect = DaoGetSectionCode( proc->activeCode );
	DaoList *list = DaoProcess_PutList( proc );
	daoint which = p[1]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	if( DaoProcess_PushSectionFrame( proc ) == NULL ) return;
	entry = proc->topFrame->entry;
	DaoProcess_AcquireCV( proc );
	for(i=0; i<self->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) node );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_VMPROC_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) node );
			if( which == 0 ) break;
		}
	}
	DaoProcess_PopFrame( proc );
	DaoProcess_ReleaseCV( proc );
}
static void GRAPH_FindEdges( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoVmCode *sect = DaoGetSectionCode( proc->activeCode );
	DaoList *list = DaoProcess_PutList( proc );
	daoint which = p[1]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	if( DaoProcess_PushSectionFrame( proc ) == NULL ) return;
	entry = proc->topFrame->entry;
	DaoProcess_AcquireCV( proc );
	for(i=0; i<self->edges->size; i++){
		DaoxEdge *edge = (DaoxEdge*) self->edges->items.pVoid[i];
		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) edge );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_VMPROC_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) edge );
			if( which == 0 ) break;
		}
	}
	DaoProcess_PopFrame( proc );
	DaoProcess_ReleaseCV( proc );
}


static DaoFuncItem DaoxNodeMeths[]=
{
	{ NODE_GetWeight, "GetWeight( self :Node<@W,@N,@E> ) => @W" },
	{ NODE_SetWeight, "SetWeight( self :Node<@W,@N,@E>, weight :@W )" },
	{ NODE_GetWeights, "GetWeights( self :Node<@W,@N,@E> ) => tuple<U1:@W,U2:@W>" },
	{ NODE_SetWeights, "SetWeights( self :Node<@W,@N,@E>, weights :tuple<U1:@W,U2:@W> )" },
	{ NODE_GetValue, "GetValue( self :Node<@W,@N,@E> ) => @N" },
	{ NODE_SetValue, "SetValue( self :Node<@W,@N,@E>, value :@N )" },
	{ NODE_GetEdges, "Edges( self :Node<@W,@N,@E>, set :enum<in,out> = $out ) => list<Edge<@W,@N,@E>>" },

	{ NODE_Search, "Search( self :Node<@W,@N,@E>, method :enum<breadth,depth> = $breadth, which :enum<first,all> = $first )[node :Node<@W,@N,@E> =>int] => list<Node<@W,@N,@E>>" },
	// { NODE_Traverse, "Traverse( self :Node<@W,@N,@E>, method :enum<breadth,depth> = $breadth ) [node :Node<@W,@N,@E>]" } ,
	{ NULL, NULL }
};

DaoTypeBase DaoxNode_Typer =
{
	"Node"TYPE_PARAMS, NULL, NULL, (DaoFuncItem*) DaoxNodeMeths, {0}, {0},
	(FuncPtrDel)DaoxNode_Delete, DaoxNode_GetGCFields
};

static DaoFuncItem DaoxEdgeMeths[]=
{
	{ EDGE_GetWeight, "GetWeight( self :Edge<@W,@N,@E> ) => @W" },
	{ EDGE_SetWeight, "SetWeight( self :Edge<@W,@N,@E>, weight :@W )" },
	{ EDGE_GetWeights, "GetWeights( self :Edge<@W,@N,@E> ) => tuple<W1:@W,W2:@W>" },
	{ EDGE_SetWeights, "SetWeights( self :Edge<@W,@N,@E>, weights :tuple<W1:@W,W2:@W> )" },
	{ EDGE_GetValue, "GetValue( self :Edge<@W,@N,@E> ) => @E" },
	{ EDGE_SetValue, "SetValue( self :Edge<@W,@N,@E>, value :@E )" },
	{ EDGE_GetNodes, "Nodes( self :Edge<@W,@N,@E> ) => tuple<first:Node<@W,@N,@E>,second:Node<@W,@N,@E>>" },
	{ NULL, NULL }
};

DaoTypeBase DaoxEdge_Typer =
{
	"Edge"TYPE_PARAMS, NULL, NULL, (DaoFuncItem*) DaoxEdgeMeths, {0}, {0},
	(FuncPtrDel)DaoxEdge_Delete, DaoxEdge_GetGCFields
};


static DaoFuncItem DaoxGraphMeths[]=
{
	/* allocaters must have names identical second the typer name: */
	{ GRAPH_Graph,    "Graph"TYPE_PARAMS"( dir :enum<undirected,directed>=$undirected )" },
	{ GRAPH_GetNodes, "Nodes( self :Graph<@W,@N,@E> ) => list<Node<@W,@N,@E>>" },
	{ GRAPH_GetEdges, "Edges( self :Graph<@W,@N,@E> ) => list<Edge<@W,@N,@E>>" },
	{ GRAPH_AddNode, "AddNode( self :Graph<@W,@N,@E> ) => Node<@W,@N,@E>" },
	{ GRAPH_AddEdge, "AddEdge( self :Graph<@W,@N,@E>, first :Node<@W,@N,@E>, second :Node<@W,@N,@E> ) => Edge<@W,@N,@E>" },

	{ GRAPH_NodeCount, "NodeCount( self :Graph<@W,@N,@E> ) => int" },
	{ GRAPH_EdgeCount, "EdgeCount( self :Graph<@W,@N,@E> ) => int" },

	{ GRAPH_RandomInit, "RandomInit( self :Graph<@W,@N,@E>, N :int, P :double ) => int" },
	{ GRAPH_RemoveSingletonNodes, "RemoveSingletonNodes( self :Graph<@W,@N,@E>, save :none|Graph<@W,@N,@E> = none ) => int" },

	{ GRAPH_FindNodes, "FindNodes( self :Graph<@W,@N,@E>, which :enum<first,all> = $first )[node :Node<@W,@N,@E> =>int] => list<Node<@W,@N,@E>>" },
	{ GRAPH_FindEdges, "FindEdges( self :Graph<@W,@N,@E>, which :enum<first,all> = $first )[node :Edge<@W,@N,@E> =>int] => list<Edge<@W,@N,@E>>" },

	//{ GRAPH_Distance, "Distance( self :Graph<@W,@N,@E>, start :Node<@W,@N,@E>, end :Node<@W,@N,@E> ) => int" },
	//{ GRAPH_Distances, "Distances( self :Graph<@W,@N,@E>, start :Node<@W,@N,@E> ) => list<tuple<end:Node<@W,@N,@E>,dist:int>>" },
	//{ GRAPH_Distances, "Distances( self :Graph<@W,@N,@E> ) => list<tuple<start:Node<@W,@N,@E>,end:Node<@W,@N,@E>,dist:int>>" },

	{ GRAPH_ConnectedComponents, "ConnectedComponents( self :Graph<@W,@N,@E> ) => list<Graph<@W,@N,@E>>" },
	//{ GRAPH_MininumSpanTree, "MininumSpanTree( self :Graph<@W,@N,@E> ) => Graph<@W,Node<@W,@N,@E>,@E>" },

	{ GRAPH_MaxFlow, "MaxFlow( self :Graph<@W,@N,@E>, source :Node<@W,@N,@E>, sink :Node<@W,@N,@E> ) => @W" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraph_Typer =
{
	"Graph"TYPE_PARAMS, NULL, NULL, (DaoFuncItem*) DaoxGraphMeths, {0}, {0},
	(FuncPtrDel)DaoxGraph_Delete, DaoxGraph_GetGCFields
};

DaoType *daox_node_template_type = NULL;
DaoType *daox_edge_template_type = NULL;
DaoType *daox_graph_template_type = NULL;

int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_node_template_type = DaoNamespace_WrapType( ns, & DaoxNode_Typer, 0 );
	daox_edge_template_type = DaoNamespace_WrapType( ns, & DaoxEdge_Typer, 0 );
	daox_graph_template_type = DaoNamespace_WrapType( ns, & DaoxGraph_Typer, 0 );
	return 0;
}
