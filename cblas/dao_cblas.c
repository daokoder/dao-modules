#include"dao_cblas.h"

DaoVmSpace *__daoVmSpace = NULL;
#ifdef __cplusplus
extern "C"{
#endif
static void dao__cblas_sdsdot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsdot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sdot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ddot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sasum( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dasum( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_scasum( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dzasum( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_snrm2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dnrm2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_scnrm2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dznrm2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_isamax( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_idamax( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_icamax( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_izamax( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_saxpy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_daxpy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_caxpy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zaxpy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_scopy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dcopy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ccopy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zcopy( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sswap( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dswap( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cswap( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zswap( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_srot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_drot( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_srotg( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_drotg( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_srotm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_drotm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_srotmg( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_drotmg( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_csscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zdscal( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sgemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dgemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cgemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zgemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sger( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dger( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cgeru( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cgerc( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zgeru( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zgerc( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_strsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtrsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctrsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztrsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_strmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtrmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctrmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztrmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssyr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsyr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cher( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zher( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssyr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsyr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cher2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zher2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sgbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dgbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cgbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zgbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_stbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_stbsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtbsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctbsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztbsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_stpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_stpsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtpsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctpsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztpsv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssymv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsymv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhemv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sspmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dspmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sspr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dspr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chpr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhpr( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sspr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dspr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chpr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhpr2( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhbmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhpmv( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_sgemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dgemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cgemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zgemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssymm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsymm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_csymm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zsymm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssyrk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsyrk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_csyrk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zsyrk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ssyr2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dsyr2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_csyr2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zsyr2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_strmm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtrmm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctrmm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztrmm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_strsm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_dtrsm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ctrsm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_ztrsm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_chemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zhemm( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cherk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zherk( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_cher2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static void dao__cblas_zher2k( DaoProcess *_proc, DaoValue *_p[], int _n );
static DaoFuncItem dao__Funcs[] = 
{
  { dao__cblas_sdsdot, "cblas_sdsdot( n: int, alpha: float, x: array<float>, incx: int, y: array<float>, incy: int )=>float" },
  { dao__cblas_dsdot, "cblas_dsdot( n: int, x: array<float>, incx: int, y: array<float>, incy: int )=>double" },
  { dao__cblas_sdot, "cblas_sdot( n: int, x: array<float>, incx: int, y: array<float>, incy: int )=>float" },
  { dao__cblas_ddot, "cblas_ddot( n: int, x: array<double>, incx: int, y: array<double>, incy: int )=>double" },
  { dao__cblas_sasum, "cblas_sasum( n: int, x: array<float>, incx: int )=>float" },
  { dao__cblas_dasum, "cblas_dasum( n: int, x: array<double>, incx: int )=>double" },
  { dao__cblas_scasum, "cblas_scasum( n: int, x: array<float>, incx: int )=>float" },
  { dao__cblas_dzasum, "cblas_dzasum( n: int, x: array<double>, incx: int )=>double" },
  { dao__cblas_snrm2, "cblas_snrm2( N: int, X: array<float>, incX: int )=>float" },
  { dao__cblas_dnrm2, "cblas_dnrm2( N: int, X: array<double>, incX: int )=>double" },
  { dao__cblas_scnrm2, "cblas_scnrm2( N: int, X: array<float>, incX: int )=>float" },
  { dao__cblas_dznrm2, "cblas_dznrm2( N: int, X: array<double>, incX: int )=>double" },
  { dao__cblas_isamax, "cblas_isamax( n: int, x: array<float>, incx: int )=>int" },
  { dao__cblas_idamax, "cblas_idamax( n: int, x: array<double>, incx: int )=>int" },
  { dao__cblas_icamax, "cblas_icamax( n: int, x: array<float>, incx: int )=>int" },
  { dao__cblas_izamax, "cblas_izamax( n: int, x: array<double>, incx: int )=>int" },
  { dao__cblas_saxpy, "cblas_saxpy( n: int, alpha: float, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_daxpy, "cblas_daxpy( n: int, alpha: double, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_caxpy, "cblas_caxpy( n: int, alpha: array<float>, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_zaxpy, "cblas_zaxpy( n: int, alpha: array<double>, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_scopy, "cblas_scopy( n: int, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_dcopy, "cblas_dcopy( n: int, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_ccopy, "cblas_ccopy( n: int, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_zcopy, "cblas_zcopy( n: int, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_sswap, "cblas_sswap( n: int, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_dswap, "cblas_dswap( n: int, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_cswap, "cblas_cswap( n: int, x: array<float>, incx: int, y: array<float>, incy: int )" },
  { dao__cblas_zswap, "cblas_zswap( n: int, x: array<double>, incx: int, y: array<double>, incy: int )" },
  { dao__cblas_srot, "cblas_srot( N: int, X: array<float>, incX: int, Y: array<float>, incY: int, c: float, s: float )" },
  { dao__cblas_drot, "cblas_drot( N: int, X: array<double>, incX: int, Y: array<double>, incY: int, c: double, s: double )" },
  { dao__cblas_srotg, "cblas_srotg( a: float, b: array<float>, c: array<float>, s: array<float> )=>float" },
  { dao__cblas_drotg, "cblas_drotg( a: double, b: array<double>, c: array<double>, s: array<double> )=>double" },
  { dao__cblas_srotm, "cblas_srotm( N: int, X: array<float>, incX: int, Y: array<float>, incY: int, P: array<float> )" },
  { dao__cblas_drotm, "cblas_drotm( N: int, X: array<double>, incX: int, Y: array<double>, incY: int, P: array<double> )" },
  { dao__cblas_srotmg, "cblas_srotmg( d1: float, d2: array<float>, b1: array<float>, b2: float, P: array<float> )=>float" },
  { dao__cblas_drotmg, "cblas_drotmg( d1: double, d2: array<double>, b1: array<double>, b2: double, P: array<double> )=>double" },
  { dao__cblas_sscal, "cblas_sscal( N: int, alpha: float, X: array<float>, incX: int )" },
  { dao__cblas_dscal, "cblas_dscal( N: int, alpha: double, X: array<double>, incX: int )" },
  { dao__cblas_cscal, "cblas_cscal( N: int, alpha: array<float>, X: array<float>, incX: int )" },
  { dao__cblas_zscal, "cblas_zscal( N: int, alpha: array<double>, X: array<double>, incX: int )" },
  { dao__cblas_csscal, "cblas_csscal( N: int, alpha: float, X: array<float>, incX: int )" },
  { dao__cblas_zdscal, "cblas_zdscal( N: int, alpha: double, X: array<double>, incX: int )" },
  { dao__cblas_sgemv, "cblas_sgemv( order: int, trans: int, m: int, n: int, alpha: float, a: array<float>, lda: int, x: array<float>, incx: int, beta: float, y: array<float>, incy: int )" },
  { dao__cblas_dgemv, "cblas_dgemv( order: int, trans: int, m: int, n: int, alpha: double, a: array<double>, lda: int, x: array<double>, incx: int, beta: double, y: array<double>, incy: int )" },
  { dao__cblas_cgemv, "cblas_cgemv( order: int, trans: int, m: int, n: int, alpha: array<float>, a: array<float>, lda: int, x: array<float>, incx: int, beta: array<float>, y: array<float>, incy: int )" },
  { dao__cblas_zgemv, "cblas_zgemv( order: int, trans: int, m: int, n: int, alpha: array<double>, a: array<double>, lda: int, x: array<double>, incx: int, beta: array<double>, y: array<double>, incy: int )" },
  { dao__cblas_sger, "cblas_sger( order: int, M: int, N: int, alpha: float, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float>, lda: int )" },
  { dao__cblas_dger, "cblas_dger( order: int, M: int, N: int, alpha: double, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double>, lda: int )" },
  { dao__cblas_cgeru, "cblas_cgeru( order: int, M: int, N: int, alpha: array<float>, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float>, lda: int )" },
  { dao__cblas_cgerc, "cblas_cgerc( order: int, M: int, N: int, alpha: array<float>, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float>, lda: int )" },
  { dao__cblas_zgeru, "cblas_zgeru( order: int, M: int, N: int, alpha: array<double>, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double>, lda: int )" },
  { dao__cblas_zgerc, "cblas_zgerc( order: int, M: int, N: int, alpha: array<double>, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double>, lda: int )" },
  { dao__cblas_strsv, "cblas_strsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_dtrsv, "cblas_dtrsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_ctrsv, "cblas_ctrsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_ztrsv, "cblas_ztrsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_strmv, "cblas_strmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_dtrmv, "cblas_dtrmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_ctrmv, "cblas_ctrmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_ztrmv, "cblas_ztrmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_ssyr, "cblas_ssyr( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, A: array<float>, lda: int )" },
  { dao__cblas_dsyr, "cblas_dsyr( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, A: array<double>, lda: int )" },
  { dao__cblas_cher, "cblas_cher( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, A: array<float>, lda: int )" },
  { dao__cblas_zher, "cblas_zher( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, A: array<double>, lda: int )" },
  { dao__cblas_ssyr2, "cblas_ssyr2( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float>, lda: int )" },
  { dao__cblas_dsyr2, "cblas_dsyr2( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double>, lda: int )" },
  { dao__cblas_cher2, "cblas_cher2( order: int, Uplo: int, N: int, alpha: array<float>, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float>, lda: int )" },
  { dao__cblas_zher2, "cblas_zher2( order: int, Uplo: int, N: int, alpha: array<double>, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double>, lda: int )" },
  { dao__cblas_sgbmv, "cblas_sgbmv( order: int, TransA: int, M: int, N: int, KL: int, KU: int, alpha: float, A: array<float>, lda: int, X: array<float>, incX: int, beta: float, Y: array<float>, incY: int )" },
  { dao__cblas_dgbmv, "cblas_dgbmv( order: int, TransA: int, M: int, N: int, KL: int, KU: int, alpha: double, A: array<double>, lda: int, X: array<double>, incX: int, beta: double, Y: array<double>, incY: int )" },
  { dao__cblas_cgbmv, "cblas_cgbmv( order: int, TransA: int, M: int, N: int, KL: int, KU: int, alpha: array<float>, A: array<float>, lda: int, X: array<float>, incX: int, beta: array<float>, Y: array<float>, incY: int )" },
  { dao__cblas_zgbmv, "cblas_zgbmv( order: int, TransA: int, M: int, N: int, KL: int, KU: int, alpha: array<double>, A: array<double>, lda: int, X: array<double>, incX: int, beta: array<double>, Y: array<double>, incY: int )" },
  { dao__cblas_ssbmv, "cblas_ssbmv( order: int, Uplo: int, N: int, K: int, alpha: float, A: array<float>, lda: int, X: array<float>, incX: int, beta: float, Y: array<float>, incY: int )" },
  { dao__cblas_dsbmv, "cblas_dsbmv( order: int, Uplo: int, N: int, K: int, alpha: double, A: array<double>, lda: int, X: array<double>, incX: int, beta: double, Y: array<double>, incY: int )" },
  { dao__cblas_stbmv, "cblas_stbmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_dtbmv, "cblas_dtbmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_ctbmv, "cblas_ctbmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_ztbmv, "cblas_ztbmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_stbsv, "cblas_stbsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_dtbsv, "cblas_dtbsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_ctbsv, "cblas_ctbsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<float>, lda: int, X: array<float>, incX: int )" },
  { dao__cblas_ztbsv, "cblas_ztbsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, K: int, A: array<double>, lda: int, X: array<double>, incX: int )" },
  { dao__cblas_stpmv, "cblas_stpmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<float>, X: array<float>, incX: int )" },
  { dao__cblas_dtpmv, "cblas_dtpmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<double>, X: array<double>, incX: int )" },
  { dao__cblas_ctpmv, "cblas_ctpmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<float>, X: array<float>, incX: int )" },
  { dao__cblas_ztpmv, "cblas_ztpmv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<double>, X: array<double>, incX: int )" },
  { dao__cblas_stpsv, "cblas_stpsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<float>, X: array<float>, incX: int )" },
  { dao__cblas_dtpsv, "cblas_dtpsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<double>, X: array<double>, incX: int )" },
  { dao__cblas_ctpsv, "cblas_ctpsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<float>, X: array<float>, incX: int )" },
  { dao__cblas_ztpsv, "cblas_ztpsv( order: int, Uplo: int, TransA: int, Diag: int, N: int, Ap: array<double>, X: array<double>, incX: int )" },
  { dao__cblas_ssymv, "cblas_ssymv( order: int, Uplo: int, N: int, alpha: float, A: array<float>, lda: int, X: array<float>, incX: int, beta: float, Y: array<float>, incY: int )" },
  { dao__cblas_dsymv, "cblas_dsymv( order: int, Uplo: int, N: int, alpha: double, A: array<double>, lda: int, X: array<double>, incX: int, beta: double, Y: array<double>, incY: int )" },
  { dao__cblas_chemv, "cblas_chemv( order: int, Uplo: int, N: int, alpha: array<float>, A: array<float>, lda: int, X: array<float>, incX: int, beta: array<float>, Y: array<float>, incY: int )" },
  { dao__cblas_zhemv, "cblas_zhemv( order: int, Uplo: int, N: int, alpha: array<double>, A: array<double>, lda: int, X: array<double>, incX: int, beta: array<double>, Y: array<double>, incY: int )" },
  { dao__cblas_sspmv, "cblas_sspmv( order: int, Uplo: int, N: int, alpha: float, Ap: array<float>, X: array<float>, incX: int, beta: float, Y: array<float>, incY: int )" },
  { dao__cblas_dspmv, "cblas_dspmv( order: int, Uplo: int, N: int, alpha: double, Ap: array<double>, X: array<double>, incX: int, beta: double, Y: array<double>, incY: int )" },
  { dao__cblas_sspr, "cblas_sspr( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, Ap: array<float> )" },
  { dao__cblas_dspr, "cblas_dspr( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, Ap: array<double> )" },
  { dao__cblas_chpr, "cblas_chpr( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, A: array<float> )" },
  { dao__cblas_zhpr, "cblas_zhpr( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, A: array<double> )" },
  { dao__cblas_sspr2, "cblas_sspr2( order: int, Uplo: int, N: int, alpha: float, X: array<float>, incX: int, Y: array<float>, incY: int, A: array<float> )" },
  { dao__cblas_dspr2, "cblas_dspr2( order: int, Uplo: int, N: int, alpha: double, X: array<double>, incX: int, Y: array<double>, incY: int, A: array<double> )" },
  { dao__cblas_chpr2, "cblas_chpr2( order: int, Uplo: int, N: int, alpha: array<float>, X: array<float>, incX: int, Y: array<float>, incY: int, Ap: array<float> )" },
  { dao__cblas_zhpr2, "cblas_zhpr2( order: int, Uplo: int, N: int, alpha: array<double>, X: array<double>, incX: int, Y: array<double>, incY: int, Ap: array<double> )" },
  { dao__cblas_chbmv, "cblas_chbmv( order: int, Uplo: int, N: int, K: int, alpha: array<float>, A: array<float>, lda: int, X: array<float>, incX: int, beta: array<float>, Y: array<float>, incY: int )" },
  { dao__cblas_zhbmv, "cblas_zhbmv( order: int, Uplo: int, N: int, K: int, alpha: array<double>, A: array<double>, lda: int, X: array<double>, incX: int, beta: array<double>, Y: array<double>, incY: int )" },
  { dao__cblas_chpmv, "cblas_chpmv( order: int, Uplo: int, N: int, alpha: array<float>, Ap: array<float>, X: array<float>, incX: int, beta: array<float>, Y: array<float>, incY: int )" },
  { dao__cblas_zhpmv, "cblas_zhpmv( order: int, Uplo: int, N: int, alpha: array<double>, Ap: array<double>, X: array<double>, incX: int, beta: array<double>, Y: array<double>, incY: int )" },
  { dao__cblas_sgemm, "cblas_sgemm( Order: int, TransA: int, TransB: int, M: int, N: int, K: int, alpha: float, A: array<float>, lda: int, B: array<float>, ldb: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_dgemm, "cblas_dgemm( Order: int, TransA: int, TransB: int, M: int, N: int, K: int, alpha: double, A: array<double>, lda: int, B: array<double>, ldb: int, beta: double, C: array<double>, ldc: int )" },
  { dao__cblas_cgemm, "cblas_cgemm( Order: int, TransA: int, TransB: int, M: int, N: int, K: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int, beta: array<float>, C: array<float>, ldc: int )" },
  { dao__cblas_zgemm, "cblas_zgemm( Order: int, TransA: int, TransB: int, M: int, N: int, K: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int, beta: array<double>, C: array<double>, ldc: int )" },
  { dao__cblas_ssymm, "cblas_ssymm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: float, A: array<float>, lda: int, B: array<float>, ldb: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_dsymm, "cblas_dsymm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: double, A: array<double>, lda: int, B: array<double>, ldb: int, beta: double, C: array<double>, ldc: int )" },
  { dao__cblas_csymm, "cblas_csymm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int, beta: array<float>, C: array<float>, ldc: int )" },
  { dao__cblas_zsymm, "cblas_zsymm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int, beta: array<double>, C: array<double>, ldc: int )" },
  { dao__cblas_ssyrk, "cblas_ssyrk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: float, A: array<float>, lda: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_dsyrk, "cblas_dsyrk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: double, A: array<double>, lda: int, beta: double, C: array<double>, ldc: int )" },
  { dao__cblas_csyrk, "cblas_csyrk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<float>, A: array<float>, lda: int, beta: array<float>, C: array<float>, ldc: int )" },
  { dao__cblas_zsyrk, "cblas_zsyrk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<double>, A: array<double>, lda: int, beta: array<double>, C: array<double>, ldc: int )" },
  { dao__cblas_ssyr2k, "cblas_ssyr2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: float, A: array<float>, lda: int, B: array<float>, ldb: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_dsyr2k, "cblas_dsyr2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: double, A: array<double>, lda: int, B: array<double>, ldb: int, beta: double, C: array<double>, ldc: int )" },
  { dao__cblas_csyr2k, "cblas_csyr2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int, beta: array<float>, C: array<float>, ldc: int )" },
  { dao__cblas_zsyr2k, "cblas_zsyr2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int, beta: array<double>, C: array<double>, ldc: int )" },
  { dao__cblas_strmm, "cblas_strmm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: float, A: array<float>, lda: int, B: array<float>, ldb: int )" },
  { dao__cblas_dtrmm, "cblas_dtrmm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: double, A: array<double>, lda: int, B: array<double>, ldb: int )" },
  { dao__cblas_ctrmm, "cblas_ctrmm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int )" },
  { dao__cblas_ztrmm, "cblas_ztrmm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int )" },
  { dao__cblas_strsm, "cblas_strsm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: float, A: array<float>, lda: int, B: array<float>, ldb: int )" },
  { dao__cblas_dtrsm, "cblas_dtrsm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: double, A: array<double>, lda: int, B: array<double>, ldb: int )" },
  { dao__cblas_ctrsm, "cblas_ctrsm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int )" },
  { dao__cblas_ztrsm, "cblas_ztrsm( Order: int, Side: int, Uplo: int, TransA: int, Diag: int, M: int, N: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int )" },
  { dao__cblas_chemm, "cblas_chemm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int, beta: array<float>, C: array<float>, ldc: int )" },
  { dao__cblas_zhemm, "cblas_zhemm( Order: int, Side: int, Uplo: int, M: int, N: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int, beta: array<double>, C: array<double>, ldc: int )" },
  { dao__cblas_cherk, "cblas_cherk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: float, A: array<float>, lda: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_zherk, "cblas_zherk( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: double, A: array<double>, lda: int, beta: double, C: array<double>, ldc: int )" },
  { dao__cblas_cher2k, "cblas_cher2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<float>, A: array<float>, lda: int, B: array<float>, ldb: int, beta: float, C: array<float>, ldc: int )" },
  { dao__cblas_zher2k, "cblas_zher2k( Order: int, Uplo: int, Trans: int, N: int, K: int, alpha: array<double>, A: array<double>, lda: int, B: array<double>, ldb: int, beta: double, C: array<double>, ldc: int )" },
  { NULL, NULL }
};
/* ./cblas.h */
static void dao__cblas_sdsdot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[5] );
  const float* y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[2] );

  float _cblas_sdsdot = cblas_sdsdot( n, alpha, x, incx, y, incy );
  DaoProcess_PutFloat( _proc, (float) _cblas_sdsdot );
}
/* ./cblas.h */
static void dao__cblas_dsdot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const float* y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  double _cblas_dsdot = cblas_dsdot( n, x, incx, y, incy );
  DaoProcess_PutDouble( _proc, (double) _cblas_dsdot );
}
/* ./cblas.h */
static void dao__cblas_sdot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const float* y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  float _cblas_sdot = cblas_sdot( n, x, incx, y, incy );
  DaoProcess_PutFloat( _proc, (float) _cblas_sdot );
}
/* ./cblas.h */
static void dao__cblas_ddot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const double* y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  double _cblas_ddot = cblas_ddot( n, x, incx, y, incy );
  DaoProcess_PutDouble( _proc, (double) _cblas_ddot );
}
/* ./cblas.h */
static void dao__cblas_sasum( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  float _cblas_sasum = cblas_sasum( n, x, incx );
  DaoProcess_PutFloat( _proc, (float) _cblas_sasum );
}
/* ./cblas.h */
static void dao__cblas_dasum( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  double _cblas_dasum = cblas_dasum( n, x, incx );
  DaoProcess_PutDouble( _proc, (double) _cblas_dasum );
}
/* ./cblas.h */
static void dao__cblas_scasum( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  float _cblas_scasum = cblas_scasum( n, x, incx );
  DaoProcess_PutFloat( _proc, (float) _cblas_scasum );
}
/* ./cblas.h */
static void dao__cblas_dzasum( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  double _cblas_dzasum = cblas_dzasum( n, x, incx );
  DaoProcess_PutDouble( _proc, (double) _cblas_dzasum );
}
/* ./cblas.h */
static void dao__cblas_snrm2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  float _cblas_snrm2 = cblas_snrm2( N, X, incX );
  DaoProcess_PutFloat( _proc, (float) _cblas_snrm2 );
}
/* ./cblas.h */
static void dao__cblas_dnrm2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  double _cblas_dnrm2 = cblas_dnrm2( N, X, incX );
  DaoProcess_PutDouble( _proc, (double) _cblas_dnrm2 );
}
/* ./cblas.h */
static void dao__cblas_scnrm2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  float _cblas_scnrm2 = cblas_scnrm2( N, X, incX );
  DaoProcess_PutFloat( _proc, (float) _cblas_scnrm2 );
}
/* ./cblas.h */
static void dao__cblas_dznrm2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  double _cblas_dznrm2 = cblas_dznrm2( N, X, incX );
  DaoProcess_PutDouble( _proc, (double) _cblas_dznrm2 );
}
/* ./cblas.h */
static void dao__cblas_isamax( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  size_t _cblas_isamax = cblas_isamax( n, x, incx );
  DaoProcess_PutInteger( _proc, (daoint) _cblas_isamax );
}
/* ./cblas.h */
static void dao__cblas_idamax( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  size_t _cblas_idamax = cblas_idamax( n, x, incx );
  DaoProcess_PutInteger( _proc, (daoint) _cblas_idamax );
}
/* ./cblas.h */
static void dao__cblas_icamax( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  size_t _cblas_icamax = cblas_icamax( n, x, incx );
  DaoProcess_PutInteger( _proc, (daoint) _cblas_icamax );
}
/* ./cblas.h */
static void dao__cblas_izamax( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  size_t _cblas_izamax = cblas_izamax( n, x, incx );
  DaoProcess_PutInteger( _proc, (daoint) _cblas_izamax );
}
/* ./cblas.h */
static void dao__cblas_saxpy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[3] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[5] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[2] );

  cblas_saxpy( n, alpha, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[4] );
}
/* ./cblas.h */
static void dao__cblas_daxpy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[3] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[5] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[2] );

  cblas_daxpy( n, alpha, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[4] );
}
/* ./cblas.h */
static void dao__cblas_caxpy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[3] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[5] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  cblas_caxpy( n, alpha, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[4] );
}
/* ./cblas.h */
static void dao__cblas_zaxpy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[3] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[5] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  cblas_zaxpy( n, alpha, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[4] );
}
/* ./cblas.h */
static void dao__cblas_scopy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  cblas_scopy( n, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_dcopy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  cblas_dcopy( n, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_ccopy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  cblas_ccopy( n, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_zcopy( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  cblas_zcopy( n, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_sswap( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  float* x = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );

  cblas_sswap( n, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_dswap( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  double* x = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );

  cblas_dswap( n, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_cswap( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  float* x = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );

  cblas_cswap( n, x, incx, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_zswap( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int n = (const int) DaoValue_TryGetInteger( _p[0] );
  double* x = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[2] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[4] );

  cblas_zswap( n, x, incx, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_srot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[4] );
  const float c = (const float) DaoValue_TryGetFloat( _p[5] );
  const float s = (const float) DaoValue_TryGetFloat( _p[6] );

  cblas_srot( N, X, incX, Y, incY, c, s );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_drot( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[4] );
  const double c = (const double) DaoValue_TryGetDouble( _p[5] );
  const double s = (const double) DaoValue_TryGetDouble( _p[6] );

  cblas_drot( N, X, incX, Y, incY, c, s );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_srotg( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  float a = (float) DaoValue_TryGetFloat( _p[0] );
  float* b = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  float* c = (float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  float* s = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_srotg( &a, b, c, s );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[2] );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
  DaoProcess_PutFloat( _proc, (float) a );
}
/* ./cblas.h */
static void dao__cblas_drotg( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  double a = (double) DaoValue_TryGetDouble( _p[0] );
  double* b = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  double* c = (double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  double* s = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_drotg( &a, b, c, s );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[2] );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
  DaoProcess_PutDouble( _proc, (double) a );
}
/* ./cblas.h */
static void dao__cblas_srotm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[3] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[4] );
  const float* P = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_srotm( N, X, incX, Y, incY, P );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_drotm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[2] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[3] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[4] );
  const double* P = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_drotm( N, X, incX, Y, incY, P );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[3] );
}
/* ./cblas.h */
static void dao__cblas_srotmg( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  float d1 = (float) DaoValue_TryGetFloat( _p[0] );
  float* d2 = (float*) DaoArray_ToFloat( (DaoArray*)_p[1] );
  float* b1 = (float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  const float b2 = (const float) DaoValue_TryGetFloat( _p[3] );
  float* P = (float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_srotmg( &d1, d2, b1, b2, P );
  DaoArray_FromFloat( (DaoArray*)_p[1] );
  DaoArray_FromFloat( (DaoArray*)_p[2] );
  DaoArray_FromFloat( (DaoArray*)_p[4] );
  DaoProcess_PutFloat( _proc, (float) d1 );
}
/* ./cblas.h */
static void dao__cblas_drotmg( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  double d1 = (double) DaoValue_TryGetDouble( _p[0] );
  double* d2 = (double*) DaoArray_ToDouble( (DaoArray*)_p[1] );
  double* b1 = (double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  const double b2 = (const double) DaoValue_TryGetDouble( _p[3] );
  double* P = (double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_drotmg( &d1, d2, b1, b2, P );
  DaoArray_FromDouble( (DaoArray*)_p[1] );
  DaoArray_FromDouble( (DaoArray*)_p[2] );
  DaoArray_FromDouble( (DaoArray*)_p[4] );
  DaoProcess_PutDouble( _proc, (double) d1 );
}
/* ./cblas.h */
static void dao__cblas_sscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[1] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );

  cblas_sscal( N, alpha, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_dscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[1] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );

  cblas_dscal( N, alpha, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_cscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[1] );

  cblas_cscal( N, alpha, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_zscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[1] );

  cblas_zscal( N, alpha, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_csscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[1] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );

  cblas_csscal( N, alpha, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_zdscal( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const int N = (const int) DaoValue_TryGetInteger( _p[0] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[1] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[3] );

  cblas_zdscal( N, alpha, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[2] );
}
/* ./cblas.h */
static void dao__cblas_sgemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int m = (const int) DaoValue_TryGetInteger( _p[2] );
  const int n = (const int) DaoValue_TryGetInteger( _p[3] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[8] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[9] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* a = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_sgemv( order, trans, m, n, alpha, a, lda, x, incx, beta, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_dgemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int m = (const int) DaoValue_TryGetInteger( _p[2] );
  const int n = (const int) DaoValue_TryGetInteger( _p[3] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[8] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[9] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* a = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dgemv( order, trans, m, n, alpha, a, lda, x, incx, beta, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_cgemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int m = (const int) DaoValue_TryGetInteger( _p[2] );
  const int n = (const int) DaoValue_TryGetInteger( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[8] );
  float* y = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* x = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* a = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_cgemv( order, trans, m, n, alpha, a, lda, x, incx, beta, y, incy );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_zgemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int m = (const int) DaoValue_TryGetInteger( _p[2] );
  const int n = (const int) DaoValue_TryGetInteger( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incx = (const int) DaoValue_TryGetInteger( _p[8] );
  double* y = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int incy = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* x = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* a = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_zgemv( order, trans, m, n, alpha, a, lda, x, incx, beta, y, incy );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_sger( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_sger( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dger( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dger( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_cgeru( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_cgeru( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_cgerc( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_cgerc( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_zgeru( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zgeru( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_zgerc( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const int M = (const int) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zgerc( order, M, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_strsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_strsv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_dtrsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dtrsv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_ctrsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_ctrsv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_ztrsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_ztrsv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_strmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_strmv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_dtrmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dtrmv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_ctrmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_ctrmv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_ztrmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_ztrmv( order, Uplo, TransA, Diag, N, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[7] );
}
/* ./cblas.h */
static void dao__cblas_ssyr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_ssyr( order, Uplo, N, alpha, X, incX, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_dsyr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dsyr( order, Uplo, N, alpha, X, incX, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_cher( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_cher( order, Uplo, N, alpha, X, incX, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_zher( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_zher( order, Uplo, N, alpha, X, incX, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ssyr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_ssyr2( order, Uplo, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dsyr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dsyr2( order, Uplo, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_cher2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_cher2( order, Uplo, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_zher2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zher2( order, Uplo, N, alpha, X, incX, Y, incY, A, lda );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_sgbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int M = (const int) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int KL = (const int) DaoValue_TryGetInteger( _p[4] );
  const int KU = (const int) DaoValue_TryGetInteger( _p[5] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[10] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[11] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[12] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[13] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );

  cblas_sgbmv( order, TransA, M, N, KL, KU, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_dgbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int M = (const int) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int KL = (const int) DaoValue_TryGetInteger( _p[4] );
  const int KU = (const int) DaoValue_TryGetInteger( _p[5] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[10] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[11] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[12] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[13] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );

  cblas_dgbmv( order, TransA, M, N, KL, KU, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_cgbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int M = (const int) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int KL = (const int) DaoValue_TryGetInteger( _p[4] );
  const int KU = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[10] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[12] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[13] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_cgbmv( order, TransA, M, N, KL, KU, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_zgbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const int M = (const int) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int KL = (const int) DaoValue_TryGetInteger( _p[4] );
  const int KU = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[10] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[12] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[13] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_zgbmv( order, TransA, M, N, KL, KU, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_ssbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int K = (const int) DaoValue_TryGetInteger( _p[3] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[9] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_ssbmv( order, Uplo, N, K, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_dsbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int K = (const int) DaoValue_TryGetInteger( _p[3] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[9] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dsbmv( order, Uplo, N, K, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_stbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_stbmv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dtbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_dtbmv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_ctbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_ctbmv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_ztbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_ztbmv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_stbsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_stbsv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dtbsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_dtbsv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_ctbsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_ctbsv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_ztbsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_ztbsv( order, Uplo, TransA, Diag, N, K, A, lda, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_stpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_stpmv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_dtpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dtpmv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ctpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_ctpmv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ztpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_ztpmv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_stpsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_stpsv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_dtpsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_dtpsv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ctpsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  float* X = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_ctpsv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ztpsv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  double* X = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_ztpsv( order, Uplo, TransA, Diag, N, Ap, X, incX );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_ssymv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[8] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[10] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_ssymv( order, Uplo, N, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_dsymv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[8] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[10] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dsymv( order, Uplo, N, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_chemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[10] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_chemv( order, Uplo, N, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_zhemv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[7] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[10] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zhemv( order, Uplo, N, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_sspmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[6] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[7] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_sspmv( order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dspmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[6] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[7] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dspmv( order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_sspr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  float* Ap = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_sspr( order, Uplo, N, alpha, X, incX, Ap );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_dspr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  double* Ap = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dspr( order, Uplo, N, alpha, X, incX, Ap );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_chpr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_chpr( order, Uplo, N, alpha, X, incX, A );
  DaoArray_FromFloat( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_zhpr( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_zhpr( order, Uplo, N, alpha, X, incX, A );
  DaoArray_FromDouble( (DaoArray*)_p[6] );
}
/* ./cblas.h */
static void dao__cblas_sspr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* A = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_sspr2( order, Uplo, N, alpha, X, incX, Y, incY, A );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_dspr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[3] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* A = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_dspr2( order, Uplo, N, alpha, X, incX, Y, incY, A );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_chpr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  float* Ap = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* Y = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_chpr2( order, Uplo, N, alpha, X, incX, Y, incY, Ap );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_zhpr2( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[5] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[7] );
  double* Ap = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* Y = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zhpr2( order, Uplo, N, alpha, X, incX, Y, incY, Ap );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_chbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int K = (const int) DaoValue_TryGetInteger( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );

  cblas_chbmv( order, Uplo, N, K, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_zhbmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int K = (const int) DaoValue_TryGetInteger( _p[3] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[6] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[8] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );

  cblas_zhbmv( order, Uplo, N, K, alpha, A, lda, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_chpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[6] );
  float* Y = (float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[9] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* X = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );
  const float* Ap = (const float*) DaoArray_ToFloat( (DaoArray*)_p[4] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[3] );

  cblas_chpmv( order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY );
  DaoArray_FromFloat( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_zhpmv( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const int N = (const int) DaoValue_TryGetInteger( _p[2] );
  const int incX = (const int) DaoValue_TryGetInteger( _p[6] );
  double* Y = (double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const int incY = (const int) DaoValue_TryGetInteger( _p[9] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* X = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );
  const double* Ap = (const double*) DaoArray_ToDouble( (DaoArray*)_p[4] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[3] );

  cblas_zhpmv( order, Uplo, N, alpha, Ap, X, incX, beta, Y, incY );
  DaoArray_FromDouble( (DaoArray*)_p[8] );
}
/* ./cblas.h */
static void dao__cblas_sgemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransB = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[10] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[11] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[12] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[13] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );

  cblas_sgemm( Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_dgemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransB = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[10] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[11] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[12] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[13] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );

  cblas_dgemm( Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_cgemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransB = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[10] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[12] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[13] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_cgemm( Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_zgemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE TransB = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int K = (const int) DaoValue_TryGetInteger( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[8] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[10] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[12] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[13] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_zgemm( Order, TransA, TransB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[12] );
}
/* ./cblas.h */
static void dao__cblas_ssymm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[10] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_ssymm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_dsymm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[10] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_dsymm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_csymm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_csymm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_zsymm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_zsymm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_ssyrk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[8] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_ssyrk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_dsyrk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[8] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_dsyrk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_csyrk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_csyrk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_zsyrk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_zsyrk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_ssyr2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[10] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_ssyr2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_dsyr2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[10] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_dsyr2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_csyr2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_csyr2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_zsyr2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_zsyr2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_strmm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[7] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  float* B = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );

  cblas_strmm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_dtrmm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[7] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  double* B = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );

  cblas_dtrmm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_ctrmm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  float* B = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );

  cblas_ctrmm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_ztrmm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  double* B = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );

  cblas_ztrmm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_strsm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[7] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  float* B = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );

  cblas_strsm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_dtrsm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[7] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  double* B = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );

  cblas_dtrsm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_ctrsm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  float* B = (float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[7] );

  cblas_ctrsm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromFloat( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_ztrsm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const enum CBLAS_TRANSPOSE TransA = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[3] );
  const enum CBLAS_DIAG Diag = (const enum CBLAS_DIAG) DaoValue_TryGetInteger( _p[4] );
  const int M = (const int) DaoValue_TryGetInteger( _p[5] );
  const int N = (const int) DaoValue_TryGetInteger( _p[6] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[9] );
  double* B = (double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[11] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[7] );

  cblas_ztrsm( Order, Side, Uplo, TransA, Diag, M, N, alpha, A, lda, B, ldb );
  DaoArray_FromDouble( (DaoArray*)_p[10] );
}
/* ./cblas.h */
static void dao__cblas_chemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* beta = (const float*) DaoArray_ToFloat( (DaoArray*)_p[10] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_chemm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_zhemm( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_SIDE Side = (const enum CBLAS_SIDE) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[2] );
  const int M = (const int) DaoValue_TryGetInteger( _p[3] );
  const int N = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* beta = (const double*) DaoArray_ToDouble( (DaoArray*)_p[10] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_zhemm( Order, Side, Uplo, M, N, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_cherk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const float alpha = (const float) DaoValue_TryGetFloat( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[8] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );

  cblas_cherk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_zherk( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const double alpha = (const double) DaoValue_TryGetDouble( _p[5] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[8] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[9] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[10] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );

  cblas_zherk( Order, Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[9] );
}
/* ./cblas.h */
static void dao__cblas_cher2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const float beta = (const float) DaoValue_TryGetFloat( _p[10] );
  float* C = (float*) DaoArray_ToFloat( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const float* B = (const float*) DaoArray_ToFloat( (DaoArray*)_p[8] );
  const float* A = (const float*) DaoArray_ToFloat( (DaoArray*)_p[6] );
  const float* alpha = (const float*) DaoArray_ToFloat( (DaoArray*)_p[5] );

  cblas_cher2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromFloat( (DaoArray*)_p[11] );
}
/* ./cblas.h */
static void dao__cblas_zher2k( DaoProcess *_proc, DaoValue *_p[], int _n )
{
  const enum CBLAS_ORDER Order = (const enum CBLAS_ORDER) DaoValue_TryGetInteger( _p[0] );
  const enum CBLAS_UPLO Uplo = (const enum CBLAS_UPLO) DaoValue_TryGetInteger( _p[1] );
  const enum CBLAS_TRANSPOSE Trans = (const enum CBLAS_TRANSPOSE) DaoValue_TryGetInteger( _p[2] );
  const int N = (const int) DaoValue_TryGetInteger( _p[3] );
  const int K = (const int) DaoValue_TryGetInteger( _p[4] );
  const int lda = (const int) DaoValue_TryGetInteger( _p[7] );
  const int ldb = (const int) DaoValue_TryGetInteger( _p[9] );
  const double beta = (const double) DaoValue_TryGetDouble( _p[10] );
  double* C = (double*) DaoArray_ToDouble( (DaoArray*)_p[11] );
  const int ldc = (const int) DaoValue_TryGetInteger( _p[12] );
  const double* B = (const double*) DaoArray_ToDouble( (DaoArray*)_p[8] );
  const double* A = (const double*) DaoArray_ToDouble( (DaoArray*)_p[6] );
  const double* alpha = (const double*) DaoArray_ToDouble( (DaoArray*)_p[5] );

  cblas_zher2k( Order, Uplo, Trans, N, K, alpha, A, lda, B, ldb, beta, C, ldc );
  DaoArray_FromDouble( (DaoArray*)_p[11] );
}
#ifdef __cplusplus
}
#endif
static DaoNumItem dao__Nums[] = 
{
  { "CblasRowMajor", DAO_INTEGER, CblasRowMajor },
  { "CblasColMajor", DAO_INTEGER, CblasColMajor },
  { "CblasNoTrans", DAO_INTEGER, CblasNoTrans },
  { "CblasTrans", DAO_INTEGER, CblasTrans },
  { "CblasConjTrans", DAO_INTEGER, CblasConjTrans },
  { "CblasConjNoTrans", DAO_INTEGER, CblasConjNoTrans },
  { "CblasUpper", DAO_INTEGER, CblasUpper },
  { "CblasLower", DAO_INTEGER, CblasLower },
  { "CblasNonUnit", DAO_INTEGER, CblasNonUnit },
  { "CblasUnit", DAO_INTEGER, CblasUnit },
  { "CblasLeft", DAO_INTEGER, CblasLeft },
  { "CblasRight", DAO_INTEGER, CblasRight },
  { NULL, 0, 0 }
};
static const char *dao__Aliases[] = 
{
	"int", "blasint",
	"int", "CBLAS_ORDER",
	"int", "CBLAS_TRANSPOSE",
	"int", "CBLAS_UPLO",
	"int", "CBLAS_DIAG",
	"int", "CBLAS_SIDE",
	NULL
};
#ifdef __cplusplus
extern "C"{
#endif
int DaoOnLoad( DaoVmSpace *vms, DaoNamespace *ns )
{
	__daoVmSpace = vms;
	DaoNamespace *aux = DaoVmSpace_LinkModule( vms, ns, "aux" );
	if( aux == NULL ) return 1;
	DaoNamespace_AddConstNumbers( ns, dao__Nums );
	DaoNamespace_TypeDefines( ns, dao__Aliases );
	DaoNamespace_WrapFunctions( ns, dao__Funcs );
	return 0;
}
#ifdef __cplusplus
}
#endif
