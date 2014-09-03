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

/* Contribution logs: */
/* 2011-02, Aleksey Danilov: added initial implementation. */
/* 2014-01, Aleksey Danilov: numerous enhancements and additions. */
/* 2014-07, Aleksey Danilov: support for Unicode file names in Windows, reorganization of the interface. */

#include<string.h>
#include<errno.h>
#include<limits.h>

#include"dao.h"
#include"daoValue.h"

#ifdef WIN32

typedef wchar_t char_t;
typedef struct __stat64 stat_t;

#define T( tcs ) L##tcs
#define T_FMT "ls"

#define tcscpy wcscpy
#define tcsncpy wcsncpy
#define tcscat wcscat
#define tcslen wcslen
#define tcscmp wcscmp
#define tcschr wcschr
#define tcsrchr wcsrchr
#define tcsstr wcsstr

#define fopen _wfopen
#define stat _wstat64
#define rename _wrename
#define rmdir _wrmdir
#define unlink _wunlink
#define mkdir _wmkdir
#define chmod _wchmod
#define getcwd _wgetcwd
#define chdir _wchdir

char_t* CharsToTChars( char *chs )
{
	char_t *res = (char_t*)dao_malloc( sizeof(char_t)*( MAX_PATH + 1) );
	size_t i;
	char *end = chs + strlen( chs );
	for ( i = 0; i < MAX_PATH + 1; i++ ){
		DCharState st = DString_DecodeChar( chs, end );
		if ( st.type == 0 )
			break;
		if ( st.value <= 0xFFFF )
			res[i] = st.value;
		else {
			st.value -= 0x10000;
			res[i++] = ( st.value >> 10 ) + 0xD800;
			if ( i < MAX_PATH + 1 )
				res[i] = ( st.value & 0x3FF ) + 0xDC00;
		}
		chs += st.width;
	}
	res[( i < MAX_PATH + 1 )? i : MAX_PATH] = T('\0');
	return res;
}

void FreeTChars( char_t *tcs)
{
	dao_free( tcs );
}

void DString_SetTChars( DString *str, char_t *tcs )
{
	DString_Clear( str );
	for ( ; *tcs != T('\0'); tcs++ )
		if ( *tcs >= 0xD800 && *tcs <= 0xDBFF ){
			size_t lead = ( (size_t)*tcs - 0xD800 ) << 10;
			tcs++;
			if ( *tcs == T('\0') )
					break;
			DString_AppendWChar( str, lead + ( (size_t)*tcs - 0xDC00 ) );
		}
		else
			DString_AppendWChar( str, *tcs );
}

DaoString* DaoString_NewTChars( char_t *tcs )
{
	DaoString *res = DaoString_New();
	DString_SetTChars( res->value, tcs );
	return res;
}

DString* DaoProcess_PutTChars( DaoProcess *proc, char_t *tcs )
{
	DString *res = DaoProcess_PutChars( proc, "" );
	DString_SetTChars( res, tcs );
	return res;
}

#include<sys/stat.h>
#include<io.h>
#include<fcntl.h>
#include<lmcons.h>
#include<aclapi.h>
#include<Shlobj.h>

#ifndef ELOOP
#define ELOOP 114
#endif

/* Windows functions which read or change working directory are not thread-safe, so certain safety measures are desirable */
static DMutex fs_mtx;
#define FS_TRANS( st ) DMutex_Lock( &fs_mtx ); st; DMutex_Unlock( &fs_mtx )
#define FS_INIT() DMutex_Init( &fs_mtx )

#else

#define _FILE_OFFSET_BITS 64
#include<dirent.h>
#include<sys/stat.h>

typedef char char_t;
typedef struct stat stat_t;

#define T( tcs ) tcs
#define T_FMT "s"

#define tcscpy strcpy
#define tcsncpy strncpy
#define tcscat strcat
#define tcslen strlen
#define tcscmp strcmp
#define tcschr strchr
#define tcsrchr strrchr
#define tcsstr strstr

#define DString_SetTChars( str, tcs ) DString_SetChars( str, tcs )
#define DaoString_NewTChars( tcs ) DString_NewChars( tcs )
#define DaoProcess_PutTChars( proc, tcs ) DaoProcess_PutChars( proc, tcs )
#define CharsToTChars( chs ) ( chs )
#define FreeTChars( tcs )

#define FS_TRANS( st ) st
#define FS_INIT()
#endif

#ifdef UNIX
#include<unistd.h>
#include<sys/time.h>
#include<pwd.h>
#endif

#ifndef MAX_PATH
#ifndef PATH_MAX
#define MAX_PATH 512
#else
#define MAX_PATH PATH_MAX
#endif
#endif

#define MAX_ERRMSG 100

static const char fserr[] = "File";

struct DInode
{
	char_t *path;
	short type;
	time_t ctime;
	time_t atime;
	time_t mtime;
	mode_t mode;
	daoint size;
#ifndef WIN32
	uid_t uid;
#endif
};

typedef struct DInode DInode;

DaoType *daox_type_entry = NULL;
DaoType *daox_type_file = NULL;
DaoType *daox_type_dir = NULL;

DInode* DInode_New()
{
	DInode *res = (DInode*)dao_malloc( sizeof(DInode) );
	res->path = NULL;
	res->type = -1;
	return res;
}

void DInode_Close( DInode *self )
{
	if( self->path ){
		dao_free( self->path );
		self->path = NULL;
		self->type = -1;
	}
}

void DInode_Delete( DInode *self )
{
	DInode_Close( self );
	dao_free( self );
}

#ifdef WIN32
#define IS_PATH_SEP( c ) ( ( c ) == T('\\') || ( c ) == T('/') || ( c ) == T(':') )
#else
#define IS_PATH_SEP( c ) ( ( c ) == T('/') )
#endif

int NormalizePath( const char_t *path, char_t *dest )
{
#ifdef WIN32
	int res;
	FS_TRANS( res = GetFullPathNameW( path, MAX_PATH + 1, dest, NULL ) != 0 );
	if ( res ){
		int i;
		for ( i = 0; dest[i] != T('\0'); i++ )
			if ( dest[i] == T('\\') )
				dest[i] = T('/');
	}
	return res? 0 : EINVAL;
#else
	return realpath( path, dest )? 0 : errno;
#endif
}

int DInode_Open( DInode *self, const char_t *path )
{
	char_t buf[MAX_PATH + 1];
	stat_t info;
	size_t len;
	int res;
	if( stat( path, &info ) != 0 )
		return errno;
	if ( ( res = NormalizePath( path, buf ) ) != 0 )
		return res;
	DInode_Close( self );
	len = tcslen( buf );
#ifdef WIN32
	if( !( info.st_mode & _S_IFDIR ) && !( info.st_mode & _S_IFREG ) )
		return -1;
	self->type = ( info.st_mode & _S_IFDIR )? 0 : 1;
	self->size = ( info.st_mode & _S_IFDIR )? 0 : info.st_size;
#else
	if( !S_ISDIR( info.st_mode ) && !S_ISREG( info.st_mode ) )
		return -1;
	self->type = ( S_ISDIR( info.st_mode ) )? 0 : 1;
	self->size = ( S_ISDIR( info.st_mode ) )? 0 : info.st_size;
	self->uid = info.st_uid;
#endif
	self->path = (char_t*)dao_malloc( sizeof(char_t)*( len + 1 ) );
	tcscpy( self->path, buf );
#ifndef WIN32
	if( self->path[len - 1] == T('/') && len > 1 )
		self->path[len - 1] = T('\0');
#endif
	self->ctime = info.st_ctime;
	self->mtime = info.st_mtime;
	self->atime = info.st_atime;
	self->mode = info.st_mode;
	return 0;
}

int DInode_Reopen( DInode *self )
{
	stat_t info;
	if( stat( self->path, &info ) != 0 )
		return errno;
#ifdef WIN32
	if( !( info.st_mode & _S_IFDIR ) && !( info.st_mode & _S_IFREG ) )
		return -1;
	self->type = ( info.st_mode & _S_IFDIR )? 0 : 1;
	self->size = ( info.st_mode & _S_IFDIR )? 0 : info.st_size;
#else
	if( !S_ISDIR( info.st_mode ) && !S_ISREG( info.st_mode ) )
		return -1;
	self->type = ( S_ISDIR( info.st_mode ) )? 0 : 1;
	self->size = ( S_ISDIR( info.st_mode ) )? 0 : info.st_size;
	self->uid = info.st_uid;
#endif
	self->ctime = info.st_ctime;
	self->mtime = info.st_mtime;
	self->atime = info.st_atime;
	self->mode = info.st_mode;
	return 0;
}

char_t* DInode_Parent( DInode *self, char_t *buffer )
{
	size_t i, len;
	if( !self->path )
		return NULL;
	len = tcslen( self->path );
#ifdef WIN32
	if ( len > 2 && self->path[len - 1] == T('/') && self->path[0] == T('/')  ) /* UNC volume */
		return NULL;
#endif
	for (i = len - 1; i > 0; i--)
		if( IS_PATH_SEP( self->path[i] ) )
			break;
#ifdef WIN32
	if( self->path[i] == T(':') ){
		tcsncpy( buffer, self->path, i + 1 );
		tcscpy( buffer + i + 1, T("/") );
	}
	else{
		if( i == 2 )
			i++;
		tcsncpy( buffer, self->path, i );
		if( self->path[0] == T('/') ){
			int j;
			int k = 0;
			for (j = 2; j < i && k < 2; j++)
				if( IS_PATH_SEP( self->path[j] ) )
					k++;
			if( i == j ){
				buffer[i] = T('/');
				i++;
			}
		}
		buffer[i] = T('\0');
	}
#else
	if( i == 0 )
		tcscpy( buffer, T("/") );
	else{
		tcsncpy( buffer, self->path, i );
		buffer[i] = T('\0');
	}
#endif
	return buffer;
}

int DInode_Rename( DInode *self, const char_t *path )
{
	char_t buf[MAX_PATH + 1], nbuf[MAX_PATH + 1];
	size_t len;
	int res;
	if( !self->path || !DInode_Parent( self, buf ) )
		return -1;
	len = tcslen( path );
	if ( len > MAX_PATH )
		return ENAMETOOLONG;
	tcscpy( buf, path );
#ifdef WIN32
	if ( len > 1 && IS_PATH_SEP( path[len - 1] ) && path[len - 2] != T(':') ){
#else
	if ( len > 1 && path[len - 1] == T('/') ){
#endif
		int i;
		for (i = tcslen( self->path ) - 1; i > 0; i--)
			if( IS_PATH_SEP( self->path[i] ) )
				break;
		tcscat( buf, self->path + i + 1 );
	}
	if( rename( self->path, buf ) != 0 )
		return errno;
	if ( ( res = NormalizePath( buf, nbuf ) ) != 0 )
		return res;
	self->path = (char_t*)dao_realloc( self->path, sizeof(char_t)*( tcslen( nbuf ) + 1 ) );
	tcscpy( self->path, nbuf );
	return 0;
}

int DInode_Remove( DInode *self )
{
	if( !self->path )
		return -1;
	if( self->type == 0 ){
		if( rmdir( self->path ) != 0 )
			return errno;
	}else{
		if( unlink( self->path ) != 0 )
			return errno;
	}
	return 0;
}

int DInode_SubInode( DInode *self, const char_t *path, int dir, DInode *dest, int exists )
{
	char_t buf[MAX_PATH + 1];
	if( !self->path || self->type != 0 )
		return -1;
	tcscpy( buf, self->path );
	if ( self->path[tcslen( self->path ) - 1] != T('/') )
		tcscat( buf, T("/") );
	if( tcslen( buf ) + tcslen( path ) > MAX_PATH )
		return ENAMETOOLONG;
	tcscat( buf, path );
	if ( !exists ){
		if ( !dir ){
			FILE *handle;
			if ( !( handle = fopen( buf, T("w") ) ) )
				return ( errno == EINVAL )? -1 : errno;
			fclose( handle );
		}
		else {
#ifdef WIN32
			if ( mkdir( buf ) != 0 )
				return ( errno == EINVAL )? -1 : errno;
#else
			if ( mkdir( buf, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH ) != 0 )
				return ( errno == EINVAL )? -1 : errno;
#endif
		}
	}
	return DInode_Open( dest, buf );
}

int DInode_ChildrenRegex( DInode *self, int type, DaoProcess *proc, DaoList *dest, DaoRegex *pattern )
{
	char_t buffer[MAX_PATH + 1];
	size_t len;
	int res;
#ifdef WIN32
	intptr_t handle;
	struct _wfinddata64_t finfo;
#else
	DIR *handle;
	struct dirent *finfo;
#endif
	if( !self->path || self->type != 0 )
		return -1;
	tcscpy( buffer, self->path );
	len = tcslen( buffer );
#ifdef WIN32
	/* Using _findfirst/_findnext for Windows */
	if( IS_PATH_SEP( buffer[len - 1] ) )
		tcscpy( buffer + len, T("*") );
    else
		tcscpy( buffer + len++, T("/*") );
	handle = _wfindfirst64( buffer, &finfo );
	if (handle != -1){
		DString *str = DString_New();
		DaoValue *value;
		DInode *fsnode;
		do
			if( tcscmp( finfo.name, T(".") ) && tcscmp( finfo.name, T("..") ) ){
				DString_SetTChars( str, finfo.name );
				if ( pattern ){
					tcscpy( buffer + len, finfo.name );
					fsnode = DInode_New();
					if( ( res = DInode_Open( fsnode, buffer ) ) != 0 ){
						DInode_Delete( fsnode );
						return res;
					}
					if( ( fsnode->type == type || type == 2 ) && DaoRegex_Match( pattern, str, NULL, NULL ) ){
						value = (DaoValue*) DaoProcess_NewCdata( proc, fsnode->type == 0? daox_type_dir : daox_type_file, fsnode, 1 );
						DaoList_PushBack( dest, value );
					}
					else
						DInode_Delete( fsnode );
				}
				else {
					DaoString *name = DaoString_New();
					DaoString_Set( name, str );
					DaoList_PushBack( dest, (DaoValue*)name );
				}
			}
		while( !_wfindnext64( handle, &finfo ) );
		DString_Delete( str );
		_findclose( handle );
	}
#else
	/* Using POSIX opendir/readdir otherwise */
	handle = opendir( buffer );
	if( !IS_PATH_SEP( buffer[len - 1] ) )
		tcscpy( buffer + len++, T("/") );
	if( handle ){
		DString *str = DString_New();
		DaoValue *value;
		DInode *fsnode;
		while( ( finfo = readdir( handle ) ) )
			if( tcscmp( finfo->d_name, T(".") ) && tcscmp( finfo->d_name, T("..") ) ){
				DString_SetTChars( str, finfo->d_name );
				if ( pattern ){
					tcscpy( buffer + len, finfo->d_name );
					fsnode = DInode_New();
					if( ( res = DInode_Open( fsnode, buffer ) ) != 0 ){
						DInode_Delete( fsnode );
						return res;
					}
					if( ( fsnode->type == type || type == 2 ) && DaoRegex_Match( pattern, str, NULL, NULL ) ){
						value = (DaoValue*) DaoProcess_NewCdata( proc, fsnode->type == 0? daox_type_dir : daox_type_file, fsnode, 1 );
						DaoList_PushBack( dest, value );
					}
					else
						DInode_Delete( fsnode );
				}
				else {
					DaoString *name = DaoString_New();
					DaoString_Set( name, str );
					DaoList_PushBack( dest, (DaoValue*)name );
				}
			}
		DString_Delete( str );
		closedir( handle );
	}
#endif
	else
		return errno;
	return 0;
}

int DInode_SetAccess( DInode *self, int ur, int uw, int ux, int gr, int gw, int gx, int otr, int otw, int otx )
{
#ifdef WIN32
	if ( chmod( self->path, (ur? _S_IREAD : 0) | (uw? _S_IWRITE : 0) ) )
#else
	if ( chmod( self->path, (ur? S_IRUSR : 0) | (uw? S_IWUSR : 0) | (ux? S_IXUSR : 0) |
				(gr? S_IRGRP : 0) | (gw? S_IWGRP : 0) | (gx? S_IXGRP : 0) |
				(otr? S_IROTH : 0) | (otw? S_IWOTH : 0) | (otx? S_IXOTH : 0) ) )
#endif
		return errno;
	return 0;
}

void DInode_GetMode( DInode *self, DaoTuple *res )
{
	/* Tuple returned from DaoProcess_PutTuple() could be a reused one: */
	res->values[0]->xString.value->size = 0;
	res->values[1]->xString.value->size = 0;
	res->values[2]->xString.value->size = 0;
#ifdef WIN32
	if ( self->mode & _S_IREAD )
		DString_AppendChars( res->values[0]->xString.value, "r" );
	if ( self->mode & _S_IWRITE )
		DString_AppendChars( res->values[0]->xString.value, "w" );
	if ( self->mode & _S_IEXEC )
		DString_AppendChars( res->values[0]->xString.value, "x" );
#else
	if ( self->mode & S_IRUSR )
		DString_AppendChars( res->values[0]->xString.value, "r" );
	if ( self->mode & S_IWUSR )
		DString_AppendChars( res->values[0]->xString.value, "w" );
	if ( self->mode & S_IXUSR )
		DString_AppendChars( res->values[0]->xString.value, "x" );

	if ( self->mode & S_IRGRP )
		DString_AppendChars( res->values[1]->xString.value, "r" );
	if ( self->mode & S_IWGRP )
		DString_AppendChars( res->values[1]->xString.value, "w" );
	if ( self->mode & S_IXGRP )
		DString_AppendChars( res->values[1]->xString.value, "x" );

	if ( self->mode & S_IROTH )
		DString_AppendChars( res->values[2]->xString.value, "r" );
	if ( self->mode & S_IWOTH )
		DString_AppendChars( res->values[2]->xString.value, "w" );
	if ( self->mode & S_IXOTH )
		DString_AppendChars( res->values[2]->xString.value, "x" );
#endif
}

int DInode_GetOwner( DInode *self, DString *name )
{
#ifdef WIN32
	PSID sid = NULL;
	SID_NAME_USE sidname;
	PSECURITY_DESCRIPTOR sd = NULL;
	char_t buf[UNLEN], dbuf[UNLEN];
	DWORD bufsize = UNLEN, sdsize = UNLEN;
	if ( GetNamedSecurityInfoW( self->path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &sid, NULL, NULL, NULL, &sd )
		 != ERROR_SUCCESS )
		return 1;
	if ( !LookupAccountSidW( NULL, sid, buf, &bufsize, dbuf, &sdsize, &sidname ) ){
		LocalFree( sd );
		return 1;
	}
	DString_SetTChars( name, buf );
	LocalFree( sd );
	return 0;
#else
	struct passwd pwd, *res;
	char_t buf[4096];
	if ( getpwuid_r( self->uid, &pwd, buf, 4096, &res ) )
		return 1;
	DString_SetTChars( name, pwd.pw_name );
	return 0;
#endif
}

int DInode_Resize( DInode *self, daoint size )
{
	int res = 0;
#ifdef WIN32
	HANDLE handle = CreateFileW( self->path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	LARGE_INTEGER usize;
	if ( handle == INVALID_HANDLE_VALUE )
		return -1;
	usize.u.LowPart = size;
	usize.u.HighPart = ( sizeof(daoint) == 4 )? 0 : ( size >> 32 );
	res = ( SetFilePointerEx( handle, usize, NULL, FILE_BEGIN ) && SetEndOfFile( handle ) )? 0 : -1;
	CloseHandle( handle );
#else
	res = truncate( self->path, size );
#endif
	if ( !res )
		self->size = size;
	return res;
}

int MakeTmpFile( char_t *dir, char_t *prefix, char_t *namebuf )
{
	char_t buf[MAX_PATH + 1];
	int res;
	size_t i, len = tcslen( dir ) + tcslen( prefix );
	if ( len > MAX_PATH - 6 )
		return -1;
	if ( ( res = NormalizePath( dir, buf ) ) != 0 )
		return res;
	for ( i = 0; i < tcslen( prefix ); i++ )
		if ( IS_PATH_SEP( prefix[i] ) )
			return -2;
#ifdef WIN32
	return GetTempFileNameW( buf, prefix, 0, namebuf )? 0 : errno;
#else
	tcscpy( namebuf, buf );
	if ( *namebuf != T('\0') && namebuf[tcslen( namebuf ) - 1] != T('/') )
		tcscat( namebuf, T("/") );
	tcscat( namebuf, prefix );
	tcscat( namebuf, T("XXXXXX") );
	res = mkstemp( namebuf );
	if ( res == -1 )
		return errno;
	else {
		close( res );
		return 0;
	}
#endif
}

int GetRootDirs( DaoList *dest )
{
#ifdef WIN32
	char_t buf[MAX_PATH + 1];
	char_t *pos, *start;
	if ( !GetLogicalDriveStringsW( MAX_PATH, buf ) )
		return 0;
	start = buf;
	while ( *start != T('\0') ){
		pos = tcschr( start, T('\0') );
		DaoList_PushBack( dest, (DaoValue*)DaoString_NewTChars( start ) );
		start = pos + 1;
	}
#else
	DaoList_PushBack( dest, (DaoValue*)DaoString_NewTChars( T("/") ) );
#endif
	return 1;
}

static void GetErrorMessage( char *buffer, int code, int special )
{
	switch ( code ){
	case EACCES:
	case EBADF:			strcpy( buffer, "Access not permitted (EACCES/EBADF)"); break;
	case EBUSY:			strcpy (buffer, "Path is being used (EBUSY)" ); break;
	case ENOTEMPTY:
	case EEXIST:		strcpy( buffer, special? "Directory is not empty (ENOTEMPTY/EEXIST)" :
												 "File object already exists (EEXIST/ENOTEMPTY)" ); break;
	case EPERM:
	case ENOTDIR:
	case EISDIR:		strcat( buffer, "Inconsistent type of file object (EPERM/ENOTDIR/EISDIR)" ); break;
	case EINVAL:		strcpy( buffer, special? "Invalid path (EINVAL)" : "Making a directory its own subdirectory (EINVAL)" ); break;
	case EMLINK:		strcat( buffer, "Too many entries in parent directory (EMLINK)" ); break;
	case ENOENT:		strcpy( buffer, "Path does not exist (ENOENT)" ); break;
	case ENOSPC:		strcpy( buffer, "Not enough free space in the file system (ENOSPC)" ); break;
	case EROFS:			strcpy( buffer, "Writing to a read-only file system (EROFS)" ); break;
	case EXDEV:			strcpy( buffer, "Relocating file object to a different file system (EXDEV)" ); break;
	case ENAMETOOLONG:	strcpy( buffer, "Path is too long (ENAMETOOLONG)" ); break;
	case EMFILE:
	case ENFILE:		strcpy( buffer, "Too many files open (EMFILE/ENFILE)" ); break;
	case ENOMEM:		strcpy( buffer, "Not enough memory (ENOMEM)" ); break;
	case EFBIG:			strcpy( buffer, "File size is too large (EFBIG)" ); break;
	case EIO:			strcpy( buffer, "Hardware I/O error (EIO)" ); break;
	case EINTR:			strcpy( buffer, "Operation interrupted by a signal (EINTR)" ); break;
	case ELOOP:			strcpy( buffer, "Too many symbolic links" ); break;
	default:			sprintf( buffer, "Unknown system error (%x)", code );
	}
}

static void FSNode_Update( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char errbuf[MAX_ERRMSG + MAX_PATH + 3];
	int res;
	if( ( res = DInode_Reopen( self ) ) != 0 ){
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, self->path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
}

static void FSNode_Path( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutTChars( proc, self->path );
}

static void FSNode_Name( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	size_t i;
	for (i = tcslen( self->path ) - 1; i > 0; i--)
		if( IS_PATH_SEP( self->path[i] ) )
			break;
	if( self->path[i + 1] == T('\0') )
		DaoProcess_PutTChars( proc, self->path );
	else
		DaoProcess_PutTChars( proc, self->path + i + 1 );
}

static void FSNode_BaseName( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	size_t i;
	char_t *name, *pos;
	for (i = tcslen( self->path ) - 1; i > 0; i--)
		if( IS_PATH_SEP( self->path[i] ) )
			break;
	name = self->path;
	if( self->path[i + 1] != T('\0') )
		name += i + 1;
	pos = tcschr( name, T('.') );
	if ( pos ){
		char_t buf[MAX_PATH + 1];
		tcsncpy( buf, name, pos - name );
		buf[pos - name] = T('\0');
		DaoProcess_PutTChars( proc, buf );
	}
	else
		DaoProcess_PutTChars( proc, name );
}

static void FSNode_Parent( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *par;
	char_t path[MAX_PATH + 1];
	int res = 0;
	if ( !DInode_Parent( self, path ) ){
		DaoProcess_PutNone( proc );
		return;
	}
	par = DInode_New();
	if( ( res = DInode_Open( par, path ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		DInode_Delete( par );
		GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)par, daox_type_dir );
}

static void FSNode_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->type == 1 ? "file" : "dir" );
}

static void FSNode_Size( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->size );
}

static void FSNode_Resize( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	daoint size = p[1]->xInteger.value;
	int res;
	if ( self->type == 0 ){
		DaoProcess_RaiseError( proc, fserr, "Resizing a directory" );
		return;
	}
	if ( ( res = DInode_Resize( self, size ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		if( res == -1 )
			strcpy( errbuf, "Failed to resize file" );
		else
			GetErrorMessage( errbuf, errno, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
}

static void FSNode_Rename( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	int res;
	char errbuf[MAX_ERRMSG];
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	if ( (res = DInode_Rename( self, path ) ) != 0 ){
		if( res == -1 )
			strcpy( errbuf, "Renaming root directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	FreeTChars( path );
}

static void FSNode_Remove( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	int res;
	char errbuf[MAX_ERRMSG];
	if ( (res = DInode_Remove( self ) ) != 0 ){
		GetErrorMessage( errbuf, res, self->type == 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
}

static void FSNode_Time( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *res = DaoProcess_PutTuple( proc, 3 );
	res->values[0]->xInteger.value = self->ctime;
	res->values[1]->xInteger.value = self->mtime;
	res->values[2]->xInteger.value = self->atime;
}

static void FSNode_Access( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *res = DaoProcess_PutTuple( proc, 3 );
	DInode_GetMode( self, res );
}

int InitPermissions( DString *mode, int *r, int *w, int *x )
{
	daoint i;
	*r = *w = *x = 0;
	for ( i = 0; i < mode->size; i++ )
		switch ( mode->chars[i] ){
		case 'r': if ( *r ) return 0; *r = 1; break;
		case 'w': if ( *w ) return 0; *w = 1; break;
		case 'x': if ( *x ) return 0; *x = 1; break;
		default: return 0;
		}
	return 1;
}

static void FSNode_SetAccess(DaoProcess *proc, DaoValue *p[], int N)
{
	DInode *self = (DInode*)DaoValue_TryGetCdata(p[0]);
	char errbuf[MAX_ERRMSG];
	int ur, uw, ux, gr, gw, gx, otr, otw, otx;
	DString *mode = p[1]->xTuple.values[0]->xString.value;
	if ( !InitPermissions( mode, &ur, &uw, &ux ) )
		goto Error;
	mode = p[1]->xTuple.values[1]->xString.value;
	if ( !InitPermissions( mode, &gr, &gw, &gx ) )
		goto Error;
	mode = p[1]->xTuple.values[2]->xString.value;
	if ( !InitPermissions( mode, &otr, &otw, &otx ) )
		goto Error;
	int res = DInode_SetAccess(self, ur, uw, ux, gr, gw, gx, otr, otw, otx);
	if (res){
		GetErrorMessage(errbuf, res, 0);
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		FSNode_Update(proc, p, N);
	return;
Error:
	DaoProcess_RaiseError( proc, fserr, "Invalid access mode format" );
	DString_Delete( mode );
}

static void FSNode_Makefile( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
		FreeTChars( path );
		return;
	}
	child = DInode_New();
	if( ( res = DInode_SubInode( self, path, 0, child, 0 ) ) != 0 ){
		char errbuf[MAX_ERRMSG] = {0};
		DInode_Delete( child );
		if( res == -1 )
			strcpy( errbuf, "Invalid file name (EINVAL)" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, (void*)child, daox_type_file );
	FreeTChars( path );
}

static void FSNode_Makedir( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
		FreeTChars( path );
		return;
	}
	child = DInode_New();
	if( ( res = DInode_SubInode( self, path, 1, child, 0 ) ) != 0 ){ // !!!
		char errbuf[MAX_ERRMSG];
		DInode_Delete( child );
		if( res == -1 )
			strcpy( errbuf, "Invalid directory name (EINVAL)" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, (void*)child, daox_type_dir );
	FreeTChars( path );
}

static void FSNode_Exists( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	if( self->type != 0 ){
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
		FreeTChars( path );
		return;
	}
	child = DInode_New();
	DaoProcess_PutEnum( proc, DInode_SubInode( self, path, 0, child, 1 ) == 0? "true" : "false" ); // !!!
	FreeTChars( path );
	DInode_Delete( child );
}

static void FSNode_Child( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	char_t buf[MAX_PATH + 1];
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
		goto Exit;
	}
	child = DInode_New();
	tcscpy( buf, self->path );
	if ( buf[tcslen( buf ) - 1] != T('/') )
		tcscat( buf, T("/") );
	if( tcslen( buf ) + tcslen( path ) > MAX_PATH ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, ENAMETOOLONG, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		DInode_Delete( child );
		goto Exit;
	}
	tcscat( buf, path );
	if( ( res = DInode_Open( child, buf ) ) != 0 ){
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		DInode_Delete( child );
		if( res == -1 )
			strcpy( errbuf, "File object is not a directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		snprintf( errbuf + strlen( errbuf ), sizeof(errbuf), ": %"T_FMT";", buf );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		return;                                  
	}
	DaoProcess_PutCdata( proc, (void*)child, child->type == 0? daox_type_dir : daox_type_file );
Exit:
	FreeTChars( buf );
}

static void DInode_Children( DInode *self, DaoProcess *proc, int type, DString *pat, int ft )
{
	DaoList *list = DaoProcess_PutList( proc );
	char buffer[MAX_PATH + 1], *filter;
	int res, i, j, len;
	DString *strbuf;
	DaoRegex *pattern;
	if( self->type != 0 ){
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
		return;
	}
	filter = DString_GetData( pat );
	len = strlen( filter );
	if( len > MAX_PATH ){
		DaoProcess_RaiseError( proc, fserr, "Filter is too large" );
		return;
	}
	if( ft == 0 ){
		buffer[0] = '^';
		buffer[1] = '(';
		for( i = 0, j = 2; i < len && j < MAX_PATH - 1; i++, j++ )
			switch( filter[i] )
			{
			case '?':
				buffer[j] = '.';
				break;
			case '*':
				buffer[j++] = '.';
				buffer[j] = '*';
				break;
			case ';':
				buffer[j] = '|';
				break;
			case '{':
			case '}':
			case '(':
			case ')':
			case '%':
			case '.':
			case '|':
			case '$':
			case '^':
			case '[':
			case ']':
			case '+':
			case '-':
			case '<':
			case '>':
				buffer[j++] = '%';
				buffer[j] = filter[i];
				break;
			default:
				buffer[j] = filter[i];
			}
		if( j >= MAX_PATH - 1 ){
			DaoProcess_RaiseError( proc, fserr, "Filter is too large" );
			return;
		}
		buffer[j] = ')';
		buffer[j + 1] = '$';
		buffer[j + 2] = '\0';
	}
	else
		strcpy( buffer, filter );
	strbuf = DString_New();
	DString_SetChars( strbuf, buffer );
	pattern = DaoProcess_MakeRegex( proc, strbuf );
    DString_Delete( strbuf );
    if( !pattern )
    	return;
	if( ( res = DInode_ChildrenRegex( self, type, proc, list, pattern ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		GetErrorMessage( errbuf, res, 1 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		return;
	}
}

static void FSNode_Children( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *pt = DString_Copy( p[1]->xString.value );
	DInode_Children( self, proc, 2, pt, DaoValue_TryGetEnum( p[2] ) );
	DString_Delete( pt );
}

static void FSNode_Files( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *pt = DString_Copy( p[1]->xString.value );
	DInode_Children( self, proc, 1, pt, DaoValue_TryGetEnum( p[2] ) );
	DString_Delete( pt );
}

static void FSNode_Dirs( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *pt = DString_Copy( p[1]->xString.value );
	DInode_Children( self, proc, 0, pt, DaoValue_TryGetEnum( p[2] ) );
	DString_Delete( pt );
}

static void FSNode_Suffix( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char_t *pos = tcsrchr( self->path, T('.') );
	DaoProcess_PutTChars( proc, pos && !tcschr( pos + 1, T('/') )? pos + 1 : T("") );
}

static void FSNode_Copy( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char_t *path = CharsToTChars( p[1]->xString.value->chars );
	FILE *src = NULL, *dest = NULL;
	DInode *copy;
	size_t len;
	int res;
	char buf[4096];
	if ( self->type == 0 ){
		DaoProcess_RaiseError( proc, fserr, "Copying of directories is not supported" );
		goto Exit;
	}
	src = fopen( self->path, T("r") );
	if ( !src ){
		char errbuf[MAX_ERRMSG] = "Unable to read file; ";
		GetErrorMessage( errbuf + strlen( errbuf ), errno, 0 );
		DaoProcess_RaiseError( proc, fserr, buf );
		goto Exit;
	}
	len = tcslen( path );
	if ( IS_PATH_SEP( path[len - 1] ) ){
		int i;
		size_t slen = tcslen( self->path );
		for (i = slen - 1; i > 0; i--)
			if( IS_PATH_SEP( self->path[i] ) )
				break;
		if ( len + ( slen - i ) > MAX_PATH ){
			DaoProcess_RaiseError( proc, fserr, "Resulting file name is too large" );
			goto Exit;
		}
		tcscpy( path + len, self->path + i );
	}
	dest = fopen( path, T("w") );
	if ( !dest ){
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		snprintf( errbuf, sizeof(errbuf), "Unable to write file: %"T_FMT";", path );
		GetErrorMessage( errbuf + strlen( errbuf ), errno, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		goto Exit;
	}
	while ( !feof( src ) ){
		size_t count = fread( buf, sizeof(char), sizeof(buf), src );
		fwrite( buf, sizeof(char), count, dest );
	}
	copy = DInode_New();
	if ( ( res = DInode_Open( copy, path ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		DInode_Delete( copy );
		GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
		goto Exit;
	}
	DaoProcess_PutCdata( proc, copy, daox_type_file );
Exit:
	FreeTChars( path );
	if ( src ) fclose( src );
	if ( dest ) fclose( dest );
}

static void FSNode_Owner( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *name = DString_New();
	int res = DInode_GetOwner( self, name );
	if ( res )
		DaoProcess_RaiseError( proc, fserr, "Unable to get information on file owner" );
	else
		DaoProcess_PutString( proc, name );
	DString_Delete( name );
}

static void FSNode_Mktemp( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char_t *pref = CharsToTChars( p[1]->xString.value->chars );
	char_t buf[MAX_PATH + 1];
	int res;
	if ( self->type != 0 )
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
	else {
		res = MakeTmpFile( self->path, pref, buf );
		DInode *fsnode = DInode_New();
		if ( res || ( res = DInode_Open( fsnode, buf ) ) != 0 ){
			char errbuf[MAX_ERRMSG];
			DInode_Delete( fsnode );
			if ( res == -1 )
				strcpy( errbuf, "Resulting file name is too large" );
			else if ( res == -2 )
				strcpy( errbuf, "Incorrect file prefix" );
			else
				GetErrorMessage( errbuf, errno, 0 );
			DaoProcess_RaiseError( proc, fserr, errbuf );
		}
		else
			DaoProcess_PutCdata( proc, fsnode, daox_type_file );
	}
	FreeTChars( pref );
}

static void FSNode_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = DInode_New();
	int res;
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	if( ( res = DInode_Open( fsnode, path ) ) != 0 ){
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		DInode_Delete( fsnode );
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, (void*)fsnode, fsnode->type == 0? daox_type_dir : daox_type_file );
	FreeTChars( path );
}

static void FS_CWD( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t buf[MAX_PATH + 1];
	int res = 0;
	DInode *fsnode = DInode_New();
	FS_TRANS( res = getcwd( buf, MAX_PATH ) != NULL );
	if( !res || ( res = DInode_Open( fsnode, buf ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		DInode_Delete( fsnode );
		GetErrorMessage( errbuf, ( res == 0 )? errno : res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_dir );
}

static void FS_SetCWD( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = (DInode*)DaoValue_TryGetCdata( p[0] );
	int res;
	FS_TRANS( res = chdir( fsnode->path ) );
	if( res ){
		char errbuf[MAX_PATH + 1];
		GetErrorMessage( errbuf, errno, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
}

static void FS_SetCWD2( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	int res;
	DInode *fsnode = DInode_New();
	if( ( res = DInode_Open( fsnode, path ) ) == 0 && fsnode->type == 0 ){
		FS_TRANS( res = chdir( fsnode->path ) );
		if ( res ){
			char errbuf[MAX_ERRMSG + MAX_PATH + 3];
			GetErrorMessage( errbuf, errno, 0 );
			DaoProcess_RaiseError( proc, fserr, errbuf );
		}
	}
	else {
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else if ( res )
			GetErrorMessage( errbuf, errno, 0 );
		else
			strcpy( errbuf, "File object is not a directory" );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	FreeTChars( path );
	DInode_Delete( fsnode );
}

static void FS_NormPath( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	int res;
	DInode *fsnode = DInode_New();
	if ( ( res = DInode_Open( fsnode, path ) ) == 0 )
		DaoProcess_PutTChars( proc, fsnode->path );
	else {
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		GetErrorMessage( errbuf, res, 0 );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	DInode_Delete( fsnode );
	FreeTChars( path );
}

static void FS_Exists( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	DInode *fsnode = DInode_New();
	DaoProcess_PutEnum( proc, DInode_Open( fsnode, path ) == 0? "true" : "false" );
	FreeTChars( path );
	DInode_Delete( fsnode );
}

static void FS_Roots( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoList *lst = DaoProcess_PutList( proc );
	if ( !GetRootDirs( lst ) )
		DaoProcess_RaiseError( proc, fserr, "Failed to obtain the list of root directories" );
}

static void FS_NewFile( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = DInode_New();
	int res;
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	if( ( res = DInode_Open( fsnode, path ) ) != 0 ){
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		DInode_Delete( fsnode );
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else if ( fsnode->type == 0 ){
		DInode_Delete( fsnode );
		DaoProcess_RaiseError( proc, fserr, "File object is not a file" );
	}
	else
		DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_file );
	FreeTChars( path );
}

static void FS_NewDir( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = DInode_New();
	int res;
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	if( ( res = DInode_Open( fsnode, path ) ) != 0 ){
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		DInode_Delete( fsnode );
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else if ( fsnode->type == 1 ){
		DInode_Delete( fsnode );
		DaoProcess_RaiseError( proc, fserr, "File object is not a directory" );
	}
	else
		DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_dir );
	FreeTChars( path );
}

static void FS_ListDir( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoList *lst = DaoProcess_PutList( proc );
	int res = DInode_ChildrenRegex( fsnode, 2, proc, lst, NULL );
	if( res ){
		char errbuf[MAX_PATH + 1];
		GetErrorMessage( errbuf, errno, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
}

static void FS_ListDir2( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t *path = CharsToTChars( p[0]->xString.value->chars );
	int res;
	DaoList *lst = DaoProcess_PutList( proc );
	DInode *fsnode = DInode_New();
	if( ( res = DInode_Open( fsnode, path ) ) == 0 && fsnode->type == 0 ){
		res = DInode_ChildrenRegex( fsnode, 2, proc, lst, NULL );
		if ( res ){
			char errbuf[MAX_ERRMSG + MAX_PATH + 3];
			GetErrorMessage( errbuf, errno, 0 );
			DaoProcess_RaiseError( proc, fserr, errbuf );
		}
	}
	else {
		char errbuf[MAX_ERRMSG + MAX_PATH + 3];
		if( res == -1 )
			strcpy( errbuf, "File object is not a file or directory" );
		else if ( res )
			GetErrorMessage( errbuf, errno, 0 );
		else
			strcpy( errbuf, "File object is not a directory" );
		if( res == -1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %"T_FMT, path );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	FreeTChars( path );
	DInode_Delete( fsnode );
}

static void FS_HomeDir( DaoProcess *proc, DaoValue *p[], int N )
{
	char_t buf[MAX_PATH + 1];
	int res = 0;
	DInode *fsnode = DInode_New();
#ifdef WIN32
	if ( SHGetFolderPathW( NULL, CSIDL_PERSONAL, NULL, 0, buf ) != S_OK ){
		DaoProcess_RaiseError( proc, fserr, "Failed to get home directory" );
		DInode_Delete( fsnode );
		return;
	}
#else
	strcpy( buf, "~" );
#endif
	if( ( res = DInode_Open( fsnode, buf ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		DInode_Delete( fsnode );
		GetErrorMessage( errbuf, ( res == 0 )? errno : res, 0 );
		DaoProcess_RaiseError( proc, fserr, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_dir );
}

static DaoFuncItem entryMeths[] =
{
	/*! Returns new \c entry bound to \a path of file or directory */
	{ FSNode_New,		"entry(path: string) => entry" },

	/*! Full path */
	{ FSNode_Path,		".path(invar self: entry) => string" },

	/*! Entry name (last component of path) */
	{ FSNode_Name,		".name(invar self: entry) => string" },

	/*! Base name (up to, but not including, the first '.' in name) */
	{ FSNode_BaseName,	".basename(invar self: entry) => string" },

	/*! Name part after the last '.' */
	{ FSNode_Suffix,	".suffix(invar self: entry) => string" },

	/*! File object kind: file or directory */
	{ FSNode_Type,		".kind(invar self: entry ) => enum<file, dir>" },

	/*! Directory which contains this entry */
	{ FSNode_Parent,	".dirup(invar self: entry)=> dir|none" },

	/*! Time of creation, last modification and access (use \c time module to operate it) */
	{ FSNode_Time,		".time(invar self: entry) => tuple<created: int, modified: int, accessed: int>" },

	/*! Owner name */
	{ FSNode_Owner,		".owner(invar self: entry) => string" },

	/*! Access mode as a combination of 'r', 'w' and 'x' flags. On Windows, only permissions for the current user are affected */
	{ FSNode_Access,	".access(invar self: entry) => tuple<user: string, group: string, other: string>" },
	{ FSNode_SetAccess,	".access=(self: entry, value: tuple<user: string, group: string, other: string>)" },

	/*! Moves (renames) entry within the file system so that its full path becomes \a path. \a path may end with directory separator,
	 * omitting the entry name, in which case the current name is assumed */
	{ FSNode_Rename,	"move(self: entry, path: string)" },

	/*! Deletes file or empty directory
	 *
	 * \note Doing this does not invalidate the entry */
	{ FSNode_Remove,	"delete(self: entry)" },

	/*! Re-reads all entry attributes */
	{ FSNode_Update,	"refresh(self: entry)" },
	{ NULL, NULL }
};

static DaoFuncItem fileMeths[] =
{
	/*! Returns \a file object bound to \a path if it points to a file, otherwise raises exception */
	{ FS_NewFile,		"file(path: string) => file" },

	/*! Size of the file in bytes */
	{ FSNode_Size,		".size(invar self: file) => int" },

	/*! Resizes the file to the given \a size */
	{ FSNode_Resize,	".size=(self: file, size: int)" },

	/*! Copies the file and returns \c file object of its copy */
	{ FSNode_Copy,		"copy(self: file, path: string) => file" },
	{ NULL, NULL }
};

static DaoFuncItem dirMeths[] =
{
	/*! Returns \c dir object bount to \a path if it points to a directory, otherwise raises exception */
	{ FS_NewDir,		"dir(path: string) => dir" },

	/*! Creates new file given relative \a path and returns its \c file object */
	{ FSNode_Makefile,	"mkfile(self: dir, path: string) => file" },

	/*! Creates new directory given relative \a path and returns its \c dir object */
	{ FSNode_Makedir,	"mkdir(self: dir, path: string) => dir" },

	/*! Returns the list of inner entries with names matching \a filter,
	 * where \a filter type is defined by \a filtering and can be either a wildcard pattern or Dao string pattern */
	{ FSNode_Children,	"entries(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<entry>" },

	/*! Returns the list of inner files with names matching \a filter,
	 * where \a filter type is defined by \a filtering and can be either a wildcard pattern or Dao string pattern */
	{ FSNode_Files,		"files(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<file>" },

	/*! Returns the list of inner directories with names matching \a filter,
	 * where \a filter type is defined by \a filtering and can be either a wildcard pattern or Dao string pattern */
	{ FSNode_Dirs,		"dirs(invar self: dir, filter = '*', filtering: enum<wildcard,pattern> = $wildcard) => list<dir>" },

	/*! Returns sub-entry given its relative \a path */
	{ FSNode_Child,		"[](invar self: dir, path: string) => entry" },

	/*! Returns \c true if sub-entry specified by relative \a path exists */
	{ FSNode_Exists,	"exists(invar self: dir, path: string) => bool" },

	/*! Creates file with unique name prefixed by \a prefix in this directory. Returns the corresponding entry */
	{ FSNode_Mktemp,	"mktemp(self: dir, prefix = '') => file" },
	{ NULL, NULL }
};

/*! \brief Provides platform-independent interface for manipulating files and directories.
 *
 * \c entry represents a generic file system object, namely a file or directory (links and other special types of file-like objects are
 * not supported). \c entry is inherited by \c file and \c dir, which provide operations specific to files and directories accordingly.
 *
 * A file object is operated by its name, no descriptors or locks are kept for the lifetime of the associated \c entry. File attributes
 * are cached and are only updated when necessary (e.g., \c resize() will update the size attribute); use \c refresh() to re-read them.
 *
 * \c entry uses '/' as the unified path separator on all platforms, Windows paths (including UNC) are automatically normalized to
 * this form. Relative paths and symbolic links are automatically expanded to their absolute form.
 *
 * \note On Windows, all path strings are assumed to be encoded in UTF-8, and are implicitly converted to UTF-16 in order to support
 * Unicode file names on this platform.
 */
DaoTypeBase entryTyper = {
	"entry", NULL, NULL, entryMeths, {NULL}, {0}, (FuncPtrDel)DInode_Delete, NULL
};

DaoTypeBase fileTyper = {
	"file", NULL, NULL, fileMeths, {&entryTyper, NULL}, {0}, (FuncPtrDel)DInode_Delete, NULL
};

DaoTypeBase dirTyper = {
	"dir", NULL, NULL, dirMeths, {&entryTyper, NULL}, {0}, (FuncPtrDel)DInode_Delete, NULL
};

static DaoFuncItem fsMeths[] =
{
	/*! Returns the current working directory */
	{ FS_CWD,		"cwd() => dir" },

	/*! Makes \a dir the current working directory */
	{ FS_SetCWD,	"cd(invar path: dir)" },
	{ FS_SetCWD2,	"cd(path: string)" },

	/*! Returns list of names of all file objects in the directory specified by \a path */
	{ FS_ListDir,	"ls(invar path: dir) => list<string>" },
	{ FS_ListDir2,	"ls(path = '.') => list<string>" },

	/*! Returns absolute form of \a path, which must point to an existing file or directory. On Windows, replaces all '\' in path
	 * with '/' */
	{ FS_NormPath,	"realpath(path: string) => string" },

	/*! Returns \c true if \a path exists and points to a file or directory */
	{ FS_Exists,	"exists(path: string) => bool" },

	/*! On Windows, returns list of root directories (drives). On other systems returns {'/'} */
	{ FS_Roots,		"roots() => list<string>" },

	/*! Returns home directory for the current user (on Windows, 'Documents' directory is assumed) */
	{ FS_HomeDir,	"home() => dir" },
	{ NULL, NULL }
};

DAO_DLL int DaoFS_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *fsns;
	FS_INIT();
	fsns = DaoVmSpace_GetNamespace( vmSpace, "fs" );
	DaoNamespace_AddConstValue( ns, "fs", (DaoValue*)fsns );
	daox_type_entry = DaoNamespace_WrapType( fsns, & entryTyper, 1 );
	daox_type_file = DaoNamespace_WrapType( fsns, & fileTyper, 1 );
	daox_type_dir = DaoNamespace_WrapType( fsns, & dirTyper, 1 );
	DaoNamespace_WrapFunctions( fsns, fsMeths );
	return 0;
}
