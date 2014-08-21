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

#include"math.h"
#include"dao_graph.h"
#include"daoGC.h"
#include"daoValue.h"


DaoxNode* DaoxNode_New( DaoxGraph *graph )
{
	DaoxNode *self = (DaoxNode*) dao_calloc( 1, sizeof(DaoxNode) );
	DaoCstruct_Init( (DaoCstruct*) self, graph->nodeType );
	self->graph = graph;
	self->outs = DList_New(0);
	self->weight = 1;
	return self;
}
void DaoxNode_Delete( DaoxNode *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	if( self->ins ) DList_Delete( self->ins );
	DList_Delete( self->outs );
	dao_free( self );
}
void DaoxNode_SetValue( DaoxNode *self, DaoValue *value )
{
	DaoValue_Move( value, & self->value, self->ctype->nested->items.pType[1] );
}

DaoxEdge* DaoxEdge_New( DaoxGraph *graph )
{
	DaoxEdge *self = (DaoxEdge*) dao_calloc( 1, sizeof(DaoxEdge) );
	DaoCstruct_Init( (DaoCstruct*) self, graph->edgeType );
	self->graph = graph;
	self->weight = 1;
	return self;
}
void DaoxEdge_Delete( DaoxEdge *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoxEdge_SetValue( DaoxEdge *self, DaoValue *value )
{
	DaoValue_Move( value, & self->value, self->ctype->nested->items.pType[1] );
}

DaoxGraph* DaoxGraph_New( DaoType *type, int directed )
{
	DaoxGraph *self = (DaoxGraph*) dao_calloc( 1, sizeof(DaoxGraph) );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->nodes = DList_New(DAO_DATA_VALUE);
	self->edges = DList_New(DAO_DATA_VALUE);
	self->directed = directed;
	self->nodeType = NULL;
	self->edgeType = NULL;
	if( type ){
		DaoType **types = type->nested->items.pType;
		daoint count = type->nested->size;
		self->nodeType = DaoType_Specialize( daox_node_template_type, types, count );
		self->edgeType = DaoType_Specialize( daox_edge_template_type, types, count );
		GC_IncRC( self->nodeType );
		GC_IncRC( self->edgeType );
	}
	return self;
}
void DaoxGraph_Delete( DaoxGraph *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->nodes );
	DList_Delete( self->edges );
	GC_DecRC( self->nodeType );
	GC_DecRC( self->edgeType );
	dao_free( self );
}

DaoxNode* DaoxGraph_AddNode( DaoxGraph *self )
{
	DaoxNode *node = DaoxNode_New( self );
	DList_Append( self->nodes, node );
	return node;
}
DaoxEdge* DaoxGraph_AddEdge( DaoxGraph *self, DaoxNode *first, DaoxNode *second )
{
	DaoxEdge *edge = DaoxEdge_New( self );
	DList_PushFront( first->outs, edge );
	if( self->directed ){
		if( second->ins == NULL ) second->ins = DList_New(DAO_DATA_VALUE);
		DList_PushBack( second->ins, edge );
	}else{
		DList_PushBack( second->outs, edge );
	}

	DList_Append( self->edges, edge );
	edge->first = first;
	edge->second = second;
	return edge;
}

static void DaoxGraph_GetGCFields( void *p, DList *values, DList *arrays, DList *maps, int remove )
{
	daoint i, n;
	DaoxGraph *self = (DaoxGraph*) p;
	DList_Append( arrays, self->nodes );
	DList_Append( arrays, self->edges );
	if( self->nodeType ) DList_Append( values, self->nodeType );
	if( self->edgeType ) DList_Append( values, self->edgeType );
	if( remove ){
		self->nodeType = NULL;
		self->edgeType = NULL;
		for(i=0,n=self->nodes->size; i<n; i++){
			DaoxNode *node = self->nodes->items.pgNode[i];
			if( node->ins ) DList_Clear( node->ins );
			DList_Clear( node->outs );
			node->graph = NULL;
		}
		for(i=0,n=self->edges->size; i<n; i++){
			DaoxEdge *edge = self->edges->items.pgEdge[i];
			edge->graph = NULL;
			edge->first = NULL;
			edge->second = NULL;
		}
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

void DaoxNode_BreadthFirstSearch( DaoxNode *self, DList *nodes )
{
	daoint i, j;
	DList_Clear( nodes );
	DList_PushBack( nodes, self );
	self->state = 1;
	for(i=0; i<nodes->size; i++){
		DaoxNode *node = (DaoxNode*) nodes->items.pVoid[i];
		for(j=0; j<node->outs->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->outs->items.pVoid[j];
			DaoxNode *node2 = node == edge->first ? edge->second : edge->first;
			if( node2->state ) continue;
			node2->state = 1;
			DList_PushBack( nodes, node2 );
		}
	}
}

void DaoxNode_DepthFirstSearch( DaoxNode *self, DList *nodes )
{
	DList *stack = DList_New(0);
	daoint j;
	DList_Clear( nodes );
	DList_PushBack( stack, self );
	while( stack->size ){
		DaoxNode *node = (DaoxNode*) DList_Back( stack );
		DList_PopBack( stack );
		if( node->state ) continue;
		node->state = 1;
		DList_PushBack( nodes, node );
		for(j=0; j<node->outs->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->outs->items.pVoid[j];
			DaoxNode *node2 = node == edge->first ? edge->second : edge->first;
			DList_PushBack( stack, node2 );
		}
	}
	DList_Delete( stack );
}



/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Connected Components                                          */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

void DaoxGraph_ConnectedComponents( DaoxGraph *self, DList *cclist )
{
	DList *nodes;
	DaoxGraph *subgraph;
	daoint i, j, k, n;
	if( self->nodes->size == 0 ){
		DList_PushBack( cclist, self );
		return;
	}
	for(i=0; i<self->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		node->state = 0;
	}
	nodes = DList_New(0);
	while( self->nodes->size ){
		DaoxNode_BreadthFirstSearch( (DaoxNode*) self->nodes->items.pVoid[0], nodes );
#if 0
		printf( "self->nodes->size = %i, %i\n", self->nodes->size, nodes->size );
#endif
		if( nodes->size == self->nodes->size ){
			DList_PushBack( cclist, self );
			break;
		}
		subgraph = DaoxGraph_New( self->ctype, self->directed );
		DList_PushBack( cclist, subgraph );
		for(i=0,n=nodes->size; i<n; i++){
			DaoxNode *node = (DaoxNode*) nodes->items.pVoid[i];
			GC_Assign( & node->graph, subgraph );
			DList_PushBack( subgraph->nodes, node );
			for(j=0; j<node->outs->size; j++){
				DaoxEdge *edge = (DaoxEdge*) node->outs->items.pVoid[j];
				if( edge->graph == subgraph ) continue;
				GC_Assign( & edge->graph, subgraph );
				DList_PushBack( subgraph->edges, edge );
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
	DList_Delete( nodes );
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
	DaoProcess_PutDouble( proc, self->weight );
}
static void NODE_SetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxNode *self = (DaoxNode*) p[0];
	self->weight = p[1]->xDouble.value;;
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
	if( self->graph->directed && p[1]->xEnum.value == 0 ){
		for(i=0,n=self->ins->size; i<n; i++) DaoList_PushBack( res, self->ins->items.pValue[i] );
	}else{
		for(i=0,n=self->outs->size; i<n; i++) DaoList_PushBack( res, self->outs->items.pValue[i] );
	}
}
static void EDGE_GetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	DaoProcess_PutDouble( proc, self->weight );
}
static void EDGE_SetWeight( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxEdge *self = (DaoxEdge*) p[0];
	self->weight = p[1]->xDouble.value;;
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
	DaoxGraph *save = (DaoxGraph*) p[1];
	daoint i, k, n, removed = 0;
	for(i=0,k=0,n=self->nodes->size; i<n; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		/* Ensure no duplication of the reference (for the Concurrent GC): */
		self->nodes->items.pVoid[i] = NULL;
		if( (node->ins->size + node->outs->size) == 0 ){
			if( save ) DList_Append( save->nodes, node );
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
	DList *cclist;
	daoint i, n;
	if( self->nodes->size == 0 ){
		DaoList_PushBack( graphs, (DaoValue*)self );
		return;
	}
	cclist = DList_New(0);
	DaoxGraph_ConnectedComponents( self, cclist );
	for(i=0,n=cclist->size; i<n; i++) DaoList_PushBack( graphs, cclist->items.pValue[i] );
	DList_Delete( cclist );
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
	DList *nodes;
	DaoList *list = DaoProcess_PutList( proc );
	DaoxNode *self = (DaoxNode*) p[0];
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	daoint method = p[1]->xEnum.value;
	daoint which = p[2]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	for(i=0; i<self->graph->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->graph->nodes->items.pVoid[i];
		node->state = 0;
	}
	nodes = DList_New(0);
	DList_PushBack( nodes, self );

	entry = proc->topFrame->entry;
	while( nodes->size ){
		DaoxNode *node = NULL;
		if( method ){
			node = (DaoxNode*) DList_Front( nodes );
			DList_PopFront( nodes );
		}else{
			node = (DaoxNode*) DList_Back( nodes );
			DList_PopBack( nodes );
		}
		if( node->state ) continue;
		node->state = 1;

		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) node );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_PROCESS_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) node );
			if( which == 0 ) break;
		}

		for(j=0; j<node->outs->size; j++){
			DaoxEdge *edge = (DaoxEdge*) node->outs->items.pVoid[j];
			DaoxNode *node2 = node == edge->first ? edge->second : edge->first;
			DList_PushBack( nodes, node2 );
		}
	}
	DaoProcess_PopFrame( proc );
	DList_Delete( nodes );
}

static void GRAPH_FindNodes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoList *list = DaoProcess_PutList( proc );
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	daoint which = p[1]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	entry = proc->topFrame->entry;
	for(i=0; i<self->nodes->size; i++){
		DaoxNode *node = (DaoxNode*) self->nodes->items.pVoid[i];
		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) node );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_PROCESS_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) node );
			if( which == 0 ) break;
		}
	}
	DaoProcess_PopFrame( proc );
}
static void GRAPH_FindEdges( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraph *self = (DaoxGraph*) p[0];
	DaoList *list = DaoProcess_PutList( proc );
	DaoVmCode *sect = DaoProcess_InitCodeSection( proc, 1 );
	daoint which = p[1]->xEnum.value;
	daoint i, j, entry;

	if( sect == NULL ) return;
	entry = proc->topFrame->entry;
	for(i=0; i<self->edges->size; i++){
		DaoxEdge *edge = (DaoxEdge*) self->edges->items.pVoid[i];
		if( sect->b >0 ) DaoProcess_SetValue( proc, sect->a, (DaoValue*) edge );
		proc->topFrame->entry = entry;
		DaoProcess_Execute( proc );
		if( proc->status == DAO_PROCESS_ABORTED ) break;
		if( proc->stackValues[0]->xInteger.value ){
			DaoList_PushBack( list, (DaoValue*) edge );
			if( which == 0 ) break;
		}
	}
	DaoProcess_PopFrame( proc );
}


static DaoFuncItem DaoxNodeMeths[]=
{
	{ NODE_GetWeight, "GetWeight( self :Node<@N,@E> ) => double" },
	{ NODE_SetWeight, "SetWeight( self :Node<@N,@E>, weight :double )" },
	{ NODE_GetValue, "GetValue( self :Node<@N,@E> ) => @N" },
	{ NODE_SetValue, "SetValue( self :Node<@N,@E>, value :@N )" },
	{ NODE_GetEdges, "Edges( self :Node<@N,@E>, set :enum<in,out> = $out ) => list<Edge<@N,@E>>" },

	{ NODE_Search, "Search( self :Node<@N,@E>, method :enum<breadth,depth> = $breadth, which :enum<first,all> = $first )[node :Node<@N,@E> =>int] => list<Node<@N,@E>>" },
	// { NODE_Traverse, "Traverse( self :Node<@N,@E>, method :enum<breadth,depth> = $breadth ) [node :Node<@N,@E>]" } ,
	{ NULL, NULL }
};

DaoTypeBase DaoxNode_Typer =
{
	"Node<@N=none,@E=none>", NULL, NULL, (DaoFuncItem*) DaoxNodeMeths, {0}, {0},
	(FuncPtrDel)DaoxNode_Delete, NULL
};

static DaoFuncItem DaoxEdgeMeths[]=
{
	{ EDGE_GetWeight, "GetWeight( self :Edge<@N,@E> ) => double" },
	{ EDGE_SetWeight, "SetWeight( self :Edge<@N,@E>, weight :double )" },
	{ EDGE_GetValue, "GetValue( self :Edge<@N,@E> ) => @E" },
	{ EDGE_SetValue, "SetValue( self :Edge<@N,@E>, value :@E )" },
	{ EDGE_GetNodes, "Nodes( self :Edge<@N,@E> ) => tuple<first:Node<@N,@E>,second:Node<@N,@E>>" },
	{ NULL, NULL }
};

DaoTypeBase DaoxEdge_Typer =
{
	"Edge<@N=none,@E=none>", NULL, NULL, (DaoFuncItem*) DaoxEdgeMeths, {0}, {0},
	(FuncPtrDel)DaoxEdge_Delete, NULL
};


static DaoFuncItem DaoxGraphMeths[]=
{
	/* allocaters must have names identical second the typer name: */
	{ GRAPH_Graph,    "Graph<@N,@E>( dir :enum<undirected,directed>=$undirected )" },
	{ GRAPH_GetNodes, "Nodes( self :Graph<@N,@E> ) => list<Node<@N,@E>>" },
	{ GRAPH_GetEdges, "Edges( self :Graph<@N,@E> ) => list<Edge<@N,@E>>" },
	{ GRAPH_AddNode, "AddNode( self :Graph<@N,@E> ) => Node<@N,@E>" },
	{ GRAPH_AddEdge, "AddEdge( self :Graph<@N,@E>, first :Node<@N,@E>, second :Node<@N,@E> ) => Edge<@N,@E>" },

	{ GRAPH_NodeCount, "NodeCount( self :Graph<@N,@E> ) => int" },
	{ GRAPH_EdgeCount, "EdgeCount( self :Graph<@N,@E> ) => int" },

	{ GRAPH_RandomInit, "RandomInit( self :Graph<@N,@E>, N :int, P :double ) => int" },
	{ GRAPH_RemoveSingletonNodes, "RemoveSingletonNodes( self :Graph<@N,@E>, save :none|Graph<@N,@E> = none ) => int" },

	{ GRAPH_FindNodes, "FindNodes( self :Graph<@N,@E>, which :enum<first,all> = $first )[node :Node<@N,@E> =>int] => list<Node<@N,@E>>" },
	{ GRAPH_FindEdges, "FindEdges( self :Graph<@N,@E>, which :enum<first,all> = $first )[node :Edge<@N,@E> =>int] => list<Edge<@N,@E>>" },

	//{ GRAPH_Distance, "Distance( self :Graph<@N,@E>, start :Node<@N,@E>, end :Node<@N,@E> ) => int" },
	//{ GRAPH_Distances, "Distances( self :Graph<@N,@E>, start :Node<@N,@E> ) => list<tuple<end:Node<@N,@E>,dist:int>>" },
	//{ GRAPH_Distances, "Distances( self :Graph<@N,@E> ) => list<tuple<start:Node<@N,@E>,end:Node<@N,@E>,dist:int>>" },

	{ GRAPH_ConnectedComponents, "ConnectedComponents( self :Graph<@N,@E> ) => list<Graph<@N,@E>>" },
	//{ GRAPH_MininumSpanTree, "MininumSpanTree( self :Graph<@N,@E> ) => Graph<Node<@N,@E>,@E>" },

	{ NULL, NULL }
};


/* @N: type of user data for nodes; */
/* @E: type of user data for edges; */

DaoTypeBase DaoxGraph_Typer =
{
	"Graph<@N=none,@E=none>", NULL, NULL, (DaoFuncItem*) DaoxGraphMeths, {0}, {0},
	(FuncPtrDel)DaoxGraph_Delete, DaoxGraph_GetGCFields
};



/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Graph Algorithm Data                                          */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

static void GD_GetGraph( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphData *self = (DaoxGraphData*) p[0];
	DaoProcess_PutValue( proc, self->graph ? (DaoValue*) self->graph : DaoValue_MakeNone() );
}
static DaoFuncItem DaoxGraphDataMeths[]=
{
	{ GD_GetGraph, "GetGraph( self :GraphData ) => none|Graph<any,any>" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphData_Typer =
{
	"GraphData", NULL, NULL, (DaoFuncItem*) DaoxGraphDataMeths, {0}, {0}, NULL, NULL
};

void DaoxGraphData_Init( DaoxGraphData *self, DaoType *type )
{
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->graph = NULL;
	self->edgeData = DString_New();
	self->nodeData = DString_New();
	DString_SetSharing( self->nodeData, 0 );
	DString_SetSharing( self->edgeData, 0 );
}
void DaoxGraphData_Clear( DaoxGraphData *self )
{
	DaoxGraphData_Reset( self, NULL, 0, 0 );
	DString_Delete( self->nodeData );
	DString_Delete( self->edgeData );
	self->edgeData = NULL;
	self->edgeData = NULL;
}
void DaoxGraphData_Reset( DaoxGraphData *self, DaoxGraph *graph, int nodeSize, int edgeSize )
{
	daoint i, M, N;
	char *data;

	if( self->graph ){
		N = self->graph->nodes->size;
		M = self->graph->edges->size;
		for(i=0; i<N; i++){
			DaoxNode *node = self->graph->nodes->items.pgNode[i];
			node->X.Void = NULL;
		}
		for(i=0; i<N; i++){
			DaoxEdge *edge = self->graph->edges->items.pgEdge[i];
			edge->X.Void = NULL;
		}
	}
	GC_Assign( & self->graph, graph );
	if( graph == NULL ) return;

	N = graph->nodes->size;
	M = graph->edges->size;
	DString_Reserve( self->nodeData, N * nodeSize );
	DString_Reserve( self->edgeData, M * edgeSize );
	for(i=0, data=self->nodeData->chars;  i<N;  i++, data+=nodeSize){
		DaoxNode *node = graph->nodes->items.pgNode[i];
		node->X.Void = data;
	}
	for(i=0, data=self->edgeData->chars;  i<M;  i++, data+=edgeSize){
		DaoxEdge *edge = graph->edges->items.pgEdge[i];
		edge->X.Void = data;
	}
}
void DaoxGraphData_GetGCFields( void *p, DList *values, DList *arrays, DList *maps, int remove )
{
	DaoxGraphData *self = (DaoxGraphData*) p;
	if( self->graph ) DList_Append( values, self->graph );
	if( remove ) self->graph = NULL;
}
int DaoxGraphData_IsAssociated( DaoxGraphData *self, DaoxGraph *graph, DaoProcess *proc )
{
	if( self->graph == graph ) return 1;
	DaoProcess_RaiseError( proc, NULL, "graph is not associated with the algorithm data!" );
	return 0;
}



/*****************************************************************/
/*****************************************************************/
/*                                                               */
/* Maximum Flow                                                  */
/*                                                               */
/*****************************************************************/
/*****************************************************************/

static void GMF_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow* GMF = DaoxGraphMaxFlow_New();
	DaoProcess_PutValue( proc, (DaoValue*) GMF );
}
static void GMF_Init( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoxGraph *graph = (DaoxGraph*) p[1];
	DaoxGraphMaxFlow_Init( self, graph );
}
static void GMF_Compute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoxNode *source = (DaoxNode*) p[1];
	DaoxNode *sink = (DaoxNode*) p[2];
	if( DaoxGraphData_IsAssociated( (DaoxGraphData*)self, source->graph, proc ) == 0 ) return;
	if( DaoxGraphData_IsAssociated( (DaoxGraphData*)self, sink->graph, proc ) == 0 ) return;
	int error = DaoxGraphMaxFlow_Compute( self, source, sink );
	DaoProcess_PutInteger( proc, error );
}
static void GMF_SetCapacity( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoxEdge *edge = (DaoxEdge*) p[1];
	if( DaoxGraphData_IsAssociated( (DaoxGraphData*)self, edge->graph, proc ) == 0 ) return;
	edge->X.MF->capacity = p[2]->xDouble.value;
}
static void GMF_GetCapacity( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoxEdge *edge = (DaoxEdge*) p[1];
	if( DaoxGraphData_IsAssociated( (DaoxGraphData*)self, edge->graph, proc ) == 0 ) return;
	DaoProcess_PutDouble( proc, edge->X.MF->capacity );
}
static void GMF_GetEdgeFlow( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoxEdge *edge = (DaoxEdge*) p[1];
	if( DaoxGraphData_IsAssociated( (DaoxGraphData*)self, edge->graph, proc ) == 0 ) return;
	DaoProcess_PutDouble( proc, edge->X.MF->flow_fw );
}
static void GMF_GetGraphFlow( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) p[0];
	DaoProcess_PutDouble( proc, self->maxflow );
}
static DaoFuncItem DaoxGraphMFMeths[]=
{
	{ GMF_New,     "GraphMaxFlow()" },
	{ GMF_Init,    "Init( self :GraphMaxFlow, graph :Graph<@N,@E> )" },
	{ GMF_Compute, "Compute( self :GraphMaxFlow, source :Node<@N,@E>, sink :Node<@N,@E> ) => int" },
	{ GMF_SetCapacity, "SetCapacity( self :GraphMaxFlow, edge :Edge<@N,@E>, capacity :double )" },
	{ GMF_GetCapacity, "GetCapacity( self :GraphMaxFlow, edge :Edge<@N,@E> ) => double" },
	{ GMF_GetEdgeFlow,  "GetFlow( self :GraphMaxFlow, edge :Edge<@N,@E> ) => double" },
	{ GMF_GetGraphFlow, "GetFlow( self :GraphMaxFlow ) => double" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphMaxFlow_Typer =
{
	"GraphMaxFlow", NULL, NULL, (DaoFuncItem*) DaoxGraphMFMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphMaxFlow_Delete, DaoxGraphData_GetGCFields
};

DaoxGraphMaxFlow* DaoxGraphMaxFlow_New()
{
	DaoxGraphMaxFlow *self = (DaoxGraphMaxFlow*) dao_calloc( 1, sizeof(DaoxGraphMaxFlow) );
	DaoxGraphData_Init( (DaoxGraphData*) self, daox_graph_maxflow_type );
	self->maxflow = 0.0;
	return self;
}
void DaoxGraphMaxFlow_Delete( DaoxGraphMaxFlow *self )
{
	DaoxGraphData_Clear( (DaoxGraphData*) self );
	dao_free( self );
}
void DaoxGraphMaxFlow_Init( DaoxGraphMaxFlow *self, DaoxGraph *graph )
{
	daoint i, n;
	DaoxGraphData_Reset( (DaoxGraphData*) self, graph, sizeof(DaoxNodeMF), sizeof(DaoxEdgeMF) );
	for(i=0, n=graph->edges->size; i<n; i++){
		DaoxEdge *edge = graph->edges->items.pgEdge[i];
		edge->X.MF->capacity = edge->weight;
	}
}
static double DaoxGraph_MaxFlow_PRTF_Double( DaoxGraph *self, DaoxNode *source, DaoxNode *sink );
int DaoxGraphMaxFlow_Compute( DaoxGraphMaxFlow *self, DaoxNode *source, DaoxNode *sink )
{
	DaoxGraph *graph = self->graph;
	if( graph == NULL || source->graph != graph || sink->graph != graph ) return 0;
	self->maxflow = DaoxGraph_MaxFlow_PRTF_Double( graph, source, sink );
	return 0;
}

/*****************************************************************/
/* Maximum Flow: Relabel-to-front algorithm, with FIFO heuristic */
/*****************************************************************/

static void MaxFlow_PushDouble( DaoxNode *node, DaoxEdge *edge )
{
	DaoxEdgeMF *E = edge->X.MF;
	DaoxNodeMF *U = node->X.MF;
	DaoxNodeMF *V = edge->second->X.MF;
	double  CUV =   E->capacity;
	double *FUV = & E->flow_fw;
	double *FVU = & E->flow_bw;
	double send;
	if( node == edge->second ){
		V = edge->first->X.MF;
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
	for(i=0,n=U->outs->size; i<n; i++){
		DaoxEdge *E = U->outs->items.pgEdge[i];
		DaoxNode *V = U == E->first ? E->second : E->first;
		DaoxEdgeMF *EMF = E->X.MF;
		DaoxNodeMF *VMF = V->X.MF;
		if( (EMF->capacity > EMF->flow_fw) && (VMF->height < min_height) ){
			min_height = VMF->height;
			U->X.MF->height = min_height + 1;
		}
	}
	for(i=0,n=U->ins->size; i<n; i++){
		DaoxEdge *E = U->ins->items.pgEdge[i];
		DaoxEdgeMF *EMF = E->X.MF;
		DaoxNodeMF *VMF = E->first->X.MF;
		if( (0 > EMF->flow_bw) && (VMF->height < min_height) ){
			min_height = VMF->height;
			U->X.MF->height = min_height + 1;
		}
	}
}
static void MaxFlow_DischargeDouble( DaoxNode *U )
{
	daoint i, n;
	DaoxNodeMF *UMF = U->X.MF;
	while( UMF->excess > 0 ){
		if( UMF->nextpush < U->outs->size ){
			DaoxEdge *E = U->outs->items.pgEdge[UMF->nextpush];
			DaoxNode *V = U == E->first ? E->second : E->first;
			DaoxEdgeMF *EMF = E->X.MF;
			DaoxNodeMF *VMF = V->X.MF;
			if( (EMF->capacity > EMF->flow_fw) && (UMF->height > VMF->height) ){
				MaxFlow_PushDouble( U, E );
			}else{
				UMF->nextpush += 1;
			}
		}else if( UMF->nextpush < (U->outs->size + U->ins->size) ){
			DaoxEdge *E = U->ins->items.pgEdge[UMF->nextpush - U->outs->size];
			DaoxEdgeMF *EMF = E->X.MF;
			DaoxNodeMF *VMF = E->first->X.MF;
			if( (0 > EMF->flow_bw) && (UMF->height > VMF->height) ){
				MaxFlow_PushDouble( U, E );
			}else{
				UMF->nextpush += 1;
			}
		}else{
			MaxFlow_RelabelDouble( U );
			UMF->nextpush = 0;
		}
	}
}
static double DaoxGraph_MaxFlow_PRTF_Double( DaoxGraph *self, DaoxNode *source, DaoxNode *sink )
{
	daoint i, n;
	double inf = 1.0;
	DList *list = DList_New(0);

	for(i=0,n=source->outs->size; i<n; i++){
		DaoxEdge *edge = source->outs->items.pgEdge[i];
		if( source == edge->first ) inf += edge->X.MF->capacity;
	}
	for(i=0,n=self->nodes->size; i<n; i++){
		DaoxNode *node = self->nodes->items.pgNode[i];
		node->X.MF->nextpush = 0;
		node->X.MF->height = 0;
		node->X.MF->excess = 0.0;
		if( node != source && node != sink ) DList_PushBack( list, node );
	}
	source->X.MF->nextpush = 0;
	source->X.MF->height = n;
	source->X.MF->excess = inf;
	for(i=0,n=self->edges->size; i<n; i++){
		DaoxEdge *edge = self->edges->items.pgEdge[i];
		edge->X.MF->flow_fw = 0.0;
		edge->X.MF->flow_bw = 0.0;
	}
	for(i=0,n=source->outs->size; i<n; i++){
		DaoxEdge *edge = source->outs->items.pgEdge[i];
		if( source == edge->first ) MaxFlow_PushDouble( source, edge );
	}
	i = 0;
	while( i < list->size ){
		DaoxNode *U = list->items.pgNode[i];
		daoint old_height = U->X.MF->height;
		MaxFlow_DischargeDouble( U );
		if( U->X.MF->height > old_height ){
			DList_Erase( list, i, 1 );
			DList_PushFront( list, U );
			i = 0;
		}else{
			i += 1;
		}
	}
	DList_Delete( list );
	inf = 0.0;
	for(i=0,n=source->outs->size; i<n; i++){
		DaoxEdge *edge = source->outs->items.pgEdge[i];
		if( source == edge->first ) inf += edge->X.MF->flow_fw;
	}
	return inf;
}




DaoType *daox_node_template_type = NULL;
DaoType *daox_edge_template_type = NULL;
DaoType *daox_graph_template_type = NULL;
DaoType *daox_graph_data_type = NULL;
DaoType *daox_graph_maxflow_type = NULL;

DAO_DLL int DaoGraph_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_node_template_type = DaoNamespace_WrapType( ns, & DaoxNode_Typer, 0 );
	daox_edge_template_type = DaoNamespace_WrapType( ns, & DaoxEdge_Typer, 0 );
	daox_graph_template_type = DaoNamespace_WrapType( ns, & DaoxGraph_Typer, 0 );
	daox_graph_data_type    = DaoNamespace_WrapType( ns, & DaoxGraphData_Typer, 0 );
	daox_graph_maxflow_type = DaoNamespace_WrapType( ns, & DaoxGraphMaxFlow_Typer, 0 );
	return 0;
}
