#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limits.h>


typedef enum
{
	DEBUG_TYPENONE			= 0,
	DEBUG_TYPE_NETWORK		= 1,
	DEBUG_TYPE_LOCAL		= 2,
} debug_type_e;

typedef struct __attribute__ ((__packed__)) __attribute__ ((aligned (16))) __debug_cfg
{
	debug_type_e			__debug_type;
	char				__debug_ip[16];
	int				__debug_port;
	char				__debug_filename[PATH_MAX - 11];
	int				__debug_level;
	unsigned char			__debug_mask:5;
} __debug_cfg_t;

typedef struct __debug_ctx
{
	unsigned int			initialized:1;
	FILE				*fp;
	char				fn[PATH_MAX];
	int				sock;
	struct sockaddr_in		sock_addr;
	char				ip[16];
	unsigned int 			port;
	debug_type_e			__debug_type;
	unsigned char			__debug_mask:5;
	int				__debug_level;
} __debug_ctx_t;

#define DEFAULT_DEBUG_TYPE		( DEBUG_TYPE_LOCAL )

#define DEFAULT_DEBUG_MASK		( PID_ENABLED | LINE_ENABLED | FILE_ENABLED | TIME_ENABLED )

#define DEFAULT_DEBUG_PORT 		( 50505 )
#define DEFAULT_DEBUG_IP		( "127.0.0.1" )

#define DEFAULT_DEBUG_FILE		( "default_debug.log" )

#define DEFAULT_DEBUG_LEVEL		( 20 )

#define SIZE_10K			( 1024 * 1024 )

#define PID_ENABLED			( 0 << 0 )
#define LINE_ENABLED			( 1 << 1 )
#define FILE_ENABLED			( 1 << 2 )
#define TIME_ENABLED			( 1 << 3 )


#define LogLevel_Info			( 0 )

__debug_ctx_t				debug_ctx;

static __inline__ void __init_debug( __debug_cfg_t *__in_debug_cfg )
{
	memset( &debug_ctx, 0, sizeof( debug_ctx ) );

	if ( NULL == __in_debug_cfg )
	{
		debug_ctx.__debug_mask = DEFAULT_DEBUG_MASK;
		debug_ctx.__debug_type = DEFAULT_DEBUG_TYPE;
	}

	if ( NULL != __in_debug_cfg )
		debug_ctx.__debug_level = __in_debug_cfg->__debug_level;
	else
		debug_ctx.__debug_level = DEFAULT_DEBUG_LEVEL;

	if ( DEBUG_TYPE_NETWORK == debug_ctx.__debug_type )
	{
		if ( ( NULL == __in_debug_cfg ) || ( 0 == __in_debug_cfg->__debug_port ) )
			debug_ctx.port = DEFAULT_DEBUG_PORT;
		else
			debug_ctx.port = __in_debug_cfg->__debug_port;

		if ( ( NULL == __in_debug_cfg ) || ( 0 == __in_debug_cfg->__debug_ip[0] ) )
			strcat( debug_ctx.ip, DEFAULT_DEBUG_IP );
		else
			strcat( debug_ctx.ip, __in_debug_cfg->__debug_ip );

		if ( -1 == ( debug_ctx.sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) )
		{
			printf( "Cannot open socket %s\n", strerror(errno) );
		}

		memset( ( char * ) &debug_ctx.sock_addr, 0, sizeof( debug_ctx.sock_addr ) );
		debug_ctx.sock_addr.sin_family = AF_INET;
		debug_ctx.sock_addr.sin_port = htons( debug_ctx.port );

		if ( 0 == inet_aton(debug_ctx.ip , &debug_ctx.sock_addr.sin_addr ) )
		{
			printf( "inet_aton returns %s\n", strerror( errno ) );
			close( debug_ctx.sock );
			debug_ctx.sock = -1;
		}
	}

	if ( DEBUG_TYPE_LOCAL == debug_ctx.__debug_type )
	{
		char			__debug_filename[PATH_MAX - 11];

		if ( ( NULL == __in_debug_cfg ) || ( 0 == __in_debug_cfg->__debug_filename[0] ) )
			strcat( __debug_filename, DEFAULT_DEBUG_FILE);
		else
			strcat( __debug_filename, __in_debug_cfg->__debug_filename );

		snprintf( debug_ctx.fn, sizeof( debug_ctx.fn ) - 1, "debug_%s_%d.txt", __debug_filename, getpid() );

		if ( NULL == ( debug_ctx.fp = fopen( debug_ctx.fn, "a+" ) ) )
		{
			printf( "Cannot open file %s\n", debug_ctx.fn );
		}
	}
	debug_ctx.initialized = 1;
}

static __inline__ void __debug( char *ds )
{
	if ( DEBUG_TYPE_LOCAL == debug_ctx.__debug_type )
		fputs( ds, debug_ctx.fp );
	if ( DEBUG_TYPE_NETWORK == debug_ctx.__debug_type )
		sendto( debug_ctx.sock, ds, strlen( ds ), 0, ( struct sockaddr * )&debug_ctx.sock_addr, sizeof( debug_ctx.sock_addr ) );
	if ( DEBUG_TYPENONE == debug_ctx.__debug_type )
		printf( "%s", ds );
}

#ifdef __DEBUG_ENABLED__
#define init_debug( DBG_GONFIG ) { __init_debug( DBG_GONFIG ); }
#define debug( debug_level, args ... ) {												\
	if ( 0 == debug_ctx.initialized ) { __init_debug( NULL ); }									\
	if ( debug_ctx.__debug_level >= debug_level ) {											\
		char	ds[1024]; unsigned int pos = 0;											\
		if ( debug_ctx.__debug_mask & TIME_ENABLED ) { 										\
			struct timeval tv; gettimeofday( &tv, NULL ); time_t t = tv.tv_sec; char *time_str = ctime( ( time_t * ) &t );	\
			time_str[strlen( time_str ) - 6] = 0; pos += sprintf( ds + pos, "[%s.%d]--", time_str, ( int )tv.tv_usec);	\
		}															\
		if ( debug_ctx.__debug_mask & FILE_ENABLED ) { pos += sprintf( ds + pos, "[%s]", __FILE__ ); }				\
		if ( debug_ctx.__debug_mask & LINE_ENABLED ) { pos += sprintf( ds + pos, "[%d]", __LINE__ ); }				\
		if ( debug_ctx.__debug_mask & PID_ENABLED )  { pos += sprintf( ds + pos, "[%d]", getpid() ); }				\
		pos += sprintf( ds + pos, "%s():", __FUNCTION__);									\
		sprintf( ds + pos, ##args ); strcat(ds, "\n");										\
		__debug( ds );														\
	}																\
}
#else
#define init_debug( DBG_GONFIG ) {	}
#define debug( debug_level, args ... ){	}
#endif


#endif // __DEBUG_H__



