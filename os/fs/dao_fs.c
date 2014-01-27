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
/* 2011-02-13, Aleksey Danilov: added initial implementation. */

#include"string.h"
#include"errno.h"
#include<sys/stat.h>

#include"dao.h"
#include"daoValue.h"

#ifdef WIN32

#include"io.h"
#include<lmcons.h>
#include<aclapi.h>
#ifdef _MSC_VER
#define chdir _chdir
#define rmdir _rmdir
#define getcwd _getcwd
#define mkdir _mkdir
#define stat _stat
#define chmod _chmod
#endif

#else
#include"dirent.h"
#endif

#ifdef UNIX
#include<unistd.h>
#include<sys/time.h>
#include<pwd.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#define MAX_ERRMSG 100

struct DInode
{
	char *path;
	short type;
	time_t ctime;
	time_t mtime;
	mode_t mode;
	size_t size;
#ifndef WIN32
	uid_t uid;
#endif
};

typedef struct DInode DInode;

DaoType *daox_type_fsnode = NULL;

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
#define IS_PATH_SEP( c ) ( ( c ) == '\\' || ( c ) == '/' || ( c ) == ':' )
#define STD_PATH_SEP "\\"
#else
#define IS_PATH_SEP( c ) ( ( c ) == '/' )
#define STD_PATH_SEP "/"
#endif

int DInode_Open( DInode *self, const char *path )
{
	char buf[MAX_PATH + 1] = {0};
	struct stat info;
	int len, i;
	DInode_Close( self );
	if( !path )
		return 1;
	len = strlen( path );
	if( stat( path, &info ) != 0 )
		return errno;
	for( i = 0; i < len; i++ )
		if( path[i] == '.' && ( i == 0 || IS_PATH_SEP( path[i - 1] ) )
			&& ( i == len - 1 || IS_PATH_SEP( path[i + 1] ) ||
			( path[i + 1] == '.' && ( i == len - 2 || IS_PATH_SEP( path[i + 2] ) ) ) ) )
			return -1;
#ifdef WIN32
	if( !( info.st_mode & _S_IFDIR ) && !( info.st_mode & _S_IFREG ) )
		return 1;
	self->type = ( info.st_mode & _S_IFDIR )? 0 : 1;
	self->size = ( info.st_mode & _S_IFDIR )? 0 : info.st_size;
	if( len < 2 || ( path[1] != ':' && path[0] != '\\' ) ){
		if( !getcwd( buf, MAX_PATH ) )
			return errno;
		strcat( buf, "\\" );
	}
#else
	if( !S_ISDIR( info.st_mode ) && !S_ISREG( info.st_mode ) )
		return 1;
	self->type = ( S_ISDIR( info.st_mode ) )? 0 : 1;
	self->size = ( S_ISDIR( info.st_mode ) )? 0 : info.st_size;
	self->uid = info.st_uid;
	if( path[0] != '/' ){
		if( !getcwd( buf, MAX_PATH ) )
			return errno;
		strcat( buf, "/" );
	}
#endif
	len += strlen( buf );
	if( len > MAX_PATH )
		return ENAMETOOLONG;
	self->path = (char*)dao_malloc( len + 1 );
	strcpy( self->path, buf );
	strcat( self->path, path );
#ifndef WIN32
	if( self->path[len - 1] == '/' && len > 1 )
		self->path[len - 1] = '\0';
#endif
	self->ctime = info.st_ctime;
	self->mtime = info.st_mtime;
	self->mode = info.st_mode;
	return 0;
}

int DInode_Reopen( DInode *self )
{
	struct stat info;
	if( stat( self->path, &info ) != 0 )
		return errno;
#ifdef WIN32
	if( !( info.st_mode & _S_IFDIR ) && !( info.st_mode & _S_IFREG ) )
		return 1;
	self->type = ( info.st_mode & _S_IFDIR )? 0 : 1;
	self->size = ( info.st_mode & _S_IFDIR )? 0 : info.st_size;
#else
	if( !S_ISDIR( info.st_mode ) && !S_ISREG( info.st_mode ) )
		return 1;
	self->type = ( S_ISDIR( info.st_mode ) )? 0 : 1;
	self->size = ( S_ISDIR( info.st_mode ) )? 0 : info.st_size;
	self->uid = info.st_uid;
#endif
	self->ctime = info.st_ctime;
	self->mtime = info.st_mtime;
	self->mode = info.st_mode;
	return 0;
}

char* DInode_Parent( DInode *self, char *buffer )
{
	int i;
	if( !self->path )
		return NULL;
	for (i = strlen( self->path ) - 1; i >= 0; i--)
		if( IS_PATH_SEP( self->path[i] ) )
			break;
	if( self->path[i + 1] == '\0' )
		return NULL;
#ifdef WIN32
	if( self->path[i] == ':' ){
		strncpy( buffer, self->path, i + 1 );
		buffer[i + 1] = '\\';
		buffer[i + 2] = '\0';
	}
	else{
		if( i == 2 )
			i++;
		strncpy( buffer, self->path, i );
		if( self->path[0] == '\\' ){
			int j;
			int k = 0;
			for (j = 2; j < i && k < 2; j++)
				if( IS_PATH_SEP( self->path[j] ) )
					k++;
			if( i == j ){
				buffer[i] = '\\';
				i++;
			}
		}
		buffer[i] = '\0';
	}
#else
	if( i == 0 )
		strcpy( buffer, "/" );
	else{
		strncpy( buffer, self->path, i );
		buffer[i] = '\0';
	}
#endif
	return buffer;
}

int DInode_Rename( DInode *self, const char *path )
{
	char buf[MAX_PATH + 1] = {0};
	int i, len, noname = 0;
	if( !self->path )
		return 1;
	len = strlen( path );
	if ( IS_PATH_SEP( path[len - 1] ) )
		noname = 1;
	for( i = 0; i < len; i++ )
		if( path[i] == '.' && ( i == 0 || IS_PATH_SEP( path[i - 1] ) )
			&& ( i == len - 1 || IS_PATH_SEP( path[i + 1] ) ||
			( path[i + 1] == '.' && ( i == len - 2 || IS_PATH_SEP( path[i + 2] ) ) ) ) )
			return -1;
	if( !DInode_Parent( self, buf ) )
		return 1;
#ifdef WIN32
	if( len < 2 || ( path[1] != ':' && path[0] != '\\' ) ){
#else
	if( path[0] != '/' ){
#endif
		strcat( buf, STD_PATH_SEP );
		len += strlen( buf );
		if( len > MAX_PATH )
			return ENAMETOOLONG;
		strcat( buf, path );
	}else{
		if( len > MAX_PATH )
			return ENAMETOOLONG;
		strcpy( buf, path );
	}
	if ( noname ){
		int i;
		for (i = strlen( self->path ) - 1; i > 0; i--)
			if( IS_PATH_SEP( self->path[i] ) )
				break;
		strcat( buf, STD_PATH_SEP );
		strcat( buf, self->path + i );
	}
	if( rename( self->path, buf ) != 0 )
		return errno;
	self->path = (char*)dao_realloc( self->path, len + 1 );
	strcpy( self->path, buf );
	return 0;
}

int DInode_Remove( DInode *self )
{
	if( !self->path )
		return 1;
	if( self->type == 0 ){
		if( rmdir( self->path ) != 0 )
			return errno;
	}else{
		if( unlink( self->path ) != 0 )
			return errno;
	}
	return 0;
}

int DInode_Append( DInode *self, const char *path, int dir, DInode *dest )
{
	int i, len;
	char buf[MAX_PATH + 1];
	FILE *handle;
	struct stat info;
	if( !self->path || self->type != 0 || !dest )
		return 1;
	len = strlen( path );
	for( i = 0; i < len; i++ )
		if( path[i] == '.' && ( i == 0 || IS_PATH_SEP( path[i - 1] ) )
			&& ( i == len - 1 || IS_PATH_SEP( path[i + 1] ) ||
			( path[i + 1] == '.' && ( i == len - 2 || IS_PATH_SEP( path[i + 2] ) ) ) ) )
			return -1;
	if( DInode_Parent( self, buf ) ){
		strcpy( buf, self->path );
		strcat( buf, STD_PATH_SEP );
	}
	else
		strcpy( buf, self->path );
	if( strlen( buf ) + len > MAX_PATH )
		return ENAMETOOLONG;
	strcat( buf, path );
#ifdef WIN32
	if( stat( buf, &info ) == 0 && ( ( dir && ( info.st_mode & _S_IFDIR ) )
		|| ( !dir && ( info.st_mode & _S_IFREG ) ) ) )
		return DInode_Open( dest, buf );
#else
	if( stat( buf, &info ) == 0 && ( ( dir && S_ISDIR( info.st_mode ) )
		|| ( !dir && S_ISREG( info.st_mode ) ) ) )
		return DInode_Open( dest, buf );
#endif
	if( !dir ){
		if( !( handle = fopen( buf, "w" ) ) )
			return ( errno == EINVAL ) ? 1 : errno;
		fclose( handle );
	}else{
#ifdef WIN32
	if( mkdir( buf ) != 0 )
		return ( errno == EINVAL ) ? 1 : errno;
#else
	if( mkdir( buf, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH ) != 0 )
		return ( errno == EINVAL ) ? 1 : errno;
#endif
	}
	return DInode_Open( dest, buf );
}

extern DaoTypeBase fsnodeTyper;

int DInode_ChildrenRegex( DInode *self, int type, DaoProcess *proc, DaoList *dest, DaoRegex *pattern )
{
	char buffer[MAX_PATH + 1];
	int len, res;
#ifdef WIN32
	intptr_t handle;
	struct _finddata_t finfo;
#else
	DIR *handle;
	struct dirent *finfo;
#endif
	if( !self->path || self->type != 0 )
		return 1;
    strcpy( buffer, self->path );
	len = strlen( buffer );
#ifdef WIN32
	/* Using _findfirst/_findnext for Windows */
	if( IS_PATH_SEP( buffer[len - 1] ) )
    	strcpy( buffer + len, "*" );
    else
		strcpy( buffer + len++, "\\*" );
	handle = _findfirst( buffer, &finfo );
	if (handle != -1){
		DString *str = DString_New( 1 );
		DaoValue *value;
		DInode *fsnode;
		do
			if( strcmp( finfo.name, "." ) && strcmp( finfo.name, ".." ) ){
				DString_SetDataMBS( str, finfo.name, strlen(finfo.name) );
				strcpy( buffer + len, finfo.name );
				fsnode = DInode_New();
				if( ( res = DInode_Open( fsnode, buffer ) ) != 0 ){
					DInode_Delete( fsnode );
					return res;
				}
				if( ( fsnode->type == type || type == 2 ) && DaoRegex_Match( pattern, str, NULL, NULL ) ){
					value = (DaoValue*) DaoProcess_NewCdata( proc, daox_type_fsnode, fsnode, 1 );
					DaoList_PushBack( dest, value );
				}
				else
					DInode_Delete( fsnode );
			}
		while( !_findnext( handle, &finfo ) );
		DString_Delete( str );
		_findclose( handle );
	}
#else
	/* Using POSIX opendir/readdir otherwise */
	handle = opendir( buffer );
	if( !IS_PATH_SEP( buffer[len - 1] ) )
		strcpy( buffer + len++,  "/");
	if( handle ){
		DString *str = DString_New( 1 );
		DaoValue *value;
		DInode *fsnode;
		while( ( finfo = readdir( handle ) ) )
			if( strcmp( finfo->d_name, "." ) && strcmp( finfo->d_name, ".." ) ){
				DString_SetDataMBS( str, finfo->d_name, strlen(finfo->d_name) );
				strcpy( buffer + len, finfo->d_name );
				fsnode = DInode_New();
				if( ( res = DInode_Open( fsnode, buffer ) ) != 0 ){
					DInode_Delete( fsnode );
					return res;
				}
				if( ( fsnode->type == type || type == 2 ) && DaoRegex_Match( pattern, str, NULL, NULL ) ){
					value = (DaoValue*) DaoProcess_NewCdata( proc, daox_type_fsnode, fsnode, 1 );
					DaoList_PushBack( dest, value );
				}
				else
					DInode_Delete( fsnode );
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
#ifdef WIN32
	if ( self->mode & _S_IREAD )
		DString_AppendMBS( res->items[0]->xString.data, "r" );
	if ( self->mode & _S_IWRITE )
		DString_AppendMBS( res->items[0]->xString.data, "w" );
	if ( self->mode & _S_IEXEC )
		DString_AppendMBS( res->items[0]->xString.data, "x" );
#else
	if ( self->mode & S_IRUSR )
		DString_AppendMBS( res->items[0]->xString.data, "r" );
	if ( self->mode & S_IWUSR )
		DString_AppendMBS( res->items[0]->xString.data, "w" );
	if ( self->mode & S_IXUSR )
		DString_AppendMBS( res->items[0]->xString.data, "x" );

	if ( self->mode & S_IRGRP )
		DString_AppendMBS( res->items[1]->xString.data, "r" );
	if ( self->mode & S_IWGRP )
		DString_AppendMBS( res->items[1]->xString.data, "w" );
	if ( self->mode & S_IXGRP )
		DString_AppendMBS( res->items[1]->xString.data, "x" );

	if ( self->mode & S_IROTH )
		DString_AppendMBS( res->items[2]->xString.data, "r" );
	if ( self->mode & S_IWOTH )
		DString_AppendMBS( res->items[2]->xString.data, "w" );
	if ( self->mode & S_IXOTH )
		DString_AppendMBS( res->items[2]->xString.data, "x" );
#endif
}

int DInode_GetOwner( DInode *self, DString *name )
{
#ifdef WIN32
	PSID sid = NULL;
	SID_NAME_USE sidname;
	PSECURITY_DESCRIPTOR sd = NULL;
	char buf[UNLEN], dbuf[UNLEN];
	DWORD bufsize = sizeof(buf), sdsize = sizeof(dbuf);
	if ( GetNamedSecurityInfo( self->path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &sid, NULL, NULL, NULL, &sd ) != ERROR_SUCCESS )
		return 1;
	if ( !LookupAccountSid( NULL, sid, buf, &bufsize, dbuf, &sdsize, &sidname ) ){
		LocalFree( sd );
		return 1;
	}
	DString_SetMBS( name, buf );
	LocalFree( sd );
	return 0;
#else
	struct passwd pwd, *res;
	char buf[16384];
	if ( getpwuid_r( self->uid, &pwd, buf, sizeof(buf), &res ) )
		return 1;
	DString_SetMBS( name, pwd.pw_name );
	return 0;
#endif
}

static void GetErrorMessage( char *buffer, int code, int special )
{
	switch ( code ){
	case -1:
		strcpy( buffer, "'.' and '..' entries in path are not allowed" );
		break;
	case EACCES:
	case EBADF:
		strcpy( buffer, "Access not permitted (EACCES/EBADF)");
		break;
	case EBUSY:
		strcpy (buffer, "The fsnode's path is being used (EBUSY)" );
		break;
	case ENOTEMPTY:
	case EEXIST:
		strcpy( buffer, special? "The directory is not empty (ENOTEMPTY/EEXIST)" : "The fsnode already exists (EEXIST/ENOTEMPTY)" );
		break;
	case EPERM:
	case ENOTDIR:
	case EISDIR:
		strcat( buffer, "Inconsistent type of the file object(s) (EPERM/ENOTDIR/EISDIR)" );
		break;
	case EINVAL:
		strcpy( buffer, special? "The fsnode's path does not exist (EINVAL)" : "Trying to make the directory its own subdirectory (EINVAL)" );
		break;
	case EMLINK:
		strcat( buffer, "Trying to create too many entries in the parent directory (EMLINK)" );
		break;
	case ENOENT:
		strcpy( buffer, "The fsnode's path does not exist (ENOENT)" );
		break;
	case ENOSPC:
		strcpy( buffer, "No space for a new entry in the file system (ENOSPC)" );
		break;
	case EROFS:
		strcpy( buffer, "Trying to write to a read-only file system (EROFS)" );
		break;
	case EXDEV:
		strcpy( buffer, "Trying to relocate the fsnode to a different file system (EXDEV)" );
		break;
	case ENAMETOOLONG:
		strcpy( buffer, "The fsnode's path is too long (ENAMETOOLONG)" );
		break;
	case EMFILE:
	case ENFILE:
		strcpy( buffer, "Too many files open (EMFILE/ENFILE)" );
		break;
	case ENOMEM:
		strcpy( buffer, "Not enough memory (ENOMEM)" );
		break;
	default:
		sprintf( buffer, "Unknown system error (%x)", code );
	}
}

static void FSNode_Update( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char errbuf[MAX_ERRMSG + MAX_PATH + 3];
	int res;
	if( ( res = DInode_Reopen( self ) ) != 0 ){
		if( res == 1 )
			strcpy( errbuf, "Trying to open something which is not a file/directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == 1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %s", self->path );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
	}
}

static void FSNode_Path( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutMBString( proc, self->path );
}

static void FSNode_Name( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	int i;
	for (i = strlen( self->path ) - 1; i >= 0; i--)
		if( IS_PATH_SEP( self->path[i] ) )
			break;
	if( self->path[i + 1] == '\0' )
		DaoProcess_PutMBString( proc, self->path );
	else
		DaoProcess_PutMBString( proc, self->path + i + 1 );
}

static void FSNode_Parent( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *par;
	char path[MAX_PATH + 1];
	int res = 0;
	par = DInode_New();
	if( !DInode_Parent( self, path ) || ( res = DInode_Open( par, path ) ) != 0 ){
		DInode_Delete( par );
		if( res == 0 )
			strcpy( path, "The fsnode has no parent" );
		else
			GetErrorMessage( path, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, path );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)par, daox_type_fsnode );
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

static void FSNode_Rename( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	int res;
	char errbuf[MAX_ERRMSG];
	if ( (res = DInode_Rename( self, DaoValue_TryGetMBString( p[1] ) ) ) != 0 ){
		if( res == 1 )
			strcpy( errbuf, "Trying to rename root fsnode" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
}

static void FSNode_Remove( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	int res;
	char errbuf[MAX_ERRMSG];
	if ( (res = DInode_Remove( self ) ) != 0 ){
		GetErrorMessage( errbuf, res, self->type == 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
}

static void FSNode_Ctime( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->ctime );
}

static void FSNode_Mtime( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->mtime );
}

static void FSNode_Access( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DaoTuple *res = DaoProcess_PutTuple( proc, 3 );
	DInode_GetMode( self, res );
}

int InitAccessBits( DString *mode, int *r, int *w, int *x )
{
	daoint i;
	DString_ToMBS( mode );
	*r = *w = *x = 0;
	for ( i = 0; i < mode->size; i++ )
		switch ( mode->mbs[i] ){
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
	DString *mode = DString_Copy( p[1]->xString.data );
	if ( !InitAccessBits( mode, &ur, &uw, &ux ) )
		goto Error;
	DString_Assign( mode, p[2]->xString.data );
	if ( !InitAccessBits( mode, &gr, &gw, &gx ) )
		goto Error;
	DString_Assign( mode, p[3]->xString.data );
	if ( !InitAccessBits( mode, &otr, &otw, &otx ) )
		goto Error;
	int res = DInode_SetAccess(self, ur, uw, ux, gr, gw, gx, otr, otw, otx);
	if (res){
		GetErrorMessage(errbuf, res, 0);
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
	}
	else
		FSNode_Update(proc, p, N);
	DString_Delete( mode );
	return;
Error:
	DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid access mode format" );
	DString_Delete( mode );
}

static void FSNode_Makefile( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char errbuf[MAX_ERRMSG];
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The fsnode is not a directory" );
		return;
	}
	child = DInode_New();
	if( ( res = DInode_Append( self, DaoValue_TryGetMBString( p[1] ), 0, child ) ) != 0 ){
		DInode_Delete( child );
		if( res == 1 )
			strcpy( errbuf, "The fsnode's name is invalid (EINVAL)" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)child, daox_type_fsnode );
}

static void FSNode_Makedir( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char errbuf[MAX_ERRMSG];
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The fsnode is not a directory" );
		return;
	}
	child = DInode_New();
	if( ( res = DInode_Append( self, DaoValue_TryGetMBString( p[1] ), 1, child ) ) != 0 ){
		DInode_Delete( child );
		if( res == 1 )
			strcpy( errbuf, "The fsnode's name is invalid (EINVAL)" );
		else
			GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)child, daox_type_fsnode );
}

static void FSNode_Child( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode *child;
	char path[MAX_PATH + 1], *str;
	int res;
	if( self->type != 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The fsnode is not a directory" );
		return;
	}
	child = DInode_New();
	strcpy( path, self->path );
	strcat( path, STD_PATH_SEP );
	str = DaoValue_TryGetMBString( p[1] );
	if( strlen( path ) + strlen( str ) > MAX_PATH ){
		GetErrorMessage( path, ENAMETOOLONG, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, path );
		return;
	}
	strcat( path, str );
	if( ( res = DInode_Open( child, path ) ) != 0 ){
		DInode_Delete( child );
		if( res == 1 )
			strcpy( path, "The fsnode is not a directory" );
		else
			GetErrorMessage( path, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, path );
		return;                                  
	}
	DaoProcess_PutCdata( proc, (void*)child, daox_type_fsnode );
}

static void DInode_Children( DInode *self, DaoProcess *proc, int type, DString *pat, int ft )
{
	DaoList *list = DaoProcess_PutList( proc );
	char buffer[MAX_PATH + 1], *filter;
	int res, i, j, len;
	DString *strbuf;
	DaoRegex *pattern;
	if( self->type != 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The fsnode is not a directory" );
		return;
	}
	filter = DString_GetMBS( pat );
	len = strlen( filter );
	if( len > MAX_PATH ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The filter is too large" );
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
			DaoProcess_RaiseException( proc, DAO_ERROR, "The filter is too large" );
			return;
		}
		buffer[j] = ')';
		buffer[j + 1] = '$';
		buffer[j + 2] = '\0';
	}
	else
		strcpy( buffer, filter );
	strbuf = DString_New( 1 );
	DString_SetMBS( strbuf, buffer );
	pattern = DaoProcess_MakeRegex( proc, strbuf, 1 );
    DString_Delete( strbuf );
    if( !pattern )
    	return;
	type = ( type == 3 ) ? 2 : ( ( type == 1 ) ? 1 : 0 );
	if( ( res = DInode_ChildrenRegex( self, type, proc, list, pattern ) ) != 0 ){
		GetErrorMessage( buffer, res, 1 );
		DaoProcess_RaiseException( proc, DAO_ERROR, buffer );
		return;
	}
}

static void FSNode_Children( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode_Children( self, proc, DaoValue_TryGetEnum( p[1] ), DaoString_Get( DaoValue_CastString( p[2] ) ),
					 DaoValue_TryGetEnum( p[3] ) );
}

static void FSNode_Files( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode_Children( self, proc, 1, DaoString_Get( DaoValue_CastString( p[1] ) ), DaoValue_TryGetEnum( p[2] ) );
}

static void FSNode_Dirs( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DInode_Children( self, proc, 2, DaoString_Get( DaoValue_CastString( p[1] ) ), DaoValue_TryGetEnum( p[2] ) );
}

static void FSNode_Suffix( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	char *pos = strrchr( self->path, '.' );
	DaoProcess_PutMBString( proc, pos && !strstr( pos + 1, STD_PATH_SEP )? pos + 1 : "" );
}

static void FSNode_Copy( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *path = DString_Copy( p[1]->xString.data );
	DString_ToMBS( path );
	FILE *src, *dest;
	DInode *copy;
	int res;
	char buf[4096];
	if ( self->type == 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "Copying of directories is not supported" );
		goto Exit;
	}
	src = fopen( self->path, "r" );
	if ( !src ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "Unable to read from initial file" );
		goto Exit;
	}
	if ( IS_PATH_SEP( path->mbs[path->size - 1] ) ){
		int i;
		for (i = strlen( self->path ) - 1; i > 0; i--)
			if( IS_PATH_SEP( self->path[i] ) )
				break;
		DString_AppendMBS( path, STD_PATH_SEP );
		DString_AppendMBS( path, self->path + i );
	}
	dest = fopen( path->mbs, "w" );
	if ( !dest ){
		fclose( src );
		DaoProcess_RaiseException( proc, DAO_ERROR, "Unable to write to destination file" );
		goto Exit;
	}
	while ( !feof( src ) ){
		size_t count = fread( buf, sizeof(char), sizeof(buf), src );
		fwrite( buf, sizeof(char), count, dest );
	}
	fclose( src );
	fclose( dest );
	copy = DInode_New();
	if ( ( res = DInode_Open( copy, path->mbs ) ) != 0 ){
		char errbuf[MAX_ERRMSG];
		DInode_Delete( copy );
		GetErrorMessage( errbuf, res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
	}
	else
		DaoProcess_PutCdata( proc, copy, daox_type_fsnode );
Exit:
	DString_Delete( path );
}

static void FSNode_Owner( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *self = (DInode*)DaoValue_TryGetCdata( p[0] );
	DString *name = DString_New( 1 );
	int res = DInode_GetOwner( self, name );
	if ( res )
		DaoProcess_RaiseException( proc, DAO_ERROR, "Unaible to get information on file owner" );
	else
		DaoProcess_PutString( proc, name );
	DString_Delete( name );
}

static void FSNode_New( DaoProcess *proc, DaoValue *p[], int N )
{
	char errbuf[MAX_ERRMSG + MAX_PATH + 3];
	DInode *fsnode = DInode_New();
	int res;
	char *path = DaoValue_TryGetMBString( p[0] );
	if( ( res = DInode_Open( fsnode, path ) ) != 0 ){
		DInode_Delete( fsnode );
		if( res == 1 )
			strcpy( errbuf, "Trying to open something which is not a file/directory" );
		else
			GetErrorMessage( errbuf, res, 0 );
		if( res == 1 || res == ENOENT )
			snprintf( errbuf + strlen( errbuf ), MAX_PATH + 3, ": %s", path );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_fsnode );
}

static void FSNode_GetCWD( DaoProcess *proc, DaoValue *p[], int N )
{
	char buf[MAX_PATH + 1];
	int res = 0;
	DInode *fsnode = DInode_New();
	if( !getcwd( buf, MAX_PATH ) || ( res = DInode_Open( fsnode, buf ) ) != 0 ){
		DInode_Delete( fsnode );
		GetErrorMessage( buf, ( res == 0 )? errno : res, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, buf );
		return;
	}
	DaoProcess_PutCdata( proc, (void*)fsnode, daox_type_fsnode );
}

static void FSNode_SetCWD( DaoProcess *proc, DaoValue *p[], int N )
{
	DInode *fsnode = (DInode*)DaoValue_TryGetCdata( p[0] );
	char errbuf[MAX_PATH + 1];
	if( fsnode->type != 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "The fsnode is not a directory" );
		return;
	}
	if( chdir( fsnode->path ) != 0 ){
		GetErrorMessage( errbuf, errno, 0 );
		DaoProcess_RaiseException( proc, DAO_ERROR, errbuf );
		return;
	}
}

static DaoFuncItem fsnodeMeths[] =
{
	/*! Returns new fsnode given @path of file or directory
	 *	\note '.' and '..' entries in @path are not allowed */
	{ FSNode_New,      "fsnode( path : string )=>fsnode" },

	/*! Returns full path */
	{ FSNode_Path,     "path( self : fsnode )=>string" },

	/*! Returns base name */
	{ FSNode_Name,     "name( self : fsnode )=>string" },

	/*! Returns type of file object */
	{ FSNode_Type,     "type( self : fsnode )=>enum<file, dir>" },

	/*! Returns size of file (0 for directory) */
	{ FSNode_Size,     "size( self : fsnode )=>int" },

	/*! Returns rightmost path part following '.' */
	{ FSNode_Suffix,   "suffix( self: fsnode )=>string" },

	/*! Returns parent directory */
	{ FSNode_Parent,   "parent( self : fsnode )=>fsnode" },

	/*! Returns creation time */
	{ FSNode_Ctime,    "ctime( self : fsnode )=>int" },

	/*! Returns last modification time */
	{ FSNode_Mtime,    "mtime( self : fsnode )=>int" },

	/*! Returns owner name */
	{ FSNode_Owner,    "owner( self : fsnode )=>string" },

	/*! Returns access mode as a combination of 'r', 'w' and 'x'. On Windows, only permissions for the current user are returned */
	{ FSNode_Access,   "access( self : fsnode )=>tuple<user: string, group: string, other: string>" },

	/*! Sets access mode to @mode, where @mode is a combination of 'r', 'w' and 'x' */
	{ FSNode_SetAccess,"access( self : fsnode, user : string, group='', other='' )" },

	/*! Moves linked file object within the file system so that its full path becomes @path. @path may end with directory separator,
	 * omitting the file object name, in which case the current name is assumed
	 *	\note '.' and '..' entries in @path are not allowed */
	{ FSNode_Rename,   "move( self : fsnode, path : string )" },

	/*! Deletes linked file object
	 * \note Doing this does not invalidate the fsnode */
	{ FSNode_Remove,   "remove( self : fsnode )" },

	/*! For directory creates new file given its relative @path and returns its fsnode
	 *	\note '.' and '..' entries in @path are not allowed */
	{ FSNode_Makefile, "mkfile( self : fsnode, path : string )=>fsnode" },

	/*! For directory creates new directory given its relative @path and returns its fsnode
	 *	\note '.' and '..' entries in @path are not allowed */
	{ FSNode_Makedir,  "mkdir( self : fsnode, path : string )=>fsnode" },

	/*! For directory returns list of inner file objects of the given @type with names matching @filter,
	 * where @filter type is defined by @filtering and can be either a wildcard pattern or normal string pattern */
	{ FSNode_Children, "children( self : fsnode, type : enum<files; dirs>, filter='*', filtering : enum<wildcard, regex> = $wildcard )=>list<fsnode>" },

	/*! For directory returns list of inner files with names matching @filter,
	 * where @filter type is defined by @filtering and can be either a wildcard pattern or normal string pattern */
	{ FSNode_Files,    "files( self : fsnode, filter='*', filtering : enum<wildcard, regex> = $wildcard )=>list<fsnode>" },

	/*! For directory returns list of inner directories with names matching @filter,
	 * where @filter type is defined by @filtering and can be either a wildcard pattern or normal string pattern */
	{ FSNode_Dirs,     "dirs( self : fsnode, filter='*', filtering : enum<wildcard, regex> = $wildcard )=>list<fsnode>" },

	/*! For directory returns inner fsnode given its relative @path
	 *	\note '.' and '..' entries in @path are not allowed */
	{ FSNode_Child,    "[]( self : fsnode, path : string )=>fsnode" },

	/*! Copies file and returns fsnode of the copy
	 *	\note '.' and '..' entries in @path are not allowed; file will still be copied, but fsnode object will not be created */
	{ FSNode_Copy,  "copy( self : fsnode, path : string )=>fsnode" },

	/*! Returns the current working directory */
	{ FSNode_GetCWD,   "cwd(  )=>fsnode" },

	/*! For directory makes it the current working directory */
	{ FSNode_SetCWD,   "set_cwd( self : fsnode )" },

	/*! Re-reads all attributes of linked file object */
	{ FSNode_Update,   "update( self : fsnode )" },
	{ NULL, NULL }
};

DaoTypeBase fsnodeTyper = {
	"fsnode", NULL, NULL, fsnodeMeths, {NULL}, {0}, (FuncPtrDel)DInode_Delete, NULL
};

DAO_DLL int DaoFS_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_fsnode = DaoNamespace_WrapType( ns, & fsnodeTyper, 1 );
	return 0;
}
