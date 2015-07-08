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
#include<assert.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<stack>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h" // "llvm/Support/TargetSelect.h" cause error!
#include "llvm/IR/DataLayout.h"
#include "llvm/PassManager.h"
#include "llvm/IR/IRPrintingPasses.h"

#include "daoJIT.h"

LLVMContext     *llvm_context = NULL;
Module          *llvm_module = NULL;
ExecutionEngine *llvm_exe_engine = NULL;
FunctionPassManager *llvm_func_optimizer = NULL;

Type *cxx_number_types[DAO_COMPLEX] = {0};
PointerType *daojit_number_types[DAO_COMPLEX] = {0};
PointerType *daojit_array_types[DAO_COMPLEX] = {0};

Type *int1_type = NULL;
Type *int8_type = NULL;
Type *int16_type = NULL;
Type *int32_type = NULL;
Type *int32_type_p = NULL;
//Type *int64_type = NULL;
Type *daoint_type = NULL; // 32 or 64 bits
Type *double_type = NULL;
Type *size_t_type = NULL;

Type       *dao_boolean_type = NULL;
Type       *dao_integer_type = NULL;
Type       *dao_float_type = NULL;
StructType *dao_complex_type = NULL;

VectorType *int8_vector2 = NULL;

Type *void_type = NULL; // void

ArrayType *daoint_array_type = NULL; // 32 or 64 bits
ArrayType *dao_boolean_array_type = NULL; // 8 bits
ArrayType *dao_integer_array_type = NULL; // 64 bits
ArrayType *dao_float_array_type = NULL;
ArrayType *dao_complex_array_type = NULL;

PointerType *daoint_array_type_p = NULL; // 32 or 64 bits
PointerType *dao_boolean_array_type_p = NULL; // 8 bits
PointerType *dao_integer_array_type_p = NULL; // 64 bits
PointerType *dao_float_array_type_p = NULL;
PointerType *dao_complex_array_type_p = NULL;

StructType *dstring_type = NULL; // DString
PointerType *dstring_type_p = NULL; // DString*
PointerType *dstring_type_pp = NULL; // DString**

StructType *darray_value_type = NULL; // DArray<DaoValue*>
StructType *darray_constant_type = NULL; // DArray<DaoConstant*>
StructType *darray_variable_type = NULL; // DArray<DaoVariable*>
PointerType *darray_value_type_p = NULL; // DArray<DaoValue*>*
PointerType *darray_constant_type_p = NULL; // DArray<DaoConstant*>*
PointerType *darray_variable_type_p = NULL; // DArray<DaoVariable*>*

StructType *darray_class_type = NULL; // DArray<DaoClass*>
PointerType *darray_class_type_p = NULL; // DArray<DaoClass*>*

StructType *darray_process_type = NULL; // DArray<DaoProcess*>
PointerType *darray_process_type_p = NULL; // DArray<DaoProcess*>*

StructType *darray_namespace_type = NULL; // DArray<DaoNamespace*>
PointerType *darray_namespace_type_p = NULL; // DArray<DaoNamespace*>*

StructType *daojit_constant_type = NULL; // DaoConstant
StructType *daojit_variable_type = NULL; // DaoVariable
StructType *daojit_value_type = NULL; // DaoValue
StructType *daojit_integer_type = NULL; // DaoInteger
StructType *daojit_float_type = NULL; // DaoFloat
StructType *daojit_complex_type = NULL; // DaoComplex
StructType *daojit_string_type = NULL; // DaoString
StructType *daojit_enum_type = NULL; // DaoEnum
StructType *daojit_list_type = NULL; // DaoList
StructType *daojit_tuple_type = NULL; // DaoTuple
StructType *daojit_array_b_type = NULL; // DaoArray
StructType *daojit_array_i_type = NULL; // DaoArray
StructType *daojit_array_f_type = NULL; // DaoArray
StructType *daojit_array_c_type = NULL; // DaoArray
StructType *daojit_class_type = NULL; // DaoClass
StructType *daojit_object_type = NULL; // DaoObject
StructType *daojit_process_type = NULL; // DaoProcess
StructType *daojit_namespace_type = NULL; // DaoNamespace
StructType *daojit_type_type = NULL; // DaoType

PointerType *void_type_p = NULL; // i8*

PointerType *daojit_constant_type_p = NULL; // DaoConstant*
PointerType *daojit_variable_type_p = NULL; // DaoVariable*
PointerType *daojit_value_type_p = NULL; // DaoValue*
PointerType *daojit_integer_type_p = NULL; // DaoInteger*
PointerType *daojit_float_type_p = NULL; // DaoFloat*
PointerType *daojit_complex_type_p = NULL; // DaoComplex*
PointerType *daojit_string_type_p = NULL; // DaoString*
PointerType *daojit_enum_type_p = NULL; // DaoEnum*
PointerType *daojit_list_type_p = NULL; // DaoList*
PointerType *daojit_tuple_type_p = NULL; // DaoTuple*
PointerType *daojit_array_b_type_p = NULL; // DaoArray*
PointerType *daojit_array_i_type_p = NULL; // DaoArray*
PointerType *daojit_array_f_type_p = NULL; // DaoArray*
PointerType *daojit_array_c_type_p = NULL; // DaoArray*
PointerType *daojit_class_type_p = NULL; // DaoClass*
PointerType *daojit_object_type_p = NULL; // DaoObject*
PointerType *daojit_process_type_p = NULL; // DaoProcess*
PointerType *daojit_namespace_type_p = NULL; // DaoNamespace*
PointerType *daojit_type_type_p = NULL; // DaoType*

PointerType *daojit_value_type_pp = NULL; // DaoValue**
PointerType *daojit_string_type_pp = NULL; // DaoString**
PointerType *daojit_enum_type_pp = NULL; // DaoEnum**
PointerType *daojit_list_type_pp = NULL; // DaoList**
PointerType *daojit_tuple_type_pp = NULL; // DaoTuple**
PointerType *daojit_class_type_pp = NULL; // DaoClass**
PointerType *daojit_object_type_pp = NULL; // DaoObject**
PointerType *daojit_process_type_pp = NULL; // DaoProcess**
PointerType *daojit_namespace_type_pp = NULL; // DaoNamespace**

ArrayType *daojit_value_ptr_array_type = NULL; // DaoValue*[]
ArrayType *daojit_constant_ptr_array_type = NULL; // DaoConstant*[]
ArrayType *daojit_variable_ptr_array_type = NULL; // DaoVariable*[]
PointerType *daojit_value_ptr_array_type_p = NULL; // DaoValue*[]*
PointerType *daojit_constant_ptr_array_type_p = NULL; // DaoConstant*[]*
PointerType *daojit_variable_ptr_array_type_p = NULL; // DaoVariable*[]*

ArrayType *daojit_type_ptr_array_type = NULL; // DaoType*[]
PointerType *daojit_type_ptr_array_type_p = NULL; // DaoType*[]*

ArrayType *daojit_class_ptr_array_type = NULL; // DaoClass*[]
PointerType *daojit_class_ptr_array_type_p = NULL; // DaoClass*[]*

ArrayType *daojit_process_ptr_array_type = NULL; // DaoProcess*[]
PointerType *daojit_process_ptr_array_type_p = NULL; // DaoProcess*[]*

ArrayType *daojit_namespace_ptr_array_type = NULL; // DaoNamespace*[]
PointerType *daojit_namespace_ptr_array_type_p = NULL; // DaoNamespace*[]*

StructType *daojit_call_data_type = NULL; // DaoJitCallData
PointerType *daojit_call_data_type_p = NULL; // DaoJitCallData*

int daojit_opcode_compilable[ DVM_NULL ];

Function *daojit_debug_function = NULL;

Function *daojit_pow_double = NULL;
Function *daojit_abs_double = NULL;
Function *daojit_acos_double = NULL;
Function *daojit_asin_double = NULL;
Function *daojit_atan_double = NULL;
Function *daojit_ceil_double = NULL;
Function *daojit_cos_double = NULL;
Function *daojit_cosh_double = NULL;
Function *daojit_exp_double = NULL;
Function *daojit_floor_double = NULL;
Function *daojit_log_double = NULL;
Function *daojit_rand_double = NULL;
Function *daojit_sin_double = NULL;
Function *daojit_sinh_double = NULL;
Function *daojit_sqrt_double = NULL;
Function *daojit_tan_double = NULL;
Function *daojit_tanh_double = NULL;

Function *daojit_string_move = NULL;
Function *daojit_string_add = NULL;
Function *daojit_string_lt = NULL;
Function *daojit_string_le = NULL;
Function *daojit_string_eq = NULL;
Function *daojit_string_ne = NULL;
Function *daojit_geti_si = NULL;
Function *daojit_seti_sii = NULL;
Function *daojit_seti_li = NULL;
Function *daojit_seti_ti = NULL;
Function *daojit_setf_tpp = NULL;

Function *daojit_array_set_items_i = NULL;
Function *daojit_array_set_items_f = NULL;
Function *daojit_array_set_items_d = NULL;
Function *daojit_array_set_items_c = NULL;
Function *daojit_array_set_items_a = NULL;

Function *daojit_array_add_integer_array = NULL;
Function *daojit_array_sub_integer_array = NULL;
Function *daojit_array_mul_integer_array = NULL;
Function *daojit_array_div_integer_array = NULL;
Function *daojit_array_mod_integer_array = NULL;
Function *daojit_array_pow_integer_array = NULL;

Function *daojit_array_add_float_array = NULL;
Function *daojit_array_sub_float_array = NULL;
Function *daojit_array_mul_float_array = NULL;
Function *daojit_array_div_float_array = NULL;
Function *daojit_array_mod_float_array = NULL;
Function *daojit_array_pow_float_array = NULL;

Function *daojit_array_add_double_array = NULL;
Function *daojit_array_sub_double_array = NULL;
Function *daojit_array_mul_double_array = NULL;
Function *daojit_array_div_double_array = NULL;
Function *daojit_array_mod_double_array = NULL;
Function *daojit_array_pow_double_array = NULL;

Function *daojit_array_add_complex_array = NULL;
Function *daojit_array_sub_complex_array = NULL;
Function *daojit_array_mul_complex_array = NULL;
Function *daojit_array_div_complex_array = NULL;
Function *daojit_array_mod_complex_array = NULL;
Function *daojit_array_pow_complex_array = NULL;

Function *daojit_array_add_array_integer = NULL;
Function *daojit_array_sub_array_integer = NULL;
Function *daojit_array_mul_array_integer = NULL;
Function *daojit_array_div_array_integer = NULL;
Function *daojit_array_mod_array_integer = NULL;
Function *daojit_array_pow_array_integer = NULL;

Function *daojit_array_add_array_float = NULL;
Function *daojit_array_sub_array_float = NULL;
Function *daojit_array_mul_array_float = NULL;
Function *daojit_array_div_array_float = NULL;
Function *daojit_array_mod_array_float = NULL;
Function *daojit_array_pow_array_float = NULL;

Function *daojit_array_add_array_double = NULL;
Function *daojit_array_sub_array_double = NULL;
Function *daojit_array_mul_array_double = NULL;
Function *daojit_array_div_array_double = NULL;
Function *daojit_array_mod_array_double = NULL;
Function *daojit_array_pow_array_double = NULL;

Function *daojit_array_add_array_complex = NULL;
Function *daojit_array_sub_array_complex = NULL;
Function *daojit_array_mul_array_complex = NULL;
Function *daojit_array_div_array_complex = NULL;
Function *daojit_array_mod_array_complex = NULL;
Function *daojit_array_pow_array_complex = NULL;

Function *daojit_array_add_array_array = NULL;
Function *daojit_array_sub_array_array = NULL;
Function *daojit_array_mul_array_array = NULL;
Function *daojit_array_div_array_array = NULL;
Function *daojit_array_mod_array_array = NULL;
Function *daojit_array_pow_array_array = NULL;

Function *daojit_load = NULL;
Function *daojit_move_pp = NULL;
Function *daojit_value_copy = NULL;
Function *daojit_value_move = NULL;

Function *daojit_array_sliced = NULL;

FunctionType *daojit_function_type = NULL;

extern "C"{

void daojit_debug( void *p )
{
	printf( "debug: %p\n", p );
	fflush( stdout );
}

double daojit_rand( double max ){ return max * rand() / (RAND_MAX+1.0); }


void DaoJIT_LOAD( DaoValue *dA, DaoValue **dC )
{
	if( dA == NULL ) return;
	if( (dA->xNone.trait & DAO_VALUE_CONST) == 0 ){
		GC_Assign( dC, dA );
	}else{
		DaoValue_Copy( dA, dC );
	}
}
int DaoArray_number_op_array( DaoArray *C, DaoValue *A, DaoArray *B, short op, DaoProcess *proc );
int DaoArray_array_op_number( DaoArray *C, DaoArray *A, DaoValue *B, short op, DaoProcess *proc );
int DaoArray_ArrayArith( DaoArray *C, DaoArray *A, DaoArray *B, short op, DaoProcess *proc );
void DaoJIT_SETI_ARRAY_I( DaoValue *C, dao_integer A, int *estatus )
{
	int rc;
	DaoInteger V = {DAO_INTEGER,0,0,0,1,0};
	V.value = A;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) C, (DaoValue*) & V, DVM_MOVE, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_SETI_ARRAY_F( DaoValue *C, dao_float A, int *estatus )
{
	int rc;
	DaoFloat V = {DAO_FLOAT,0,0,0,1,0.0};
	V.value = A;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) C, (DaoValue*) & V, DVM_MOVE, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_SETI_ARRAY_C( DaoValue *C, dao_complex A, int *estatus )
{
	int rc;
	DaoComplex V = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	V.value = A;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) C, (DaoValue*) & V, DVM_MOVE, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_SETI_ARRAY_A( DaoValue *C, DaoValue *A, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) C, (DaoArray*) A, DVM_MOVE, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}


void DaoJIT_BINOP_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus, int op )
{
	int rc;
	DaoInteger V = {DAO_INTEGER,0,0,0,1,0};
	V.value = A;
	rc = DaoArray_number_op_array( (DaoArray*) C, (DaoValue*) & V, (DaoArray*) B, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_BINOP_FLOAT_ARRAY( DaoValue *C, double A, DaoValue *B, int *estatus, int op )
{
	int rc;
	DaoFloat V = {DAO_FLOAT,0,0,0,1,0.0};
	V.value = A;
	rc = DaoArray_number_op_array( (DaoArray*) C, (DaoValue*) & V, (DaoArray*) B, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_BINOP_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus, int op )
{
	int rc;
	DaoComplex V = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	V.value = A;
	rc = DaoArray_number_op_array( (DaoArray*) C, (DaoValue*) & V, (DaoArray*) B, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}

void DaoJIT_ADD_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_ADD );
}
void DaoJIT_ADD_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_ADD );
}
void DaoJIT_ADD_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_ADD );
}

void DaoJIT_SUB_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_SUB );
}
void DaoJIT_SUB_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_SUB );
}
void DaoJIT_SUB_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_SUB );
}

void DaoJIT_MUL_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_MUL );
}
void DaoJIT_MUL_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_MUL );
}
void DaoJIT_MUL_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_MUL );
}

void DaoJIT_DIV_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_DIV );
}
void DaoJIT_DIV_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_DIV );
}
void DaoJIT_DIV_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_DIV );
}

void DaoJIT_MOD_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_MOD );
}
void DaoJIT_MOD_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_MOD );
}
void DaoJIT_MOD_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_MOD );
}

void DaoJIT_POW_INTEGER_ARRAY( DaoValue *C, dao_integer A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_INTEGER_ARRAY( C, A, B, estatus, DVM_POW );
}
void DaoJIT_POW_FLOAT_ARRAY( DaoValue *C, float A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_FLOAT_ARRAY( C, A, B, estatus, DVM_POW );
}
void DaoJIT_POW_COMPLEX_ARRAY( DaoValue *C, dao_complex A, DaoValue *B, int *estatus )
{
	DaoJIT_BINOP_COMPLEX_ARRAY( C, A, B, estatus, DVM_POW );
}


void DaoJIT_BINOP_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus, int op )
{
	int rc;
	DaoInteger V = {DAO_INTEGER,0,0,0,1,0};
	V.value = B;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) A, (DaoValue*) & V, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_BINOP_ARRAY_FLOAT( DaoValue *C, DaoValue *A, double B, int *estatus, int op )
{
	int rc;
	DaoFloat V = {DAO_FLOAT,0,0,0,1,0.0};
	V.value = B;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) A, (DaoValue*) & V, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_BINOP_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus, int op )
{
	int rc;
	DaoComplex V = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	V.value = B;
	rc = DaoArray_array_op_number( (DaoArray*) C, (DaoArray*) A, (DaoValue*) & V, op, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}

void DaoJIT_ADD_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_ADD );
}
void DaoJIT_ADD_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_ADD );
}
void DaoJIT_ADD_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_ADD );
}

void DaoJIT_SUB_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_SUB );
}
void DaoJIT_SUB_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_SUB );
}
void DaoJIT_SUB_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_SUB );
}

void DaoJIT_MUL_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_MUL );
}
void DaoJIT_MUL_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_MUL );
}
void DaoJIT_MUL_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_MUL );
}

void DaoJIT_DIV_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_DIV );
}
void DaoJIT_DIV_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_DIV );
}
void DaoJIT_DIV_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_DIV );
}

void DaoJIT_MOD_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_MOD );
}
void DaoJIT_MOD_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_MOD );
}
void DaoJIT_MOD_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_MOD );
}

void DaoJIT_POW_ARRAY_INTEGER( DaoValue *C, DaoValue *A, dao_integer B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_INTEGER( C, A, B, estatus, DVM_POW );
}
void DaoJIT_POW_ARRAY_FLOAT( DaoValue *C, DaoValue *A, float B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_FLOAT( C, A, B, estatus, DVM_POW );
}
void DaoJIT_POW_ARRAY_COMPLEX( DaoValue *C, DaoValue *A, dao_complex B, int *estatus )
{
	DaoJIT_BINOP_ARRAY_COMPLEX( C, A, B, estatus, DVM_POW );
}


void DaoJIT_ADD_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_ADD, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_SUB_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_SUB, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_MUL_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_MUL, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_DIV_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_DIV, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_MOD_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_MOD, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_POW_ARRAY_ARRAY( DaoValue *C, DaoValue *A, DaoValue *B, int *estatus )
{
	int rc = DaoArray_ArrayArith( (DaoArray*) C, (DaoArray*) A, (DaoArray*) B, DVM_POW, NULL );
	if( rc == 0 ) *estatus = DAO_ERROR<<16;
}
void DaoJIT_MOVE_PP( DaoValue *dA, DaoValue **dC )
{
	GC_Assign( dC, dA );
}
void DaoJIT_MOVE_SS( DaoValue *dA, DaoValue **dC2 )
{
	DaoValue *dC = *dC2;
	if( dC && dC->type == DAO_STRING ){
		DString_Assign( dC->xString.value, dA->xString.value );
	}else{
		DaoString *S = DaoString_Copy( (DaoString*) dA );
		GC_Assign( dC2, S );
	}
}
void DaoJIT_ADD_SS( DaoValue *dA, DaoValue *dB, DaoValue **dC2 )
{
	DaoValue *dC = *dC2;
	if( dA == dC ){
		DString_Append( dA->xString.value, dB->xString.value );
	}else if( dB == dC ){
		DString_Insert( dB->xString.value, dA->xString.value, 0, 0, 0 );
	}else{
		if( dC && dC->type == DAO_STRING ){
			DString_Assign( dC->xString.value, dA->xString.value );
		}else{
			DaoString *S = DaoString_Copy( (DaoString*) dA );
			GC_Assign( dC2, S );
		}
		DString_Append( dC->xString.value, dB->xString.value );
	}
}
void DaoJIT_ADD_LT( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.value, dB->xString.value ) <0);
}
void DaoJIT_ADD_LE( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.value, dB->xString.value ) <=0);
}
void DaoJIT_ADD_EQ( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.value, dB->xString.value ) ==0);
}
void DaoJIT_ADD_NE( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.value, dB->xString.value ) !=0);
}
// instruction index is passed in as estatus:
dao_integer DaoJIT_GETI_SI( DaoValue *dA, dao_integer id, int *estatus )
{
	DString *string = dA->xString.value;
	char *mbs = string->chars;
	if( id <0 ) id += string->size;
	if( id <0 || id >= string->size ){
		*estatus |= (DAO_ERROR_INDEX<<16);
		return 0;
	}
	return mbs[id];
}
void DaoJIT_SETI_SII( dao_integer ch, dao_integer id, DaoValue *dC, int *estatus )
{
	DString *string = dC->xString.value;
	char *mbs = string->chars;
	if( id <0 ) id += string->size;
	if( id <0 || id >= string->size ){
		*estatus |= (DAO_ERROR_INDEX<<16);
		return;
	}
	mbs[id] = ch;
}
void DaoJIT_SETI_LI( DaoValue *dA, dao_integer id, DaoValue *dC, int *estatus )
{
	DaoList *list = (DaoList*) dC;
	if( id <0 ) id += list->value->size;
	if( id <0 || id >= list->value->size ){
		*estatus = DAO_ERROR_INDEX;
		return;
	}
	DaoValue_Copy( dA, list->value->items.pValue + id );
}
void DaoJIT_SETI_TI( DaoValue *dA, dao_integer id, DaoValue *dC, int *estatus )
{
	DaoTuple *tuple = (DaoTuple*) dC;
	DaoType *type = NULL;
	if( id <0 || id >= tuple->size ){
		*estatus = DAO_ERROR_INDEX;
		return;
	}
	type = tuple->ctype->nested->items.pType[id];
	if( type->tid == DAO_PAR_NAMED ) type = & type->aux->xType;
	if( DaoValue_Move( dA, tuple->values + id, type ) ==0 ) *estatus = DAO_ERROR_VALUE;
}
void DaoJIT_SETF_TPP( DaoValue *dA, int id, DaoValue *dC, int *estatus )
{
	DaoTuple *tuple = (DaoTuple*) dC;
	GC_Assign( & tuple->values[id], dA );
}
}

void DaoJIT_Init( DaoVmSpace *vms, DaoJIT *jit )
{
	int i;
	llvm::GlobalValue::LinkageTypes linkage = Function::ExternalLinkage;
	memset( daojit_opcode_compilable, 0, DVM_NULL*sizeof(int) );
	for(i=DVM_MOVE_BB; i<=DVM_MOVE_CC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_AND_BBB; i<=DVM_NE_BFF; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_ADD_CCC; i<=DVM_NE_BCC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_ADD_SSS; i<=DVM_NE_BSS; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_GETF_KCI; i<=DVM_SETF_OVCC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_GETI_TI; i<=DVM_SETF_TSS; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_GETI_LBI; i<=DVM_GETI_LCI; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_SETI_LBIB; i<=DVM_SETI_LCIC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_GETI_ABI; i<=DVM_SETI_ACIC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_GETMI_ABI; i<=DVM_SETMI_ACIC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_DATA_B; i<=DVM_SETVG_CC; i++) daojit_opcode_compilable[i] = 1;
	for(i=DVM_NOT_B; i<=DVM_TILDE_I; i++) daojit_opcode_compilable[i] = 1;
	daojit_opcode_compilable[ DVM_LOAD ] = 1;
	daojit_opcode_compilable[ DVM_GOTO ] = 1;
	daojit_opcode_compilable[ DVM_SWITCH ] = 1;
	daojit_opcode_compilable[ DVM_CASE ] = 1;
	daojit_opcode_compilable[ DVM_TEST ] = 1;
	daojit_opcode_compilable[ DVM_TEST_B ] = 1;
	daojit_opcode_compilable[ DVM_TEST_I ] = 1;
	daojit_opcode_compilable[ DVM_TEST_F ] = 1;
	daojit_opcode_compilable[ DVM_GETI_SI ] = 1;
	daojit_opcode_compilable[ DVM_SETI_SII ] = 1;
	daojit_opcode_compilable[ DVM_GETI_LI ] = 1;
	daojit_opcode_compilable[ DVM_SETI_LI ] = 1;
	daojit_opcode_compilable[ DVM_SETI_TI ] = 1;
	daojit_opcode_compilable[ DVM_SETF_TPP ] = 1;
	daojit_opcode_compilable[ DVM_MOVE_PP ] = 1;
	daojit_opcode_compilable[ DVM_MOVE_SS ] = 1;

	// TODO: GETCX, GETVX, swith, complex, string, array, tuple, list etc.

	InitializeNativeTarget();
	llvm_context = new LLVMContext();
	llvm_module = new Module("DaoJIT", *llvm_context);

	int size_t_bits = CHAR_BIT * sizeof(size_t) - 1;

	Type *string_size_type = Type::getIntNTy( *llvm_context, size_t_bits - 1 );

	int1_type = Type::getInt1Ty( *llvm_context );
	int8_type = Type::getInt8Ty( *llvm_context );
	int16_type = Type::getInt16Ty( *llvm_context );
	int32_type = Type::getInt32Ty( *llvm_context );
	int8_vector2 = VectorType::get( int8_type, 2 );

	daoint_type = int32_type;
	if( sizeof(void*) == 8 ) daoint_type = Type::getInt64Ty( *llvm_context );

	dao_boolean_type = int8_type;
	dao_integer_type = Type::getInt64Ty( *llvm_context );
	dao_float_type = Type::getDoubleTy( *llvm_context );
	double_type = dao_float_type;

	std::vector<Type*> vc( 2, dao_float_type );
	dao_complex_type = StructType::get( *llvm_context, vc );

	cxx_number_types[DAO_BOOLEAN - DAO_BOOLEAN] = dao_integer_type;
	cxx_number_types[DAO_INTEGER - DAO_BOOLEAN] = dao_integer_type;
	cxx_number_types[DAO_FLOAT   - DAO_BOOLEAN] = dao_float_type;
	cxx_number_types[DAO_COMPLEX - DAO_BOOLEAN] = dao_complex_type;

	daoint_array_type = ArrayType::get( daoint_type, 0 );
	dao_boolean_array_type = ArrayType::get( dao_boolean_type, 0 );
	dao_integer_array_type = ArrayType::get( dao_integer_type, 0 );
	dao_float_array_type = ArrayType::get( dao_float_type, 0 );
	dao_complex_array_type = ArrayType::get( dao_complex_type, 0 );

	daoint_array_type_p = PointerType::getUnqual( daoint_array_type );
	dao_boolean_array_type_p = PointerType::getUnqual( dao_boolean_array_type );
	dao_integer_array_type_p = PointerType::getUnqual( dao_integer_array_type );
	dao_float_array_type_p = PointerType::getUnqual( dao_float_array_type );
	dao_complex_array_type_p = PointerType::getUnqual( dao_complex_array_type );

	size_t_type = daoint_type;

	void_type = Type::getVoidTy( *llvm_context );
	void_type_p = PointerType::getUnqual( int8_type );

	int32_type_p = PointerType::getUnqual( int32_type );

	std::vector<Type*> field_types( 6, string_size_type );
	field_types[0] = PointerType::getUnqual( int8_type );
	field_types[2] = int1_type;
	field_types[4] = int1_type;

	dstring_type = StructType::get( *llvm_context, field_types );
	dstring_type_p = PointerType::getUnqual( dstring_type );
	dstring_type_pp = PointerType::getUnqual( dstring_type_p );

	field_types.clear();
	field_types.resize( 4, int8_type );
	field_types.push_back( int32_type ); // refCount

	// type { i8, i8, i8, i8, i32 }
	daojit_value_type = StructType::get( *llvm_context, field_types );
	daojit_value_type_p = PointerType::getUnqual( daojit_value_type );
	daojit_value_type_pp = PointerType::getUnqual( daojit_value_type_p );
	daojit_value_ptr_array_type = ArrayType::get( daojit_value_type_p, 0 );
	daojit_value_ptr_array_type_p = PointerType::getUnqual( daojit_value_ptr_array_type );

	field_types.push_back( int32_type );
	field_types.push_back( daojit_value_type_p );

	daojit_constant_type = StructType::get( *llvm_context, field_types );
	daojit_constant_type_p = PointerType::getUnqual( daojit_constant_type );
	daojit_constant_ptr_array_type = ArrayType::get( daojit_constant_type_p, 0 );
	daojit_constant_ptr_array_type_p = PointerType::getUnqual( daojit_constant_ptr_array_type );

	daojit_variable_type = daojit_constant_type;
	daojit_variable_type_p = daojit_constant_type_p;
	daojit_variable_ptr_array_type = daojit_constant_ptr_array_type;
	daojit_variable_ptr_array_type_p = daojit_constant_ptr_array_type_p;


	// type { i8, i8, i8, i8, i32, dao_integer }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( dao_integer_type );
	daojit_integer_type = StructType::get( *llvm_context, field_types );
	daojit_integer_type_p = PointerType::getUnqual( daojit_integer_type );
	//daojit_integer_type_pp = PointerType::getUnqual( daojit_integer_type_p );

	// type { i8, i8, i8, i8, i32, dao_float }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( dao_float_type );
	daojit_float_type = StructType::get( *llvm_context, field_types );
	daojit_float_type_p = PointerType::getUnqual( daojit_float_type );
	//daojit_float_type_pp = PointerType::getUnqual( daojit_float_type_p );

	field_types[5] = dao_complex_type;
	daojit_complex_type = StructType::get( *llvm_context, field_types );
	daojit_complex_type_p = PointerType::getUnqual( daojit_complex_type );

	daojit_number_types[DAO_BOOLEAN - DAO_BOOLEAN] = daojit_integer_type_p;
	daojit_number_types[DAO_INTEGER - DAO_BOOLEAN] = daojit_integer_type_p;
	daojit_number_types[DAO_FLOAT   - DAO_BOOLEAN] = daojit_float_type_p;
	daojit_number_types[DAO_COMPLEX - DAO_BOOLEAN] = daojit_complex_type_p;

	// type { i8, i8, i8, i8, i32, DString* }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( dstring_type_p );
	daojit_string_type = StructType::get( *llvm_context, field_types );
	daojit_string_type_p = PointerType::getUnqual( daojit_string_type );
	daojit_string_type_pp = PointerType::getUnqual( daojit_string_type_p );

	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( int32_type ); // cycRefCount

	// type { i8, i8, i8, i8, i32, i32, i32 }
	field_types.push_back( int32_type ); // value
	daojit_enum_type = StructType::get( *llvm_context, field_types );
	daojit_enum_type_p = PointerType::getUnqual( daojit_enum_type );
	daojit_enum_type_pp = PointerType::getUnqual( daojit_enum_type_p );

	// type { i8, i8, i8, i8, i32, i32, i8 }
	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int8_type ); // value
	daojit_type_type = StructType::get( *llvm_context, field_types );
	daojit_type_type_p = PointerType::getUnqual( daojit_type_type );
	daojit_type_ptr_array_type = ArrayType::get( daojit_type_type_p, 0 );
	daojit_type_ptr_array_type_p = PointerType::getUnqual( daojit_type_ptr_array_type );


	std::vector<Type*> array_types( 1, daojit_value_ptr_array_type_p ); // items
	array_types.push_back( size_t_type ); // size
	darray_value_type = StructType::get( *llvm_context, array_types );
	darray_value_type_p = PointerType::getUnqual( darray_value_type );

	array_types[0] = daojit_constant_ptr_array_type_p;
	darray_constant_type = StructType::get( *llvm_context, array_types );
	darray_constant_type_p = PointerType::getUnqual( darray_constant_type );

	darray_variable_type = darray_constant_type;
	darray_variable_type_p = darray_constant_type_p;


	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( daojit_type_type_p ); // ctype
	field_types.push_back( darray_value_type_p ); // value
	daojit_list_type = StructType::get( *llvm_context, field_types );
	daojit_list_type_p = PointerType::getUnqual( daojit_list_type );
	daojit_list_type_pp = PointerType::getUnqual( daojit_list_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int32_type ); // size
	field_types.push_back( daojit_type_type_p ); // ctype
	field_types.push_back( daojit_value_ptr_array_type ); // values
	daojit_tuple_type = StructType::get( *llvm_context, field_types );
	daojit_tuple_type_p = PointerType::getUnqual( daojit_tuple_type );
	daojit_tuple_type_pp = PointerType::getUnqual( daojit_tuple_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int8_type ); // etype
	field_types.push_back( int8_type ); // owner 
	field_types.push_back( int16_type ); // ndim
	field_types.push_back( daoint_type ); // size
	field_types.push_back( daoint_array_type_p ); // dims
	field_types.push_back( dao_boolean_array_type_p ); // data.i
	field_types.push_back( void_type_p ); // original
	field_types.push_back( void_type_p ); // slices

	daojit_array_b_type = StructType::get( *llvm_context, field_types );
	daojit_array_b_type_p = PointerType::getUnqual( daojit_array_b_type );

	field_types[11] = dao_integer_array_type_p;
	daojit_array_i_type = StructType::get( *llvm_context, field_types );
	daojit_array_i_type_p = PointerType::getUnqual( daojit_array_i_type );

	field_types[11] = dao_float_array_type_p;
	daojit_array_f_type = StructType::get( *llvm_context, field_types );
	daojit_array_f_type_p = PointerType::getUnqual( daojit_array_f_type );

	field_types[11] = dao_complex_array_type_p;
	daojit_array_c_type = StructType::get( *llvm_context, field_types );
	daojit_array_c_type_p = PointerType::getUnqual( daojit_array_c_type );

	daojit_array_types[DAO_BOOLEAN - DAO_BOOLEAN] = daojit_array_b_type_p;
	daojit_array_types[DAO_INTEGER - DAO_BOOLEAN] = daojit_array_i_type_p;
	daojit_array_types[DAO_FLOAT   - DAO_BOOLEAN] = daojit_array_f_type_p;
	daojit_array_types[DAO_COMPLEX - DAO_BOOLEAN] = daojit_array_c_type_p;

	field_types.erase( field_types.begin()+6, field_types.end() );
	for(i=0; i<2; i++) field_types.push_back( void_type_p );
	field_types.push_back( darray_constant_type_p );
	field_types.push_back( darray_variable_type_p );
	daojit_class_type = StructType::get( *llvm_context, field_types );
	daojit_class_type_p = PointerType::getUnqual( daojit_class_type );
	daojit_class_type_pp = PointerType::getUnqual( daojit_class_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	for(i=0; i<8; i++) field_types.push_back( void_type_p );
	field_types.push_back( daojit_value_ptr_array_type_p );
	daojit_process_type = StructType::get( *llvm_context, field_types );
	daojit_process_type_p = PointerType::getUnqual( daojit_process_type );
	daojit_process_type_pp = PointerType::getUnqual( daojit_process_type_p );

	daojit_process_ptr_array_type = ArrayType::get( daojit_process_type_p, 0 );
	daojit_process_ptr_array_type_p = PointerType::getUnqual( daojit_process_ptr_array_type );

	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int16_type ); // bit fields;
	field_types.push_back( int16_type ); // valueCount;
	field_types.push_back( daojit_class_type_p ); // defClass;
	field_types.push_back( void_type_p ); // rootObject;
	field_types.push_back( void_type_p ); // parent;
	field_types.push_back( daojit_value_ptr_array_type_p ); // objValues;
	daojit_object_type = StructType::get( *llvm_context, field_types );
	daojit_object_type_p = PointerType::getUnqual( daojit_object_type );
	daojit_object_type_pp = PointerType::getUnqual( daojit_object_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	for(i=0; i<2; i++) field_types.push_back( void_type_p );
	for(i=0; i<3; i++) field_types.push_back( darray_value_type_p );
	daojit_namespace_type = StructType::get( *llvm_context, field_types );
	daojit_namespace_type_p = PointerType::getUnqual( daojit_namespace_type );
	daojit_namespace_type_pp = PointerType::getUnqual( daojit_namespace_type_p );


	array_types[0] = daojit_class_ptr_array_type_p;
	darray_class_type = StructType::get( *llvm_context, array_types );
	darray_class_type_p = PointerType::getUnqual( darray_class_type );

	array_types[0] = daojit_process_ptr_array_type_p;
	darray_process_type = StructType::get( *llvm_context, array_types );
	darray_process_type_p = PointerType::getUnqual( darray_process_type );

	array_types[0] = daojit_namespace_ptr_array_type_p;
	darray_namespace_type = StructType::get( *llvm_context, array_types );
	darray_namespace_type_p = PointerType::getUnqual( darray_namespace_type );

	
	std::vector<Type*> jitcd_types( 3, daojit_value_ptr_array_type_p );
	jitcd_types.push_back( daojit_variable_ptr_array_type_p );
	jitcd_types.push_back( daojit_constant_ptr_array_type_p );
	jitcd_types.push_back( daojit_variable_ptr_array_type_p );
	jitcd_types.push_back( daojit_constant_ptr_array_type_p );
	jitcd_types.push_back( daojit_process_ptr_array_type_p );
	daojit_call_data_type = StructType::get( *llvm_context, jitcd_types );
	daojit_call_data_type_p = PointerType::getUnqual( daojit_call_data_type );


	std::vector<Type*> jitParams( 1, daojit_call_data_type_p );
	daojit_function_type = FunctionType::get( int32_type, jitParams, false );

	std::vector<Type*> value_params( 1, daojit_value_type_p );
	value_params.push_back( daojit_value_type_p );
	FunctionType *ft0 = FunctionType::get( void_type, value_params, false );
	daojit_value_copy = Function::Create( ft0, linkage, "DaoValue_Copy", llvm_module );

	value_params.clear();
	value_params.push_back( daojit_value_type );
	value_params.push_back( daojit_value_type_p );
	value_params.push_back( daojit_type_type_p );
	ft0 = FunctionType::get( int32_type, value_params, false );
	daojit_value_move = Function::Create( ft0, linkage, "DaoValue_Move", llvm_module );

	std::vector<Type*> value2( 1, daojit_value_type_p );
	value2.push_back( daojit_value_type_pp );
	FunctionType *ft2 = FunctionType::get( void_type, value2, false );
	daojit_string_move = Function::Create( ft2, linkage, "DaoJIT_MOVE_SS", llvm_module );
	daojit_load = Function::Create( ft2, linkage, "DaoJIT_LOAD", llvm_module );
	daojit_move_pp = Function::Create( ft2, linkage, "DaoJIT_MOVE_PP", llvm_module );

	value2.push_back( int32_type_p );
	value2[0] = daojit_value_type_p;
	value2[1] = dao_integer_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_set_items_i = Function::Create( ft2, linkage, "DaoJIT_SETI_ARRAY_I", llvm_module );
	value2[1] = dao_float_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_set_items_f = Function::Create( ft2, linkage, "DaoJIT_SETI_ARRAY_F", llvm_module );
	value2[1] = dao_complex_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_set_items_c = Function::Create( ft2, linkage, "DaoJIT_SETI_ARRAY_C", llvm_module );
	value2[1] = daojit_value_type_p;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_set_items_a = Function::Create( ft2, linkage, "DaoJIT_SETI_ARRAY_A", llvm_module );

	value2.push_back( int32_type_p );
	value2[0] = daojit_value_type_p;
	value2[1] = dao_integer_type;
	value2[2] = daojit_value_type_p;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_integer_array = Function::Create( ft2, linkage, "DaoJIT_ADD_INTEGER_ARRAY", llvm_module );
	daojit_array_sub_integer_array = Function::Create( ft2, linkage, "DaoJIT_SUB_INTEGER_ARRAY", llvm_module );
	daojit_array_mul_integer_array = Function::Create( ft2, linkage, "DaoJIT_MUL_INTEGER_ARRAY", llvm_module );
	daojit_array_div_integer_array = Function::Create( ft2, linkage, "DaoJIT_DIV_INTEGER_ARRAY", llvm_module );
	daojit_array_mod_integer_array = Function::Create( ft2, linkage, "DaoJIT_MOD_INTEGER_ARRAY", llvm_module );
	daojit_array_pow_integer_array = Function::Create( ft2, linkage, "DaoJIT_POW_INTEGER_ARRAY", llvm_module );

	value2[1] = dao_float_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_float_array = Function::Create( ft2, linkage, "DaoJIT_ADD_FLOAT_ARRAY", llvm_module );
	daojit_array_sub_float_array = Function::Create( ft2, linkage, "DaoJIT_SUB_FLOAT_ARRAY", llvm_module );
	daojit_array_mul_float_array = Function::Create( ft2, linkage, "DaoJIT_MUL_FLOAT_ARRAY", llvm_module );
	daojit_array_div_float_array = Function::Create( ft2, linkage, "DaoJIT_DIV_FLOAT_ARRAY", llvm_module );
	daojit_array_mod_float_array = Function::Create( ft2, linkage, "DaoJIT_MOD_FLOAT_ARRAY", llvm_module );
	daojit_array_pow_float_array = Function::Create( ft2, linkage, "DaoJIT_POW_FLOAT_ARRAY", llvm_module );

	value2[1] = dao_complex_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_complex_array = Function::Create( ft2, linkage, "DaoJIT_ADD_COMPLEX_ARRAY", llvm_module );
	daojit_array_sub_complex_array = Function::Create( ft2, linkage, "DaoJIT_SUB_COMPLEX_ARRAY", llvm_module );
	daojit_array_mul_complex_array = Function::Create( ft2, linkage, "DaoJIT_MUL_COMPLEX_ARRAY", llvm_module );
	daojit_array_div_complex_array = Function::Create( ft2, linkage, "DaoJIT_DIV_COMPLEX_ARRAY", llvm_module );
	daojit_array_mod_complex_array = Function::Create( ft2, linkage, "DaoJIT_MOD_COMPLEX_ARRAY", llvm_module );
	daojit_array_pow_complex_array = Function::Create( ft2, linkage, "DaoJIT_POW_COMPLEX_ARRAY", llvm_module );

	value2[0] = daojit_value_type_p;
	value2[1] = daojit_value_type_p;
	value2[2] = dao_integer_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_array_integer = Function::Create( ft2, linkage, "DaoJIT_ADD_ARRAY_INTEGER", llvm_module );
	daojit_array_sub_array_integer = Function::Create( ft2, linkage, "DaoJIT_SUB_ARRAY_INTEGER", llvm_module );
	daojit_array_mul_array_integer = Function::Create( ft2, linkage, "DaoJIT_MUL_ARRAY_INTEGER", llvm_module );
	daojit_array_div_array_integer = Function::Create( ft2, linkage, "DaoJIT_DIV_ARRAY_INTEGER", llvm_module );
	daojit_array_mod_array_integer = Function::Create( ft2, linkage, "DaoJIT_MOD_ARRAY_INTEGER", llvm_module );
	daojit_array_pow_array_integer = Function::Create( ft2, linkage, "DaoJIT_POW_ARRAY_INTEGER", llvm_module );

	value2[2] = dao_float_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_array_float = Function::Create( ft2, linkage, "DaoJIT_ADD_ARRAY_FLOAT", llvm_module );
	daojit_array_sub_array_float = Function::Create( ft2, linkage, "DaoJIT_SUB_ARRAY_FLOAT", llvm_module );
	daojit_array_mul_array_float = Function::Create( ft2, linkage, "DaoJIT_MUL_ARRAY_FLOAT", llvm_module );
	daojit_array_div_array_float = Function::Create( ft2, linkage, "DaoJIT_DIV_ARRAY_FLOAT", llvm_module );
	daojit_array_mod_array_float = Function::Create( ft2, linkage, "DaoJIT_MOD_ARRAY_FLOAT", llvm_module );
	daojit_array_pow_array_float = Function::Create( ft2, linkage, "DaoJIT_POW_ARRAY_FLOAT", llvm_module );

	value2[2] = dao_complex_type;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_array_complex = Function::Create( ft2, linkage, "DaoJIT_ADD_ARRAY_COMPLEX", llvm_module );
	daojit_array_sub_array_complex = Function::Create( ft2, linkage, "DaoJIT_SUB_ARRAY_COMPLEX", llvm_module );
	daojit_array_mul_array_complex = Function::Create( ft2, linkage, "DaoJIT_MUL_ARRAY_COMPLEX", llvm_module );
	daojit_array_div_array_complex = Function::Create( ft2, linkage, "DaoJIT_DIV_ARRAY_COMPLEX", llvm_module );
	daojit_array_mod_array_complex = Function::Create( ft2, linkage, "DaoJIT_MOD_ARRAY_COMPLEX", llvm_module );
	daojit_array_pow_array_complex = Function::Create( ft2, linkage, "DaoJIT_POW_ARRAY_COMPLEX", llvm_module );

	value2[0] = daojit_value_type_p;
	value2[1] = daojit_value_type_p;
	value2[2] = daojit_value_type_p;
	ft2 = FunctionType::get( void_type, value2, false );
	daojit_array_add_array_array = Function::Create( ft2, linkage, "DaoJIT_ADD_ARRAY_ARRAY", llvm_module );
	daojit_array_sub_array_array = Function::Create( ft2, linkage, "DaoJIT_SUB_ARRAY_ARRAY", llvm_module );
	daojit_array_mul_array_array = Function::Create( ft2, linkage, "DaoJIT_MUL_ARRAY_ARRAY", llvm_module );
	daojit_array_div_array_array = Function::Create( ft2, linkage, "DaoJIT_DIV_ARRAY_ARRAY", llvm_module );
	daojit_array_mod_array_array = Function::Create( ft2, linkage, "DaoJIT_MOD_ARRAY_ARRAY", llvm_module );
	daojit_array_pow_array_array = Function::Create( ft2, linkage, "DaoJIT_POW_ARRAY_ARRAY", llvm_module );

	std::vector<Type*> value3( 2, daojit_value_type_p );
	value3.push_back( daojit_value_type_pp );
	FunctionType *ft3 = FunctionType::get( void_type, value3, false );
	daojit_string_add = Function::Create( ft3, linkage, "DaoJIT_ADD_SS", llvm_module );

	value3[2] = daojit_value_type_p;
	ft3 = FunctionType::get( void_type, value3, false );
	daojit_string_lt = Function::Create( ft3, linkage, "DaoJIT_ADD_LT", llvm_module );
	daojit_string_le = Function::Create( ft3, linkage, "DaoJIT_ADD_LE", llvm_module );
	daojit_string_eq = Function::Create( ft3, linkage, "DaoJIT_ADD_EQ", llvm_module );
	daojit_string_ne = Function::Create( ft3, linkage, "DaoJIT_ADD_NE", llvm_module );

	value3[1] = dao_integer_type;
	value3[2] = int32_type_p;
	FunctionType *ft4 = FunctionType::get( dao_integer_type, value3, false );
	daojit_geti_si = Function::Create( ft4, linkage, "DaoJIT_GETI_SI", llvm_module );

	value3[0] = dao_integer_type;
	value3[2] = daojit_value_type_p;
	value3.push_back( int32_type_p );
	ft4 = FunctionType::get( void_type, value3, false );
	daojit_seti_sii = Function::Create( ft4, linkage, "DaoJIT_SETI_SII", llvm_module );

	value3[0] = daojit_value_type_p;
	ft4 = FunctionType::get( void_type, value3, false );
	daojit_seti_li = Function::Create( ft4, linkage, "DaoJIT_SETI_LI", llvm_module );
	daojit_seti_ti = Function::Create( ft4, linkage, "DaoJIT_SETI_TI", llvm_module );
	daojit_setf_tpp = Function::Create( ft4, linkage, "DaoJIT_SETF_TPP", llvm_module );

	std::vector<Type*> param1( 1, void_type_p );
	FunctionType *sliceft = FunctionType::get( int32_type, param1, false );
	daojit_array_sliced = Function::Create( sliceft, linkage, "DaoArray_Sliced", llvm_module );
	 
	std::vector<Type*> double2( 2, double_type );
	FunctionType *funtype = FunctionType::get( double_type, double2, false );
	daojit_pow_double = Function::Create( funtype, linkage, "pow", llvm_module );

	std::vector<Type*> double1( 1, double_type );
	std::vector<Type*> voidp1( 1, void_type_p );
	FunctionType *mathft = FunctionType::get( double_type, double1, false );
	FunctionType *debugft = FunctionType::get( void_type, voidp1, false );
	daojit_debug_function = Function::Create( debugft, linkage, "daojit_debug", llvm_module );
	daojit_abs_double = Function::Create( mathft, linkage, "abs", llvm_module );
	daojit_acos_double = Function::Create( mathft, linkage, "acos", llvm_module );
	daojit_asin_double = Function::Create( mathft, linkage, "asin", llvm_module );
	daojit_atan_double = Function::Create( mathft, linkage, "atan", llvm_module );
	daojit_ceil_double = Function::Create( mathft, linkage, "ceil", llvm_module );
	daojit_cos_double = Function::Create( mathft, linkage, "cos", llvm_module );
	daojit_cosh_double = Function::Create( mathft, linkage, "cosh", llvm_module );
	daojit_exp_double = Function::Create( mathft, linkage, "exp", llvm_module );
	daojit_floor_double = Function::Create( mathft, linkage, "floor", llvm_module );
	daojit_log_double = Function::Create( mathft, linkage, "log", llvm_module );
	daojit_rand_double = Function::Create( mathft, linkage, "daojit_rand", llvm_module );
	daojit_sin_double = Function::Create( mathft, linkage, "sin", llvm_module );
	daojit_sinh_double = Function::Create( mathft, linkage, "sinh", llvm_module );
	daojit_sqrt_double = Function::Create( mathft, linkage, "sqrt", llvm_module );
	daojit_tan_double = Function::Create( mathft, linkage, "tan", llvm_module );
	daojit_tanh_double = Function::Create( mathft, linkage, "tanh", llvm_module );

	llvm_exe_engine = EngineBuilder( llvm_module ).setEngineKind(EngineKind::JIT).create();
#if 0
	llvm_exe_engine->addGlobalMapping( daojit_rand_double, (void*) daojit_rand );
	llvm_exe_engine->addGlobalMapping( daojit_pow_double, (void*) pow );
	llvm_exe_engine->addGlobalMapping( daojit_abs_double, (void*) abs );
	llvm_exe_engine->addGlobalMapping( daojit_acos_double, (void*) acos );
	llvm_exe_engine->addGlobalMapping( daojit_asin_double, (void*) asin );
	llvm_exe_engine->addGlobalMapping( daojit_atan_double, (void*) atan );
	llvm_exe_engine->addGlobalMapping( daojit_ceil_double, (void*) ceil );
	llvm_exe_engine->addGlobalMapping( daojit_cos_double, (void*) cos );
	llvm_exe_engine->addGlobalMapping( daojit_cosh_double, (void*) cosh );
	llvm_exe_engine->addGlobalMapping( daojit_exp_double, (void*) exp );
	llvm_exe_engine->addGlobalMapping( daojit_floor_double, (void*) floor );
	llvm_exe_engine->addGlobalMapping( daojit_log_double, (void*) log );
	llvm_exe_engine->addGlobalMapping( daojit_sin_double, (void*) sin );
	llvm_exe_engine->addGlobalMapping( daojit_sinh_double, (void*) sinh );
	llvm_exe_engine->addGlobalMapping( daojit_sqrt_double, (void*) sqrt );
	llvm_exe_engine->addGlobalMapping( daojit_tan_double, (void*) tan );
#endif

	llvm_func_optimizer = new FunctionPassManager( llvm_module );
	//llvm_func_optimizer->add(new DataLayout(*llvm_exe_engine->getDataLayout()));
	llvm_func_optimizer->add(createBasicAliasAnalysisPass());
	llvm_func_optimizer->add(createInstructionCombiningPass());
	llvm_func_optimizer->add(createReassociatePass());
	llvm_func_optimizer->add(createGVNPass());
	//llvm_func_optimizer->add(createCFGSimplificationPass());
	llvm_func_optimizer->doInitialization();
}
void DaoJIT_Quit()
{
	delete llvm_func_optimizer;
	delete llvm_exe_engine;
	delete llvm_context;
}


// Create a function with signature: void (DaoProcess*,DaoRoutine*)
Function* DaoJitHandle::NewFunction( DaoRoutine *routine, int id )
{
	int i;
	char buf[100];
	std::string name = routine->routName->chars;
	sprintf( buf, "_daojit_%p_%i", routine, id );
	name += buf;

	jitFunction = cast<Function>( llvm_module->getOrInsertFunction( name, daojit_function_type ) );
	entryBlock = BasicBlock::Create( *llvm_context, "EntryBlock", jitFunction );
	secondBlock = BasicBlock::Create( *llvm_context, "Second", jitFunction );
	lastBlock = secondBlock;
	SetInsertPoint( entryBlock );

	Argument *jitcdata = jitFunction->arg_begin();
	jitcdata->setName("JitCallData");

	Value *value = CreateConstGEP2_32( jitcdata, 0, 0 ); // jitcdata->localValues: DaoValue*[]**
	localValues = CreateLoad( value ); // jitcdata->localValues: DaoValue*[]*

	value = CreateConstGEP2_32( jitcdata, 0, 1 ); // jitcdata->localConsts: DaoValue*[]**
	localConsts = CreateLoad( value ); // jitcdata->localConsts: DaoValue*[]*

	estatus = CreateAlloca( int32_type );
	Constant *cst = ConstantInt::get( int32_type, 0 );
	CreateStore( cst, estatus );

	directValues.resize( routine->body->vmCodes->size );
	for(i=0; i<routine->body->vmCodes->size; i++) directValues[i] = NULL;
	stackValues.resize( routine->body->regCount );
	localRefers.resize( routine->body->regCount );
	dataItems.resize( routine->body->regCount );
	for(i=0; i<routine->body->regCount; i++){
		stackValues[i] = NULL;
		localRefers[i] = NULL;
		dataItems[i] = std::pair<Value*,BasicBlock*>( NULL, NULL );
	}
	objectValues = NULL;
	classValues  = NULL;
	classConsts  = NULL;
	globalValues = NULL;
	globalConsts = NULL;
	processes = NULL;
	mapObjectValueRefers.clear();
	mapClassValueRefers.clear();
	mapClassConstValues.clear();
	mapGlobalValueRefers.clear();
	mapGlobalConstValues.clear();

	localValues->setName( "localValues" );
	localConsts->setName( "routConsts" );
#ifdef DEBUG
#endif
	return jitFunction;
}
BasicBlock* DaoJitHandle::NewBlock( int vmc )
{
	char name[ 256 ];
	iplist<BasicBlock> & blist = jitFunction->getBasicBlockList();
	sprintf( name, "block%i", (unsigned int)blist.size() );
	lastBlock = BasicBlock::Create( *llvm_context, name, jitFunction );
	SetInsertPoint( lastBlock );
	return lastBlock;
}
BasicBlock* DaoJitHandle::NewBlock( DaoVmCodeX *vmc )
{
	char name[ 256 ];
	iplist<BasicBlock> & blist = jitFunction->getBasicBlockList();
	sprintf( name, "%s%i", DaoVmCode_GetOpcodeName( vmc->code ), (unsigned int)blist.size() );
	lastBlock = BasicBlock::Create( *llvm_context, name, jitFunction );
	SetInsertPoint( lastBlock );
	return lastBlock;
}


struct IndexRange
{
	int start;
	int end;
	IndexRange( int s=0, int e=0 ){ start = s; end = e; }

	bool operator<( const IndexRange & other )const{
		return end < other.start;
	}
};


extern DMutex mutex_routine_specialize;
void DaoJIT_Free( void *jitdata )
{
	/* LLVMContext provides no locking guarantees: */
	DMutex_Lock( & mutex_routine_specialize );
	std::vector<DaoJitFunctionData> *jitFuncs = (std::vector<DaoJitFunctionData>*) jitdata;
	for(int i=0,n=jitFuncs->size(); i<n; i++) jitFuncs->operator[](i).llvmFunction->eraseFromParent();
	delete jitFuncs;
	DMutex_Unlock( & mutex_routine_specialize );
}
static bool CompilableSETI( DaoVmCodeX *vmc, DaoType **types )
{
	//printf("%s %s %s\n",types[vmc->a]->name->chars,types[vmc->b]->name->chars,types[vmc->c]->name->chars);
	if( types[vmc->c]->tid != DAO_ARRAY ) return false;
	if( types[vmc->b]->tid != DAO_NONE ) return false;
	if( types[vmc->a]->tid == DAO_ARRAY ) return true;
	return types[vmc->a]->tid >= DAO_BOOLEAN && types[vmc->a]->tid <= DAO_COMPLEX;
}
static bool CompilableArrayBinOp( DaoVmCodeX *vmc, DaoType **types )
{
	DaoType *at = types[vmc->a];
	DaoType *bt = types[vmc->b];
	//printf("%s %s %s\n",types[vmc->a]->name->chars,types[vmc->b]->name->chars,types[vmc->c]->name->chars);
	if( types[vmc->c]->tid != DAO_ARRAY ) return false;
	if( at->tid != DAO_ARRAY && (at->tid < DAO_INTEGER || at->tid > DAO_COMPLEX) ) return false;
	if( bt->tid != DAO_ARRAY && (bt->tid < DAO_INTEGER || bt->tid > DAO_COMPLEX) ) return false;
	return true;
}
/*
A compilable block is a block of virtual instructions that only branch within the block, 
or just branch to the instruction right after this block.
*/
void DaoJIT_SearchCompilable( DaoRoutine *routine, std::vector<IndexRange> & segments )
{
	std::map<IndexRange,int> ranges;
	std::map<IndexRange,int>::iterator it;
	DaoValue **routConsts = routine->routConsts->value->items.pValue;
	DaoType **types = routine->body->regType->items.pType;
	DaoVmCodeX *vmc, **vmcs = routine->body->annotCodes->items.pVmc;
	int i, j, m, jump, N = routine->body->annotCodes->size;
	bool compilable, last = false;
	size_t k;
	int case_mode = DAO_CASE_UNORDERED;
	for(i=0; i<N; i++){ // find the maximum blocks
		vmc = vmcs[i];
		compilable = daojit_opcode_compilable[ vmc->code ];
		if( vmc->code != DVM_CASE ) case_mode = DAO_CASE_UNORDERED;
		// all branching instructions are assumed to be jit compilable for now,
		// so that they can be checked in the next stage:
		switch( vmc->code ){
		case DVM_MATH :
		case DVM_MATH_I :
		case DVM_MATH_F :
			j = types[vmc->b]->tid;
			m = types[vmc->c]->tid;
			compilable = j and j <= DAO_FLOAT and m and m <= DAO_FLOAT;
			break;
		case DVM_SETI :
			compilable = CompilableSETI( vmc, types );
			break;
		case DVM_ADD :
		case DVM_SUB :
		case DVM_MUL :
		case DVM_DIV :
		case DVM_MOD :
		case DVM_POW :
			compilable = CompilableArrayBinOp( vmc, types );
			break;
		case DVM_DATA : compilable = vmc->a <= DAO_FLOAT; break;
		case DVM_GETCL : compilable = vmc->a == 0; break;
		default : break;
		}
		if( compilable ){
			if( last ){
				segments.back().end = i;
			}else{
				segments.push_back( IndexRange( i, i ) );
			}
		}
		last = compilable;
#ifdef DEBUG
		printf( "%3i  %i: ", i, compilable ); DaoVmCodeX_Print( *vmc, NULL, NULL );
#endif
	}
	for(k=0; k<segments.size(); k++) ranges[segments[k]] = 1;
	for(k=0; k<segments.size(); k++){
		int code, start = segments[k].start;
		int end = segments[k].end;
		bool modified = false;
		for(j=start; j<=end; j++){
			vmc = vmcs[j];
			code = vmc->code;
			jump = vmc->b;
			if( code != DVM_CASE ) case_mode = DAO_CASE_UNORDERED;
			//printf( "checking %3i: ", j ); DaoVmCodeX_Print( *vmc, NULL );
			switch( code ){
			case DVM_GOTO : case DVM_TEST :
			case DVM_SWITCH : case DVM_CASE :
			case DVM_TEST_B : case DVM_TEST_I : case DVM_TEST_F :
				compilable = false;
				switch( code ){
				case DVM_GOTO : case DVM_TEST_B : case DVM_TEST_I : case DVM_TEST_F :
					// branchs out of the block
					compilable = vmc->b >= start and vmc->b <= (end+1);
					break;
				case DVM_SWITCH : 
					m = types[vmc->a]->tid;
					if( m == DAO_INTEGER or m == DAO_ENUM ) case_mode = vmcs[j+1]->c; // first case
					compilable = case_mode >= DAO_CASE_INTS;
					if( vmc->b < start or vmc->b > (end+1) ) compilable = false;
					break;
				case DVM_CASE : 
					case_mode = vmc->c;
					compilable = case_mode >= DAO_CASE_INTS;
					m = routConsts[ vmc->a ]->type;
					if( m != DAO_INTEGER and m != DAO_ENUM ) compilable = false;
					if( vmc->b < start or vmc->b >= (end+1) ) compilable = false;
					break;
				}
				if( compilable == false ){
					//printf( "%3i  %i: ", j, compilable ); DaoVmCodeX_Print( *vmc, NULL );
					// break the block:
					segments.push_back( IndexRange( start, j-1 ) );
					segments.push_back( IndexRange( j+1, end ) );
					// check branching into another block:
					it = ranges.find( IndexRange( j, j ) );
					if( it != ranges.end() ) ranges.erase( it );
					ranges[ IndexRange( start, j-1 ) ] = 1;
					ranges[ IndexRange( j+1, end ) ] = 1;
					// check branching inside block:
					if( jump >= start and jump <= end ){
						it = ranges.find( IndexRange( jump, jump ) );
						if( it != ranges.end() ) ranges.erase( it );
						if( jump < j ){
							segments.push_back( IndexRange( start, jump-1 ) );
							segments.push_back( IndexRange( jump+1, j-1 ) );
							ranges[ IndexRange( start, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, j-1 ) ] = 1;
						}else if( jump > j ){
							segments.push_back( IndexRange( j+1, jump-1 ) );
							segments.push_back( IndexRange( jump+1, end ) );
							ranges[ IndexRange( j+1, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, end ) ] = 1;
						}
					}else{ // branching into another block:
						it = ranges.find( IndexRange( jump, jump ) );
						if( it != ranges.end() ){
							int start2 = it->first.start;
							int end2 = it->first.end;
							ranges.erase( it );
							segments.push_back( IndexRange( start2, jump-1 ) );
							segments.push_back( IndexRange( jump+1, end2 ) );
							ranges[ IndexRange( start2, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, end2 ) ] = 1;
						}
					}
					modified = true;
				}
				break;
			}
			if( modified ) break;
		}
	}
	segments.clear();
	for(it=ranges.begin(); it!=ranges.end(); it++){
		if( it->first.start <= it->first.end ) segments.push_back( it->first );
	}
#ifdef DEBUG
	printf( "number of segments: %i\n", (int)segments.size() );
	for(k=0;k<segments.size();k++) printf( "%3li:%5i%5i\n", k, segments[k].start, segments[k].end );
#endif
}


Value* DaoJitHandle::GetLocalConstant( int id )
{
	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	Value *value = CreateConstGEP2_32( localConsts, 0, id );
	value = CreateLoad( value );
	SetValueName( value, "locst", id );
	SetInsertPoint( current );
	return value;
}
void DaoJitHandle::SetValueName( Value *value, const char *name, int id )
{
	char buf[100];
	sprintf( buf, "%s_%i_", name, id );
	value->setName( buf );
}
Value* DaoJitHandle::GetLocalReference( int reg )
{
	BasicBlock *current = GetInsertBlock();
	Value *refer;
	if( localRefers[reg] ) return localRefers[reg];
	SetInsertPoint( entryBlock );
	refer = CreateConstGEP2_32( localValues, 0, reg );
	localRefers[reg] = refer;
	SetValueName( refer, "loref", reg );
	SetInsertPoint( current );
	return refer;
}
Value* DaoJitHandle::GetLocalValue( int reg )
{
	Value *value;
	dataItems[reg] = std::pair<Value*,BasicBlock*>( NULL, NULL );
	value = GetLocalReference( reg );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectValueRefer( int index )
{
	Value *value;
	std::map<int,Value*>::iterator it = mapObjectValueRefers.find( index );
	if( it != mapObjectValueRefers.end() ) return it->second;

	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( objectValues == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		objectValues = CreateConstGEP2_32( jitcdata, 0, 2 );
		objectValues = CreateLoad( objectValues );
	}
	value = CreateConstGEP2_32( objectValues, 0, index );
	mapObjectValueRefers[index] = value;
	SetValueName( value, "OVR", index );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetObjectValueValue( int index )
{
	Value *value;
	value = GetObjectValueRefer( index );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetClassValueRefer( int index )
{
	Value *value;
	std::map<int,Value*>::iterator it = mapClassValueRefers.find( index );
	if( it != mapClassValueRefers.end() ) return it->second;

	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( classValues == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		classValues = CreateConstGEP2_32( jitcdata, 0, 3 );
		classValues = CreateLoad( classValues );
	}
	value = CreateConstGEP2_32( classValues, 0, index );
	value = CreateLoad( value ); // DaoVariable
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	mapClassValueRefers[index] = value;
	SetValueName( value, "OVR", index );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetClassValueValue( int index )
{
	Value *value;
	value = GetClassValueRefer( index );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetGlobalValueRefer( int index )
{
	Value *value;
	std::map<int,Value*>::iterator it = mapGlobalValueRefers.find( index );
	if( it != mapGlobalValueRefers.end() ) return it->second;

	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( globalValues == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		globalValues = CreateConstGEP2_32( jitcdata, 0, 5 );
		globalValues = CreateLoad( globalValues );
	}
	value = CreateConstGEP2_32( globalValues, 0, index );
	value = CreateLoad( value ); // DaoVariable
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	mapGlobalValueRefers[index] = value;
	SetValueName( value, "OVR", index );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetGlobalValueValue( int index )
{
	Value *value;
	value = GetGlobalValueRefer( index );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetClassConstValue( int index )
{
	Value *value;
	std::map<int,Value*>::iterator it = mapClassConstValues.find( index );
	if( it != mapClassConstValues.end() ) return it->second;

	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( classConsts == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		classConsts = CreateConstGEP2_32( jitcdata, 0, 4 );
		classConsts = CreateLoad( classConsts );
	}
	value = CreateConstGEP2_32( classConsts, 0, index );
	value = CreateLoad( value ); // DaoConstant
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	value = CreateLoad( value );
	mapClassConstValues[index] = value;
	SetValueName( value, "OVR", index );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetGlobalConstValue( int index )
{
	Value *value;
	std::map<int,Value*>::iterator it = mapGlobalConstValues.find( index );
	if( it != mapGlobalConstValues.end() ) return it->second;

	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( globalConsts == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		globalConsts = CreateConstGEP2_32( jitcdata, 0, 6 );
		globalConsts = CreateLoad( globalConsts );
	}
	value = CreateConstGEP2_32( globalConsts, 0, index );
	value = CreateLoad( value ); // DaoConstant
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	value = CreateLoad( value );
	mapGlobalConstValues[index] = value;
	SetValueName( value, "OVR", index );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetUpValueRefer( int up, int index )
{
	Value *value;
	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	if( processes == NULL ){
		Argument *jitcdata = jitFunction->arg_begin();
		processes = CreateConstGEP2_32( jitcdata, 0, 7 );
		processes = CreateLoad( processes );
	}
	value = CreateConstGEP2_32( processes, 0, up );
	value = CreateLoad( value ); // DaoProcess*
	value = CreateConstGEP2_32( value, 0, 14 ); // DaoValue*[]*
	SetInsertPoint( current );
	value = CreateLoad( value ); // DaoValue*[]
	value = CreateConstGEP2_32( value, 0, index );
	SetValueName( value, "UP", index );
	return value;
}
Value* DaoJitHandle::GetUpValueValue( int up, int index )
{
	return CreateLoad( GetUpValueRefer( up, index ) );
}

Value* DaoJitHandle::GetValueTypePointer( Value *value )
{
	return CreateConstGEP2_32( value, 0, 0 );
}
Value* DaoJitHandle::GetValueDataPointer( Value *value )
{
	return CreateConstGEP2_32( value, 0, 5 );
}
Value* DaoJitHandle::GetValueNumberPointer( Value *value, Type *type )
{
	value = CreatePointerCast( value, type );
	return CreateConstGEP2_32( value, 0, 5 );
}
Value* DaoJitHandle::GetValueNumberValue( Value *value, Type *type )
{
	return CreateLoad( GetValueNumberPointer( value, type ) );
}
Value* DaoJitHandle::Dereference( Value *value )
{
	return CreateLoad( value );
}


Value* DaoJitHandle::GetNumberOperand( int reg )
{
	BasicBlock *current = GetInsertBlock();
	Value *A = GetDirectValue( reg );
	if( A ) return A;
	if( stackValues[reg] ) return CreateLoad( stackValues[reg] );

	DaoType *type = routine->body->regType->items.pType[reg];

	SetInsertPoint( entryBlock );
	stackValues[reg] = CreateAlloca( cxx_number_types[type->tid - DAO_BOOLEAN] );
	SetValueName( stackValues[reg], "stack", reg );

	A = CreateLoad( GetLocalReference( reg ) );
	A = GetValueNumberValue( A, daojit_number_types[type->tid - DAO_BOOLEAN] );
	CreateStore( A, stackValues[reg] );
	SetInsertPoint( current );
	return CreateLoad( stackValues[reg] );
}
Value* DaoJitHandle::GetValueItem( Value *array, Value *index )
{
	std::vector<Value*> ids;
	if( sizeof(void*) == 8 ){
		ids.push_back( getInt64( 0 ) );
	}else{
		ids.push_back( getInt32( 0 ) );
	}
	ids.push_back( index );
	return CreateGEP( array, ids );
}
Value* DaoJitHandle::GetTupleItems( int reg )
{
	Value *value;
	DaoCnode *node;
	BasicBlock *block = GetInsertBlock();
	daoint i, k = 0;
	for(i=0; i<currentNode->defs->size; i++){
		node = currentNode->defs->items.pCnode[i];
		if( node->lvalue == reg ) k += 1;
	}
	if( k == 1 && dataItems[reg].first && dataItems[reg].second == block )
		return dataItems[reg].first;

	value = GetLocalValue( reg );
	SetInsertPoint( block );
	value = CreatePointerCast( value, daojit_tuple_type_p ); // DaoTuple*
	value = CreateConstGEP2_32( value, 0, 8 ); // tuple->values: DaoValue*[]*
	if( k == 1 ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, block );
	return value;
}
void DaoJitHandle::AddReturnCodeChecking( Value *retcode, int vmc )
{
	BasicBlock *block = GetInsertBlock();
	Constant *cst = ConstantInt::get( int32_type, 0 );

	retcode = CreateLoad( retcode );
	Value *cmp = CreateICmpNE( retcode, cst );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );
	SetInsertPoint( block );
	CreateCondBr( cmp, bltrue, blfalse );
	SetInsertPoint( bltrue );
	cst = ConstantInt::get( int32_type, vmc );
	retcode = CreateOr( retcode, cst );
	CreateRet( retcode );
	SetInsertPoint( blfalse );
	lastBlock = blfalse;
}
// index: dao_integer_type
// size: any integer type
Value* DaoJitHandle::AddIndexChecking( Value *index, Value *size, int vmc )
{
	BasicBlock *block = GetInsertBlock();

	size = Dereference( size );
	size = CreateIntCast( size, dao_integer_type, false );

	Constant *zero = ConstantInt::get( dao_integer_type, 0 );
	Value *index2 = CreateAdd( index, size );
	Value *cmp = CreateICmpSLT( index, zero );
	index = CreateSelect( cmp, index2, index );
	cmp = CreateOr( CreateICmpSLT( index, zero ), CreateICmpSGE( index, size ) );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );
	SetInsertPoint( block );
	CreateCondBr( cmp, bltrue, blfalse );
	SetInsertPoint( bltrue );
	CreateRet( ConstantInt::get( int32_type, (DAO_ERROR_INDEX<<16)|vmc ) );
	SetInsertPoint( blfalse );
	lastBlock = blfalse;
	return index;
}
Value* DaoJitHandle::GetArrayItem( int reg, int index, int vmc )
{
	DaoType *type = routine->body->regType->items.pType[reg];
	BasicBlock *current = GetInsertBlock();
	ConstantPointerNull *null = ConstantPointerNull::get( void_type_p );
	Constant *zero = ConstantInt::get( dao_integer_type, 0 );
	Constant *zero2 = ConstantInt::get( int32_type, 0 );
	Value *value = GetLocalValue( reg );
	int tid = DAO_INTEGER;

	if( type && type->nested && type->nested->size ) type = type->nested->items.pType[0];
	if( type && type->tid && type->tid <= DAO_COMPLEX ) tid = type->tid;

	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_array_types[tid - DAO_BOOLEAN] );

	Value *original = CreateConstGEP2_32( value, 0, 12 ); // array->original
	original = CreateLoad( original );
	original = CreateIsNotNull( original );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blret = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );

	SetInsertPoint( current );
	CreateCondBr( original, bltrue, blfalse );

	SetInsertPoint( bltrue );
	original = CreatePointerCast( value, void_type_p );
	original = CreateCall( daojit_array_sliced, original );
	original = CreateICmpEQ( original, zero2 );
	CreateCondBr( original, blret, blfalse );

	SetInsertPoint( blret );
	CreateRet( ConstantInt::get( int32_type, (DAO_ERROR_INDEX<<16)|vmc ) );

	SetInsertPoint( blfalse );
	lastBlock = blfalse;

	Value *id = GetNumberOperand( index );
	Value *size = CreateConstGEP2_32( value, 0, 9 ); // array->size
	id = AddIndexChecking( id, size, vmc );

	value = CreateConstGEP2_32( value, 0, 11 ); // array->data.x: dao_integer/dao_float**
	value = Dereference( value ); // array->data.x: dao_integer/dao_float*
	std::vector<Value*> ids;
	ids.push_back( zero );
	ids.push_back( id );
	return CreateGEP( value, ids );
}
Value* DaoJitHandle::GetArrayItemMI( int reg, int index, int vmc )
{
	DaoType *type = routine->body->regType->items.pType[reg];
	BasicBlock *current = GetInsertBlock();
	ConstantPointerNull *null = ConstantPointerNull::get( void_type_p );
	Constant *zero = ConstantInt::get( daoint_type, 0 );
	Constant *zero2 = ConstantInt::get( int32_type, 0 );
	Constant *idm = ConstantInt::get( int16_type, index );
	Value *value = GetLocalValue( reg );
	int i, tid = DAO_INTEGER;

	if( type && type->nested && type->nested->size ) type = type->nested->items.pType[0];
	if( type && type->tid && type->tid <= DAO_COMPLEX ) tid = type->tid;

	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_array_types[tid - DAO_BOOLEAN] );

	Value *original = CreateConstGEP2_32( value, 0, 12 ); // array->original
	SetValueName( original, "original", vmc );
	original = CreateLoad( original );
	original = CreateIsNotNull( original );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blret = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );
	BasicBlock *blfalse2 = NewBlock( vmc );

	SetInsertPoint( current );
	CreateCondBr( original, bltrue, blfalse );

	SetInsertPoint( bltrue );
	original = CreatePointerCast( value, void_type_p );
	original = CreateCall( daojit_array_sliced, original );
	original = CreateICmpEQ( original, zero2 );
	CreateCondBr( original, blret, blfalse );

	SetInsertPoint( blret );
	CreateRet( ConstantInt::get( int32_type, (DAO_ERROR_INDEX<<16)|vmc ) );

	SetInsertPoint( blfalse );
	lastBlock = blfalse;

	Value *ndim = CreateLoad( CreateConstGEP2_32( value, 0, 8 ) ); // array->ndim
	Value *cmp = CreateICmpUGT( idm, ndim );
	CreateCondBr( cmp, blret, blfalse2 );

	SetValueName( ndim, "ndim", vmc );

	SetInsertPoint( blfalse2 );
	lastBlock = blfalse;

	Value *dims = CreateLoad( CreateConstGEP2_32( value, 0, 10 ) ); // array->dims
	SetValueName( dims, "dims", vmc );

	std::vector<Value*> ids;
	ids.push_back( zero );
	ids.push_back( ndim );
	Value *accums = CreateGEP( dims, ids );
	accums = CreatePointerCast( accums, daoint_array_type_p );
	SetValueName( accums, "accums", vmc );

	Value *id = zero;
	for(i=0; i<index; i++){
		Value *dim = CreateLoad( CreateConstGEP2_32( dims, 0, i ) );
		Value *acc = CreateLoad( CreateConstGEP2_32( accums, 0, i ) );
		Value *idx = GetNumberOperand( reg + i + 1 );

		idx = CreateIntCast( idx, daoint_type, false );

		Value *idx2 = CreateAdd( idx, dim );
		Value *cmp = CreateICmpSLT( idx, zero );
		idx = CreateSelect( cmp, idx2, idx );
		cmp = CreateOr( CreateICmpSLT( idx, zero ), CreateICmpSGE( idx, dim ) );

		BasicBlock *current = GetInsertBlock();
		BasicBlock *blfalse3 = NewBlock( vmc );

		SetInsertPoint( current );
		CreateCondBr( cmp, blret, blfalse3 );

		SetInsertPoint( blfalse3 );
		id = CreateAdd( id, CreateMul( idx, acc ) );
	}

	Value *size = CreateLoad( CreateConstGEP2_32( value, 0, 9 ) ); // array->size
	cmp = CreateOr( CreateICmpSLT( id, zero ), CreateICmpSGE( id, size ) );

	current = GetInsertBlock();
	BasicBlock *blfalse3 = NewBlock( vmc );

	SetInsertPoint( current );
	CreateCondBr( cmp, blret, blfalse3 );
	SetInsertPoint( blfalse3 );

	value = CreateConstGEP2_32( value, 0, 11 ); // array->data.x: daoint/float/double**
	value = Dereference( value ); // array->data.x: daoint/float/double*
	ids[0] = zero;
	ids[1] = id;
	return CreateGEP( value, ids );
}
Value* DaoJitHandle::GetListItem( int reg, int index, int vmc )
{
	BasicBlock *current = GetInsertBlock();
	Constant *zero = ConstantInt::get( dao_integer_type, 0 );
	Value *value = GetLocalValue( reg );
	Value *id = GetNumberOperand( index );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_list_type_p );
	value = CreateConstGEP2_32( value, 0, 7 ); // list->items: DArray**
	value = Dereference( value ); // list->items: DArray*

	Value *size = CreateConstGEP2_32( value, 0, 1 );
	id = AddIndexChecking( id, size, vmc );

	value = CreateConstGEP2_32( value, 0, 0 ); // list->value->items.pValue: DaoValue*[]**
	value = Dereference( value ); // list->value->items.pValue: DaoValue*[]*
	std::vector<Value*> ids;
	ids.push_back( zero );
	ids.push_back( id );
	value = CreateGEP( value, ids );
	return Dereference( value );
}
Value* DaoJitHandle::GetItemValue( int reg, int field, int *maycache )
{
	Value *value;
	DaoCnode *node;
	BasicBlock *current = GetInsertBlock();
	daoint i, k = 0;
	for(i=0; i<currentNode->defs->size; i++){
		node = currentNode->defs->items.pCnode[i];
		if( node->lvalue == reg ) k += 1;
	}
	*maycache = k == 1;
	if( k == 1 && dataItems[reg].first && dataItems[reg].second == current ){
		value = dataItems[reg].first;
		value = CreateConstGEP2_32( value, 0, field );
		value = CreateLoad( value );
		return value;
	}
	return NULL;
}
Value* DaoJitHandle::GetClassConstant( int reg, int field )
{
	int maycache = 0;
	BasicBlock *current = GetInsertBlock();
	Value *value = GetItemValue( reg, field, & maycache );
	if( value ) return value;

	value = GetLocalValue( reg );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_class_type_p );
	value = CreateConstGEP2_32( value, 0, 8 ); // klass->constants: DArray**;
	value = Dereference( value ); // klass->constants: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->constants->items.pConst: DaoConstant*[]**
	value = Dereference( value ); // klass->constants->items.pConst: DaoConstant*[]*
	if( maycache ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, current );

	value = CreateConstGEP2_32( value, 0, field ); // klass->constants->items.pConst[i] 
	value = CreateLoad( value ); // klass->constants->items.pConst[i]
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	return CreateLoad( value );
}
Value* DaoJitHandle::GetClassStatic( int reg, int field )
{
	int maycache = 0;
	BasicBlock *current = GetInsertBlock();
	Value *value = GetItemValue( reg, field, & maycache );
	if( value ) return value;

	value = GetLocalValue( reg );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_class_type_p );
	value = CreateConstGEP2_32( value, 0, 9 ); // klass->variables: DArray**;
	value = Dereference( value ); // klass->variables: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->variables->items.pVar: DaoValue*[]**
	value = Dereference( value ); // klass->variables->items.pVar: DaoValue*[]*
	if( maycache ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, current );

	value = CreateConstGEP2_32( value, 0, field );
	value = CreateLoad( value ); // klass->variables->items.pVar[i]
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectConstant( int reg, int field )
{
	int maycache = 0;
	BasicBlock *current = GetInsertBlock();
	Value *value = GetItemValue( reg, field, & maycache );
	if( value ) return value;

	value = GetLocalValue( reg );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_object_type_p );
	value = CreateConstGEP2_32( value, 0, 8 /*bit fields as one*/ ); // object->defClass: DaoClass**;
	value = Dereference( value ); // object->defClass: DaoClass*;
	value = CreateConstGEP2_32( value, 0, 8 ); // klass->constants: DArray**;
	value = Dereference( value ); // klass->constants: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->constants->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->constants->data: DaoValue*[]*
	if( maycache ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, current );

	value = CreateConstGEP2_32( value, 0, field );
	value = CreateLoad( value ); // DaoConstant
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectStatic( int reg, int field )
{
	int maycache = 0;
	BasicBlock *current = GetInsertBlock();
	Value *value = GetItemValue( reg, field, & maycache );
	if( value ) return value;

	value = GetLocalValue( reg );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_object_type_p );
	value = CreateConstGEP2_32( value, 0, 8 /*bit fields as one*/ ); // object->defClass: DaoClass**;
	value = Dereference( value ); // object->defClass: DaoClass*;
	value = CreateConstGEP2_32( value, 0, 9 ); // klass->variables: DArray**;
	value = Dereference( value ); // klass->variables: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->variables->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->variables->data: DaoValue*[]*
	if( maycache ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, current );

	value = CreateConstGEP2_32( value, 0, field );
	value = CreateLoad( value ); // DaoVariable
	value = CreateConstGEP2_32( value, 0, 6 ); // DaoValue**
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectVariable( int reg, int field )
{
	int maycache = 0;
	BasicBlock *current = GetInsertBlock();
	Value *value = GetItemValue( reg, field, & maycache );
	if( value ) return value;

	value = GetLocalValue( reg );
	SetInsertPoint( current );
	value = CreatePointerCast( value, daojit_object_type_p );
	value = CreateConstGEP2_32( value, 0, 11 /*bit fields*/ ); // object->objValues: DaoValue*[]**;
	value = Dereference( value ); // object->objValues: DaoValue*[]*;
	if( maycache ) dataItems[reg] = std::pair<Value*,BasicBlock*>( value, current );

	value = CreateConstGEP2_32( value, 0, field );
	value = CreateLoad( value );
	SetValueName( value, "OV", field );
	return value;
}
int DaoJitHandle::IsDirectValue( int reg )
{
	daoint i, j, m, n, dcount = 0;
	DaoCnode *node;
	for(i=0,n=currentNode->uses->size; i<n; i++){
		dcount = 0;
		node = currentNode->uses->items.pCnode[i];
		if( node->index < start || node->index > end ) return 0; // Used outside of compilable codes;
		for(j=0,m=node->defs->size; j<m; j++) dcount += node->defs->items.pCnode[j]->lvalue == reg;
		if( dcount > 1 ) return 0; // Multi-defintion value;
	}
	return 1;
}
Value* DaoJitHandle::GetDirectValue( int reg )
{
	DaoCnode *def = NULL;
	daoint i, n, ndef = 0;
	for(i=0,n=currentNode->defs->size; i<n; i++){
		DaoCnode *node = currentNode->defs->items.pCnode[i];
		if( node->lvalue != reg ) continue;
		if( node->index < start || node->index > end ) continue;
		def = node;
		ndef += 1;
		if( ndef > 1 ) return NULL;
	}
	if( def == NULL ) return NULL;
	return directValues[ def->index ];
}
void DaoJitHandle::StoreNumber( Value *value, int reg )
{
	BasicBlock *current = GetInsertBlock();
	//printf( "%3i  %i  %p\n", reg, IsDirectValue( reg ), directValues[ currentNode->index ] );
	if( IsDirectValue( reg ) ){
		SetValueName( value, "direct", reg );
		directValues[ currentNode->index ] = value;
		return;
	}
	if( stackValues[reg] == NULL ){
		DaoType *type = routine->body->regType->items.pType[reg];
		SetInsertPoint( entryBlock );
		stackValues[reg] = CreateAlloca( cxx_number_types[type->tid - DAO_BOOLEAN] );
		SetValueName( stackValues[reg], "stack", reg );
		if( DaoCnode_FindResult( lastNode, IntToPointer(reg) ) >= 0 ){
			// This stack value is alive at the end of the jit codes
			// and will be wrote back to the VM stack,
			// so it is necessary to initialize it with the current VM stack value,
			// in case that the following store is not executed!
			Value *A = CreateLoad( GetLocalReference( reg ) );
			A = GetValueNumberValue( A, daojit_number_types[type->tid - DAO_BOOLEAN] );
			CreateStore( A, stackValues[reg] );
		}
	}
	SetInsertPoint( current );
	CreateStore( value, stackValues[reg] );
}
Value* DaoJitHandle::MoveValue( Value *dA, Value *dC, Type *type )
{
	dA = GetValueNumberPointer( dA, type );
	dC = GetValueNumberPointer( dC, type );
	dA = Dereference( dA );
	return CreateStore( dA, dC );
}
Function* DaoJitHandle::Compile( int start, int end )
{
	DaoCnode *node, **nodes = optimizer->nodes->items.pCnode;
	DaoValue **routConsts = routine->routConsts->value->items.pValue;
	DaoType *at, *bt, *ct, **types = routine->body->regType->items.pType;
	DaoVmCodeX *vmc, **vmcs = routine->body->annotCodes->items.pVmc;
	Function *jitfunc = NewFunction( routine, start );
	Function *mathfunc = NULL;

	BasicBlock *current;
	Argument *arg_context = jitFunction->arg_begin();
	Constant *zero32 = ConstantInt::get( int32_type, 0 );
	Constant *zero8 = ConstantInt::get( int8_type, 0 );
	Constant *dao_integer_zero = ConstantInt::get( dao_integer_type, 0 );
	Constant *dao_float_zero = ConstantExpr::getSIToFP( dao_integer_zero, dao_float_type );
	Constant *dao_complex_zero = ConstantStruct::get( dao_complex_type, dao_float_zero, dao_float_zero, NULL );
	Value *type, *vdata, *dA, *dB, *dC, *real1, *real2, *imag1, *imag2, *tmp;
	Value *value=NULL, *refer=NULL;
	Type *numtype = NULL;
	ConstantInt *caseint;
	SwitchInst *inswitch;
	int code, i, k, m;

	std::map<Value*,Value*> stores;
	std::map<int,Value*> comparisons;
	std::map<int,BasicBlock*> branchings;
	std::map<int,BasicBlock*> labels;
	std::map<int,BasicBlock*>::iterator iter, stop;
	std::vector<unsigned> fidx0( 1, 0 );
	std::vector<unsigned> fidx1( 1, 1 );

	this->start = start;
	this->end = end;
	firstNode = nodes[start];
	lastNode = nodes[end];
	for(i=start; i<=end; i++){
		vmc = vmcs[i];
		code = vmc->code;
		switch( code ){
		case DVM_CASE :
		case DVM_GOTO : case DVM_TEST_B : case DVM_TEST_I : case DVM_TEST_F :
			branchings[i] = NULL;
			labels[i+1] = NULL;
			labels[vmc->b] = NULL;
			if( vmc->b > start ) branchings[vmc->b-1] = NULL;
			break;
		case DVM_SWITCH :
			branchings[i] = NULL;
			labels[i+1] = NULL;
			labels[vmc->b] = NULL;
			branchings[vmc->b-1] = NULL;
			// use DVM_CASE to add labels
			for(k=1; k<=vmc->c; k++) labels[vmcs[i+k]->b] = NULL;
			break;
		}
	}

	dC = NULL;
	SetInsertPoint( secondBlock );
	for(i=start; i<=end; i++){
		Constant *vmcIndex = ConstantInt::get( int32_type, i );
		vmc = vmcs[i];
		code = vmc->code;
		currentNode = nodes[i];
#ifdef DEBUG
		printf( "%3i ", i ); DaoVmCodeX_Print( *vmc, NULL, NULL );
#endif
		if( labels.find( i ) != labels.end() ) labels[i] = NewBlock( vmc );
		switch( code ){
		case DVM_DATA :
			value = getInt32( (int) vmc->b );
			switch( vmc->a ){
			case DAO_NONE : break;
			case DAO_BOOLEAN : break;
			case DAO_INTEGER : break;
			case DAO_FLOAT : value = CreateUIToFP( value, dao_float_type ); break;
			default: goto Failed;
			}
			if( vmc->a ) StoreNumber( value, vmc->c );
			break;
		case DVM_LOAD :
			value = GetDirectValue(vmc->a);
			dA = GetLocalValue( vmc->a );
			if( stackValues[vmc->a] || value ){
				DaoType *type = types[vmc->a];
				if( value == NULL ) value = CreateLoad( stackValues[vmc->a] );
				dB = GetValueNumberPointer( dA, daojit_number_types[type->tid - DAO_BOOLEAN] );
				CreateStore( value, dB );
			}
			dC = GetLocalReference( vmc->c );
			tmp = CreateCall2( daojit_load, dA, dC );
			break;
		case DVM_GETCL :
			dC = GetLocalReference( vmc->c );
			dB = GetLocalConstant( vmc->b );
			tmp = CreateCall2( daojit_move_pp, dB, dC );
			break;
		case DVM_MATH :
		case DVM_MATH_B :
		case DVM_MATH_I :
		case DVM_MATH_F :
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->b ]->tid ){
			case DAO_BOOLEAN :
			case DAO_INTEGER : dB = CreateSIToFP( dB, dao_float_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, dao_float_type ); break;
			}
			switch( vmc->a ){
			case DVM_MATH_ABS  : mathfunc = daojit_abs_double;  break;
			case DVM_MATH_ACOS : mathfunc = daojit_acos_double; break;
			case DVM_MATH_ASIN : mathfunc = daojit_asin_double; break;
			case DVM_MATH_ATAN : mathfunc = daojit_atan_double; break;
			case DVM_MATH_CEIL : mathfunc = daojit_ceil_double; break;
			case DVM_MATH_COS  : mathfunc = daojit_cos_double;  break;
			case DVM_MATH_COSH : mathfunc = daojit_cosh_double; break;
			case DVM_MATH_EXP  : mathfunc = daojit_exp_double;  break;
			case DVM_MATH_FLOOR : mathfunc = daojit_floor_double; break;
			case DVM_MATH_LOG  : mathfunc = daojit_log_double;  break;
			case DVM_MATH_SIN  : mathfunc = daojit_sin_double;  break;
			case DVM_MATH_SINH : mathfunc = daojit_sinh_double; break;
			case DVM_MATH_SQRT : mathfunc = daojit_sqrt_double; break;
			case DVM_MATH_TAN  : mathfunc = daojit_tan_double;  break;
			case DVM_MATH_TANH : mathfunc = daojit_tanh_double; break;
			default : break;
			}
			dB = CreateCall( mathfunc, dB );
			switch( types[ vmc->c ]->tid ){
			case DAO_BOOLEAN :
			case DAO_INTEGER : dB = CreateFPToSI( dB, daoint_type ); break;
			case DAO_FLOAT : break;
			}
			StoreNumber( dB, vmc->c );
			break;
		case DVM_DATA_B :
		case DVM_DATA_I :
		case DVM_DATA_F :
		case DVM_DATA_C :
			value = sizeof(dao_integer) == 4 ? getInt32( vmc->b ) : getInt64( vmc->b );
			switch( vmc->code ){
			case DVM_DATA_C :
				dA = CreateUIToFP( dao_float_zero, dao_float_type );
				dB = CreateUIToFP( value, dao_float_type );
				value = ConstantStruct::get( dao_complex_type, dA, dB, NULL );
				break;
			case DVM_DATA_B : break;
			case DVM_DATA_I : break;
			case DVM_DATA_F : value = CreateUIToFP( value, dao_float_type ); break;
			default: goto Failed;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETCL_B :
		case DVM_GETCL_I :
		case DVM_GETCL_F :
		case DVM_GETCL_C :
			numtype = daojit_number_types[ vmc->code - DVM_GETCL_B ];
			dB = NULL;
			dB = GetLocalConstant( vmc->b );
			current = GetInsertBlock();
			SetInsertPoint( entryBlock );
			dB = GetValueNumberPointer( dB, numtype );
			dB = CreateLoad( dB );
			SetInsertPoint( current );
			StoreNumber( dB, vmc->c );
			break;
		case DVM_GETCK_B : 
		case DVM_GETCK_I : 
		case DVM_GETCK_F : 
		case DVM_GETCK_C : 
			value = GetClassConstValue( vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETCK_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETCG_B : 
		case DVM_GETCG_I : 
		case DVM_GETCG_F : 
		case DVM_GETCG_C : 
			value = GetGlobalConstValue( vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETCG_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETVH_B : 
		case DVM_GETVH_I : 
		case DVM_GETVH_F : 
		case DVM_GETVH_C : 
			value = GetUpValueValue( vmc->a, vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETVH_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETVO_B : 
		case DVM_GETVO_I : 
		case DVM_GETVO_F : 
		case DVM_GETVO_C : 
			value = GetObjectValueValue( vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETVO_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETVK_B : 
		case DVM_GETVK_I : 
		case DVM_GETVK_F : 
		case DVM_GETVK_C : 
			value = GetClassValueValue( vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETVK_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETVG_B : 
		case DVM_GETVG_I : 
		case DVM_GETVG_F : 
		case DVM_GETVG_C : 
			value = GetGlobalValueValue( vmc->b );
			value = GetValueNumberValue( value, daojit_number_types[vmc->code - DVM_GETVG_B] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_SETVH_BB : 
		case DVM_SETVH_II : 
		case DVM_SETVH_FF : 
		case DVM_SETVH_CC : 
			dA = GetNumberOperand( vmc->a );
			dC = GetUpValueValue( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETVH_BB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETVO_BB : 
		case DVM_SETVO_II : 
		case DVM_SETVO_FF : 
		case DVM_SETVO_CC : 
			dA = GetNumberOperand( vmc->a );
			dC = GetObjectValueValue( vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETVO_BB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETVK_BB : 
		case DVM_SETVK_II : 
		case DVM_SETVK_FF : 
		case DVM_SETVK_CC : 
			dA = GetNumberOperand( vmc->a );
			dC = GetClassValueValue( vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETVK_BB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETVG_BB : 
		case DVM_SETVG_II : 
		case DVM_SETVG_FF : 
		case DVM_SETVG_CC : 
			dA = GetNumberOperand( vmc->a );
			dC = GetGlobalValueValue( vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETVG_BB] );
			CreateStore( dA, dC );
			break;
		case DVM_MOVE_BB : case DVM_MOVE_BI : case DVM_MOVE_BF :
		case DVM_MOVE_IB : case DVM_MOVE_II : case DVM_MOVE_IF :
		case DVM_MOVE_FB : case DVM_MOVE_FI : case DVM_MOVE_FF :
		case DVM_MOVE_CC :
			dA = GetNumberOperand( vmc->a );
			switch( code ){
			case DVM_MOVE_IF : dA = CreateFPToSI( dA, dao_integer_type ); break;
			case DVM_MOVE_FI : dA = CreateSIToFP( dA, dao_float_type ); break;
			}
			StoreNumber( dA, vmc->c );
			break;
		case DVM_MOVE_CF :
			dA = GetNumberOperand( vmc->a );
			dA = CreateFPCast( dA, dao_float_type );
			dC = CreateBitCast( dao_complex_zero, dao_complex_type );
			dC = CreateInsertValue( dC, dA, fidx0 );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_NOT_B :
		case DVM_NOT_I :
			dA = GetNumberOperand( vmc->a );
			dC = CreateICmpEQ( dA, dao_integer_zero );
			dC = CreateIntCast( dC, dao_integer_type, false );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_NOT_F :
			dA = GetNumberOperand( vmc->a );
			dC = CreateFCmpOEQ( dA, dao_float_zero );
			dC = CreateIntCast( dC, dao_integer_type, false );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_MINUS_I :
			dA = GetNumberOperand( vmc->a );
			dC = CreateSub( dao_integer_zero, dA );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_MINUS_F :
			dA = GetNumberOperand( vmc->a );
			dC = CreateFSub( dao_float_zero, dA );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_MINUS_C :
			dA = GetNumberOperand( vmc->a );
			real1 = CreateExtractValue( dA, fidx0 );
			imag1 = CreateExtractValue( dA, fidx1 );
			real1 = CreateFSub( dao_float_zero, real1 );
			imag1 = CreateFSub( dao_float_zero, imag1 );
			dC = CreateBitCast( dao_complex_zero, dao_complex_type );
			dC = CreateInsertValue( dC, real1, fidx0 );
			dC = CreateInsertValue( dC, imag1, fidx1 );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_ADD_III :
		case DVM_SUB_III :
		case DVM_MUL_III :
		case DVM_DIV_III :
		case DVM_MOD_III :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_ADD_III : value = CreateAdd( dA, dB ); break;
			case DVM_SUB_III : value = CreateSub( dA, dB ); break;
			case DVM_MUL_III : value = CreateMul( dA, dB ); break;
			case DVM_DIV_III : value = CreateSDiv( dA, dB ); break;
			case DVM_MOD_III : value = CreateSRem( dA, dB ); break;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_III :
			dA = dB = CreateSIToFP( GetNumberOperand( vmc->a ), dao_float_type );
			if( vmc->a != vmc->b ) dB = CreateSIToFP( GetNumberOperand( vmc->b ), dao_float_type );
			tmp = CreateCall2( daojit_pow_double, dA, dB );
			tmp = CreateFPToSI( tmp, daoint_type );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_AND_BBB :
		case DVM_OR_BBB :
		case DVM_AND_BII :
		case DVM_OR_BII :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_AND_BBB :
			case DVM_AND_BII : value = CreateAnd( dA, dB ); break;
			case DVM_OR_BBB :
			case DVM_OR_BII  : value = CreateOr( dA, dB ); break;
			}
			value = CreateIntCast( value, dao_integer_type, false );
			StoreNumber( value, vmc->c );
			break;
		case DVM_LT_BBB :
		case DVM_LE_BBB :
		case DVM_EQ_BBB :
		case DVM_NE_BBB :
		case DVM_LT_BII :
		case DVM_LE_BII :
		case DVM_EQ_BII :
		case DVM_NE_BII :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_LT_BBB :
			case DVM_LT_BII : value = CreateICmpSLT( dA, dB ); break;
			case DVM_LE_BBB :
			case DVM_LE_BII : value = CreateICmpSLE( dA, dB ); break;
			case DVM_EQ_BBB :
			case DVM_EQ_BII : value = CreateICmpEQ( dA, dB ); break;
			case DVM_NE_BBB :
			case DVM_NE_BII : value = CreateICmpNE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_B and m <= DVM_TEST_F) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						/* Do not store C, if it is no longer alive: */
						if( DaoCnode_FindResult( nodes[i+1], IntToPointer(vmc->c) ) < 0 ) break;
					}
				}
			}
			value = CreateIntCast( value, dao_integer_type, false );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITAND_III :
		case DVM_BITOR_III :
		case DVM_BITXOR_III :
		case DVM_BITLFT_III :
		case DVM_BITRIT_III :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_BITAND_III : value = CreateAnd( dA, dB ); break;
			case DVM_BITOR_III  : value = CreateOr( dA, dB ); break;
			case DVM_BITXOR_III : value = CreateXor( dA, dB ); break;
			case DVM_BITLFT_III : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_III : value = CreateLShr( dA, dB ); break;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_FFF :
		case DVM_SUB_FFF :
		case DVM_MUL_FFF :
		case DVM_DIV_FFF :
		case DVM_MOD_FFF :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_ADD_FFF : value = CreateFAdd( dA, dB ); break;
			case DVM_SUB_FFF : value = CreateFSub( dA, dB ); break;
			case DVM_MUL_FFF : value = CreateFMul( dA, dB ); break;
			case DVM_DIV_FFF : value = CreateFDiv( dA, dB ); break;
			case DVM_MOD_FFF : value = CreateFRem( dA, dB ); break;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_FFF :
			dA = dB = CreateFPCast( GetNumberOperand( vmc->a ), dao_float_type );
			if( vmc->a != vmc->b ) dB = CreateFPCast( GetNumberOperand( vmc->b ), dao_float_type );
			tmp = CreateCall2( daojit_pow_double, dA, dB );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_AND_BFF :
		case DVM_OR_BFF :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_AND_BFF : value = CreateAnd( dA, dB ); break;
			case DVM_OR_BFF  : value = CreateOr( dA, dB ); break;
			}
			value = CreateIntCast( value, dao_integer_type, false );
			StoreNumber( value, vmc->c );
			break;
		case DVM_LT_BFF :
		case DVM_LE_BFF :
		case DVM_EQ_BFF :
		case DVM_NE_BFF :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_LT_BFF : value = CreateFCmpOLT( dA, dB ); break;
			case DVM_LE_BFF : value = CreateFCmpOLE( dA, dB ); break;
			case DVM_EQ_BFF : value = CreateFCmpOEQ( dA, dB ); break;
			case DVM_NE_BFF : value = CreateFCmpONE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_B and m <= DVM_TEST_F) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						if( DaoCnode_FindResult( nodes[i+1], IntToPointer(vmc->c) ) < 0 ) break;
					}
				}
			}
			value = CreateIntCast( value, dao_integer_type, false );
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_CCC :
		case DVM_SUB_CCC :
		case DVM_MUL_CCC :
		case DVM_DIV_CCC :
			dA = dB = GetNumberOperand( vmc->a );
			real1 = real2 = CreateExtractValue( dA, fidx0 );
			imag1 = imag2 = CreateExtractValue( dA, fidx1 );
			if( vmc->a != vmc->b ){
				dB = GetNumberOperand( vmc->b );
				real2 = CreateExtractValue( dB, fidx0 );
				imag2 = CreateExtractValue( dB, fidx1 );
			}
			switch( code ){
			case DVM_ADD_CCC :
				dA = CreateFAdd( real1, real2 );
				dB = CreateFAdd( imag1, imag2 );
				break;
			case DVM_SUB_CCC :
				dA = CreateFSub( real1, real2 );
				dB = CreateFSub( imag1, imag2 );
				break;
			case DVM_MUL_CCC :
				dA = CreateFSub( CreateFMul( real1, real2 ), CreateFMul( imag1, imag2 ) );
				dB = CreateFAdd( CreateFMul( real1, imag2 ), CreateFMul( imag1, real2 ) );
				break;
			case DVM_DIV_CCC :
				dA = CreateFAdd( CreateFMul( real1, real2 ), CreateFMul( imag1, imag2 ) );
				dB = CreateFSub( CreateFMul( imag1, real2 ), CreateFMul( real1, imag2 ) );
				dC = CreateFAdd( CreateFMul( real2, real2 ), CreateFMul( imag2, imag2 ) );
				dA = CreateFDiv( dA, dC );
				dB = CreateFDiv( dB, dC );
				break;
			}
			dC = CreateBitCast( dao_complex_zero, dao_complex_type );
			dC = CreateInsertValue( dC, dA, fidx0 );
			dC = CreateInsertValue( dC, dB, fidx1 );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_EQ_BCC :
		case DVM_NE_BCC :
			dA = dB = GetNumberOperand( vmc->a );
			real1 = real2 = CreateExtractValue( dA, fidx0 );
			imag1 = imag2 = CreateExtractValue( dA, fidx1 );
			if( vmc->a != vmc->b ){
				dB = GetNumberOperand( vmc->b );
				real2 = CreateExtractValue( dB, fidx0 );
				imag2 = CreateExtractValue( dB, fidx1 );
			}
			switch( code ){
			case DVM_EQ_BCC :
				dC = CreateAnd( CreateFCmpOEQ( real1, real2 ), CreateFCmpOEQ( imag1, imag2 ) );
				break;
			case DVM_NE_BCC :
				dC = CreateOr( CreateFCmpONE( real1, real2 ), CreateFCmpONE( imag1, imag2 ) );
				break;
			}
			dC = CreateIntCast( dC, dao_integer_type, false );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_ADD_SSS :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			dC = GetLocalReference( vmc->c );
			CreateCall3( daojit_string_add, dA, dB, dC );
			break;
		case DVM_LT_BSS :
		case DVM_LE_BSS :
		case DVM_EQ_BSS :
		case DVM_NE_BSS :
			dA = dB = GetNumberOperand( vmc->a );
			if( vmc->a != vmc->b ) dB = GetNumberOperand( vmc->b );
			dC = GetLocalValue( vmc->c );
			switch( code ){
			case DVM_LT_BSS : CreateCall3( daojit_string_lt, dA, dB, dC ); break;
			case DVM_LE_BSS : CreateCall3( daojit_string_le, dA, dB, dC ); break;
			case DVM_EQ_BSS : CreateCall3( daojit_string_eq, dA, dB, dC ); break;
			case DVM_NE_BSS : CreateCall3( daojit_string_ne, dA, dB, dC ); break;
			}
			break;
		case DVM_GOTO :
			break;
		case DVM_TEST_B :
		case DVM_TEST_I :
			if( comparisons.find( i ) != comparisons.end() ) break;
			dA = GetNumberOperand( vmc->a );
			value = CreateICmpNE( dA, dao_integer_zero );
			comparisons[i] = value;
			break;
		case DVM_TEST_F :
			if( comparisons.find( i ) != comparisons.end() ) break;
			dA = GetNumberOperand( vmc->a );
			value = CreateFCmpONE( dA, dao_float_zero );
			comparisons[i] = value;
			break;
		case DVM_SWITCH :
			m = types[vmc->a]->tid; // integer or enum
			dA = NULL;
			if( m == DAO_ENUM ){
				dA = GetLocalValue( vmc->a );
				dA = CreatePointerCast( dA, daojit_enum_type_p );
				dA = CreateConstGEP2_32( dA, 0, 6 );
				dA = Dereference( dA );
			}else{
				dA = GetNumberOperand( vmc->a );
			}
			comparisons[i] = dA;
			break;
		case DVM_CASE :
			break;
		case DVM_SETI :
			if( CompilableSETI( vmc, types ) == false ) goto Failed;
			dC = GetLocalValue( vmc->c );
			if( types[vmc->a]->tid == DAO_ARRAY ){
				dA = GetLocalValue( vmc->a );
				CreateCall3( daojit_array_set_items_a, dC, dA, estatus );
				AddReturnCodeChecking( estatus, i );
				break;
			}
			dA = GetNumberOperand( vmc->a );
			switch( types[vmc->a]->tid ){
			case DAO_INTEGER : CreateCall3( daojit_array_set_items_i, dC, dA, estatus ); break;
			case DAO_FLOAT   : CreateCall3( daojit_array_set_items_f, dC, dA, estatus ); break;
			case DAO_COMPLEX : CreateCall3( daojit_array_set_items_c, dC, dA, estatus ); break;
			}
			AddReturnCodeChecking( estatus, i );
			break;
		case DVM_ADD :
		case DVM_SUB :
		case DVM_MUL :
		case DVM_DIV :
		case DVM_MOD :
		case DVM_POW :
			at = types[vmc->a];
			bt = types[vmc->b];
			ct = types[vmc->c];
			if( CompilableArrayBinOp( vmc, types ) == false ) goto Failed;
			dC = GetLocalValue( vmc->c );
			if( at->tid == DAO_ARRAY && bt->tid == DAO_ARRAY ){
				dA = GetLocalValue( vmc->a );
				dB = GetLocalValue( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_array_array, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_array_array, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_array_array, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_array_array, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_array_array, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_array_array, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_ARRAY && bt->tid == DAO_INTEGER ){
				dA = GetLocalValue( vmc->a );
				dB = GetNumberOperand( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_array_integer, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_array_integer, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_array_integer, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_array_integer, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_array_integer, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_array_integer, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_ARRAY && bt->tid == DAO_FLOAT ){
				dA = GetLocalValue( vmc->a );
				dB = GetNumberOperand( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_array_float, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_array_float, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_array_float, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_array_float, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_array_float, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_array_float, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_ARRAY && bt->tid == DAO_COMPLEX ){
				dA = GetLocalValue( vmc->a );
				dB = GetNumberOperand( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_array_complex, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_array_complex, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_array_complex, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_array_complex, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_array_complex, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_array_complex, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_INTEGER ){
				dA = GetNumberOperand( vmc->a );
				dB = GetLocalValue( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_integer_array, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_integer_array, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_integer_array, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_integer_array, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_integer_array, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_integer_array, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_FLOAT ){
				dA = GetNumberOperand( vmc->a );
				dB = GetLocalValue( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_float_array, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_float_array, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_float_array, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_float_array, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_float_array, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_float_array, dC, dA, dB, estatus ); break;
				}
			}else if( at->tid == DAO_COMPLEX ){
				dA = GetNumberOperand( vmc->a );
				dB = GetLocalValue( vmc->b );
				switch( code ){
				case DVM_ADD: CreateCall4( daojit_array_add_complex_array, dC, dA, dB, estatus ); break;
				case DVM_SUB: CreateCall4( daojit_array_sub_complex_array, dC, dA, dB, estatus ); break;
				case DVM_MUL: CreateCall4( daojit_array_mul_complex_array, dC, dA, dB, estatus ); break;
				case DVM_DIV: CreateCall4( daojit_array_div_complex_array, dC, dA, dB, estatus ); break;
				case DVM_MOD: CreateCall4( daojit_array_mod_complex_array, dC, dA, dB, estatus ); break;
				case DVM_POW: CreateCall4( daojit_array_pow_complex_array, dC, dA, dB, estatus ); break;
				}
			}
			AddReturnCodeChecking( estatus, i );
			break;
		case DVM_MOVE_SS :
			dA = GetLocalValue( vmc->a );
			dC = GetLocalReference( vmc->c );
			CreateCall2( daojit_string_move, dA, dC );
			break;
		case DVM_MOVE_PP : 
			value = GetLocalValue( vmc->a );
			refer = GetLocalReference( vmc->c );
			CreateCall2( daojit_move_pp, value, refer );
			break;
		case DVM_GETI_SI :
			dA = GetLocalValue( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dC = CreateCall3( daojit_geti_si, dA, dB, estatus );
			AddReturnCodeChecking( estatus, i );
			StoreNumber( dC, vmc->c );
			break;
		case DVM_SETI_SII :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dC = GetLocalValue( vmc->c );
			CreateCall4( daojit_seti_sii, dA, dB, dC, estatus );
			AddReturnCodeChecking( estatus, i );
			break;
		case DVM_GETI_LI : 
			value = GetListItem( vmc->a, vmc->b, i );
			refer = GetLocalReference( vmc->c );
			CreateCall2( daojit_move_pp, value, refer );
			break;
		case DVM_SETI_LI : 
			dA = GetLocalValue( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dC = GetLocalValue( vmc->c );
			CreateCall4( daojit_seti_li, dA, dB, dC, estatus );
			AddReturnCodeChecking( estatus, i );
			break;
		case DVM_GETI_LBI :
		case DVM_GETI_LII : case DVM_GETI_LFI :
		case DVM_GETI_LCI :
			value = GetListItem( vmc->a, vmc->b, i );
			value = GetValueNumberValue( value, daojit_number_types[code - DVM_GETI_LBI] );
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETI_LSI :
			dA = GetListItem( vmc->a, vmc->b, i );
			dC = GetLocalReference( vmc->c );
			tmp = CreateCall2( daojit_move_pp, dA, dC );
			break;
		case DVM_SETI_LBIB :
		case DVM_SETI_LIII :
		case DVM_SETI_LFIF :
		case DVM_SETI_LCIC :
			dC = GetListItem( vmc->c, vmc->b, i );
			dA = GetNumberOperand( vmc->a );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETI_LBIB] );
			tmp = CreateStore( dA, dC );
			break;
		case DVM_GETI_ABI :
		case DVM_GETI_AII :
		case DVM_GETI_AFI :
		case DVM_GETI_ACI :
			dA = GetArrayItem( vmc->a, vmc->b, i );
			dA = CreateLoad( dA );
			if( code == DVM_GETI_ABI ) dA = CreateIntCast( dA, dao_integer_type, false );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_SETI_ABIB :
		case DVM_SETI_AIII :
		case DVM_SETI_AFIF :
		case DVM_SETI_ACIC :
			dC = GetArrayItem( vmc->c, vmc->b, i );
			dA = GetNumberOperand( vmc->a );
			if( code == DVM_SETI_ABIB ) dA = CreateIntCast( dA, dao_boolean_type, false );
			CreateStore( dA, dC );
			break;
		case DVM_GETMI_ABI :
		case DVM_GETMI_AII :
		case DVM_GETMI_AFI :
		case DVM_GETMI_ACI :
			dA = GetArrayItemMI( vmc->a, vmc->b, i );
			dA = CreateLoad( dA );
			if( code == DVM_GETMI_ABI ) dA = CreateIntCast( dA, dao_integer_type, false );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_SETMI_ABIB :
		case DVM_SETMI_AIII :
		case DVM_SETMI_AFIF :
		case DVM_SETMI_ACIC :
			dC = GetArrayItemMI( vmc->c, vmc->b, i );
			dA = GetNumberOperand( vmc->a );
			if( code == DVM_SETMI_ABIB ) dA = CreateIntCast( dA, dao_boolean_type, false );
			CreateStore( dA, dC );
			break;
		case DVM_GETI_TI :
			value = GetTupleItems( vmc->a );
			dB = GetNumberOperand( vmc->b );
			value = GetValueItem( value, dB );
			refer = GetLocalReference( vmc->c );
			tmp = CreateCall2( daojit_move_pp, value, refer );
			break;
		case DVM_GETF_TX :
			value = GetTupleItems( vmc->a );
			value = CreateConstGEP2_32( value, 0, vmc->b );
			value = Dereference( value );
			refer = GetLocalReference( vmc->c );
			tmp = CreateCall2( daojit_move_pp, value, refer );
			break;
		case DVM_GETF_TB :
		case DVM_GETF_TI :
		case DVM_GETF_TF :
		case DVM_GETF_TC :
			dA = GetTupleItems( vmc->a );
			dA = CreateConstGEP2_32( dA, 0, vmc->b );
			dA = Dereference( dA );
			numtype = daojit_number_types[ code - DVM_GETF_TB ];
			dA = GetValueNumberValue( dA, numtype );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_SETI_TI : 
			dA = GetLocalValue( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dC = GetLocalValue( vmc->c );
			CreateCall4( daojit_seti_ti, dA, dB, dC, estatus );
			AddReturnCodeChecking( estatus, i );
			break;
		case DVM_SETF_TPP : 
			dA = GetLocalValue( vmc->a );
			dB = ConstantInt::get( int32_type, vmc->b );
			dC = GetLocalValue( vmc->c );
			CreateCall4( daojit_setf_tpp, dA, dB, dC, estatus );
			break;
		case DVM_SETF_TBB :
		case DVM_SETF_TII :
		case DVM_SETF_TFF :
		case DVM_SETF_TCC :
			dC = GetTupleItems( vmc->c );
			dC = CreateConstGEP2_32( dC, 0, vmc->b );
			dC = Dereference( dC );
			dA = GetNumberOperand( vmc->a );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETF_TBB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_TSS :
			dA = GetLocalValue( vmc->a );
			dC = GetTupleItems( vmc->c );
			dC = CreateConstGEP2_32( dC, 0, vmc->b );
			CreateCall2( daojit_string_move, dA, dC );
			break;
		case DVM_GETF_KCB :
		case DVM_GETF_KCI :
		case DVM_GETF_KCF :
		case DVM_GETF_KCC :
			dA = GetClassConstant( vmc->a, vmc->b );
			dA = GetValueNumberValue( dA, daojit_number_types[ code - DVM_GETF_KCB ] );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_GETF_KGB :
		case DVM_GETF_KGI :
		case DVM_GETF_KGF :
		case DVM_GETF_KGC :
			dA = GetClassStatic( vmc->a, vmc->b );
			dA = GetValueNumberValue( dA, daojit_number_types[ code - DVM_GETF_KGB ] );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_GETF_OCB :
		case DVM_GETF_OCI :
		case DVM_GETF_OCF :
		case DVM_GETF_OCC :
			dA = GetObjectConstant( vmc->a, vmc->b );
			dA = GetValueNumberValue( dA, daojit_number_types[ code - DVM_GETF_OCB ] );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_GETF_OGB :
		case DVM_GETF_OGI :
		case DVM_GETF_OGF :
		case DVM_GETF_OGC :
			dA = GetObjectStatic( vmc->a, vmc->b );
			dA = GetValueNumberValue( dA, daojit_number_types[ code - DVM_GETF_OGB ] );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_GETF_OVB :
		case DVM_GETF_OVI :
		case DVM_GETF_OVF :
		case DVM_GETF_OVC :
			dA = GetObjectVariable( vmc->a, vmc->b );
			dA = GetValueNumberValue( dA, daojit_number_types[ code - DVM_GETF_OVB ] );
			StoreNumber( dA, vmc->c );
			break;
		case DVM_SETF_KGBB :
		case DVM_SETF_KGII :
		case DVM_SETF_KGFF :
		case DVM_SETF_KGCC :
			dA = GetNumberOperand( vmc->a );
			dC = GetClassStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETF_KGBB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OGBB :
		case DVM_SETF_OGII :
		case DVM_SETF_OGFF :
		case DVM_SETF_OGCC :
			dA = GetNumberOperand( vmc->a );
			dC = GetObjectStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETF_OGBB] );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OVBB :
		case DVM_SETF_OVII :
		case DVM_SETF_OVFF :
		case DVM_SETF_OVCC :
			dA = GetNumberOperand( vmc->a );
			dC = GetObjectVariable( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, daojit_number_types[code - DVM_SETF_OVBB] );
			CreateStore( dA, dC );
			break;
		default : goto Failed;
		}
		if( branchings.find( i ) != branchings.end() ) branchings[i] = GetInsertBlock();
		//if( code == DVM_SWITCH ) i += vmc->c + 1; // skip cases and one goto.
	}
	if( labels.find( start ) != labels.end() ){
		SetInsertPoint( entryBlock );
		CreateBr( secondBlock );
	}
	for(iter=branchings.begin(), stop=branchings.end(); iter!=stop; iter++){
		vmc = vmcs[ iter->first ];
		code = vmc->code;
		k = iter->first + 1;
		switch( code ){
		case DVM_GOTO : case DVM_TEST_B : case DVM_TEST_I : case DVM_TEST_F :
		case DVM_SWITCH :
			k = vmc->b;
			break;
		}
		if( k > end ){
			labels[end+1] = NewBlock( vmcs[end+1] );
			break;
		}
	}
#if 0
	for(iter=labels.begin(), stop=labels.end(); iter!=stop; iter++){
		printf( "%3i %9p\n", iter->first, iter->second );
	}
#endif
	for(iter=branchings.begin(), stop=branchings.end(); iter!=stop; iter++){
		currentNode = nodes[ iter->first ];
		vmc = vmcs[ iter->first ];
		code = vmc->code;
		if( labels[ vmc->b ] == NULL ) labels[vmc->b] = lastBlock;
		//printf( "%3i %3i %p\n", iter->first, vmc->b, labels[vmc->b] );
		//printf( "%3i %9p: ", iter->first, iter->second ); DaoVmCodeX_Print( *vmc, NULL );
		SetInsertPoint( iter->second );
		switch( code ){
		case DVM_GOTO :
			CreateBr( labels[ vmc->b ] );
			break;
		case DVM_TEST_B :
		case DVM_TEST_I :
			value = comparisons[iter->first];
			CreateCondBr( value, labels[ iter->first + 1 ], labels[ vmc->b ] );
			break;
		case DVM_TEST_F :
			value = comparisons[iter->first];
			CreateCondBr( value, labels[ iter->first + 1 ], labels[ vmc->b ] );
			break;
		case DVM_SWITCH :
			m = types[vmc->a]->tid;
			value = comparisons[iter->first];
			inswitch = CreateSwitch( value, labels[ vmc->b ], vmc->c );
			// use DVM_CASE to add switch labels
			for(k=1; k<=vmc->c; k++){
				DaoVmCodeX *vmc2 = vmcs[ iter->first + k ];
				daoint ic = routConsts[vmc2->a]->xInteger.value;
				if( m == DAO_ENUM ) ic = routConsts[vmc2->a]->xEnum.value;
				caseint = cast<ConstantInt>( ConstantInt::get( daoint_type, ic ) );
				inswitch->addCase( caseint, labels[vmc2->b] );
			}
			break;
		default :
			if( iter->first < end ){
				CreateBr( labels[ iter->first + 1 ] );
			}else{
				CreateBr( labels[ end + 1 ] );
			}
			break;
		}
	}
	SetInsertPoint( lastBlock );
	node = optimizer->nodes->items.pCnode[end];
	for(i=0; i<routine->body->regCount; i++){
		DaoType *type = types[i];
		if( stackValues[i] == NULL ) continue;
		if( DaoCnode_FindResult( node, IntToPointer(i) ) < 0 ) continue; // skip dead variables;
		Value *A = CreateLoad( stackValues[i] );
		Value *C = GetLocalValue( i );
		C = GetValueNumberPointer( C, daojit_number_types[type->tid - DAO_BOOLEAN] );
		CreateStore( A, C );
	}
	CreateRet( zero32 );
	SetInsertPoint( entryBlock );
	if( entryBlock->back().isTerminator() == false ) CreateBr( secondBlock );
	if( secondBlock->size() ==0 or secondBlock->back().isTerminator() == false ){
		Function::BasicBlockListType & blocks = jitfunc->getBasicBlockList();
		SetInsertPoint( secondBlock );
		if( blocks.size() > 2 )
			CreateBr( ++(++blocks.begin()) );
		else
			CreateRet( zero32 );
	}
	return jitfunc;
Failed:
	printf( "failed compiling: %s %4i %4i\n", routine->routName->chars, start, end );
	jitfunc->eraseFromParent();
	return NULL;
}
void DaoJIT_Compile( DaoRoutine *routine, DaoOptimizer *optimizer )
{
	DaoJitHandle handle( *llvm_context, routine, optimizer );
	std::vector<DaoJitFunctionData> *jitFunctions;
	std::vector<IndexRange> segments;

	DaoJIT_SearchCompilable( routine, segments );
	if( segments.size() == 0 ) return;
	DaoOptimizer_LinkDU( optimizer, routine );
	DaoOptimizer_DoLVA( optimizer, routine );

	jitFunctions = new std::vector<DaoJitFunctionData>();
	for(int i=0, n=segments.size(); i<n; i++){
#ifndef DEBUG
		if( (segments[i].end - segments[i].start) < 10 ) continue;
#endif
		Function *jitfunc = handle.Compile( segments[i].start, segments[i].end );
		if( jitfunc == NULL ) continue;
		//llvm_func_optimizer->run( *jitfunc );

		DaoVmCode *vmc = routine->body->vmCodes->data.codes + segments[i].start;
		vmc->code = DVM_JITC;
		vmc->a = jitFunctions->size();
		vmc->b = segments[i].end - segments[i].start + 1;

		void *fptr = llvm_exe_engine->getPointerToFunction( jitfunc );
		DaoJitFunction jitfunc2 = (DaoJitFunction) fptr;
		jitFunctions->push_back( DaoJitFunctionData( jitfunc, jitfunc2 ) );
	}
	if( jitFunctions->size() == 0 ){
		delete jitFunctions;
		jitFunctions = NULL;
	}
	routine->body->jitData = jitFunctions;

#ifdef DEBUG
	PassManager PM;
	PM.add(createPrintModulePass(outs()));
	PM.run(*llvm_module);
	verifyModule(*llvm_module, &outs());
#endif
}

const char* const dao_exception_names[] =
{
	"Exception" ,
	"Warning" ,
	"Error" ,
	"Error::Field" ,
	"Error::Field::NotExist" ,
	"Error::Field::NotPermit" ,
	"Error::Float" ,
	"Error::Float::DivByZero" ,
	"Error::Float::OverFlow" ,
	"Error::Float::UnderFlow" ,
	"Error::Index" ,
	"Error::Index::Range" ,
	"Error::Key" ,
	"Error::Key::NotExist" ,
	"Error::Param" ,
	"Error::Syntax" ,
	"Error::Type" ,
	"Error::Value" ,
	"Error::File"
};

void DaoJIT_Execute( DaoProcess *process, DaoJitCallData *data, int jitcode )
{
	DaoRoutine *routine = process->activeRoutine;
	std::vector<DaoJitFunctionData> & jitFuncs = *(std::vector<DaoJitFunctionData>*) routine->body->jitData;

	int rc = (*jitFuncs[jitcode].funcPointer)( data );
	if( rc ){
		int vmc = rc & 0xffff;
		int ec = rc >> 16;
		process->activeCode = process->topFrame->codes + vmc;
		DaoProcess_RaiseException( process, dao_exception_names[ec], "", NULL );
	}
}

extern "C"{
DAO_DLL int DaoJIT_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoStream *stream = vmSpace->errorStream;
	if( sizeof(dao_integer) != sizeof(long long) ){
		DaoStream_WriteChars( stream, "DaoJIT supports 64 bit int type only!" );
		return 1;
	}
	if( sizeof(dao_float) != sizeof(double) ){
		DaoStream_WriteChars( stream, "DaoJIT supports 64 bit float type only!" );
		return 1;
	}
	DaoJIT_Init( vmSpace, & dao_jit );
	dao_jit.Quit = DaoJIT_Quit;
	dao_jit.Free = DaoJIT_Free;
	dao_jit.Compile = DaoJIT_Compile;
	dao_jit.Execute = DaoJIT_Execute;
	return 0;
}
}
