#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

#include "debug.h"
#include "thread_pool.h"

void *___thread( void *___data );

typedef enum
{
	__THREAD_ENTRY_STATUS_NOT_INIT = 0,
	__THREAD_ENTRY_STATUS_FREE,
	__THREAD_ENTRY_STATUS_WORKING,
} ___entry_status_t;

typedef struct ___thread_data
{
	pthread_t				thread;
	pthread_attr_t				attr;
	pthread_mutex_t			 	thread_data_lock;
	pthread_cond_t				thread_data_cond;
	___entry_status_t			status;
	int					exit_thread;
	int					id;
	void					*data;
} ___thread_data_t;

typedef struct ___processing_ctx
{
	unsigned int				thread_pool_size;
	___thread_data_t			**___data;
} ___processing_ctx_t;

typedef struct ___poll_cmd_map
{
	pollfunc				cmd_func;
} ___poll_cmd_map_s;

static ___poll_cmd_map_s	command_map[POLL_CMD_END];
static ___processing_ctx_t	*processing_ctx;



int add_command_map( poll_cmd_e cmd, pollfunc cmd_func )
{
	if ( cmd >= POLL_CMD_END )
		return EERROR;
	if ( NULL != command_map[ cmd ].cmd_func )
		return EERROR;

	command_map[ cmd ].cmd_func = cmd_func;
	return EOK;
}

int remove_command_map( poll_cmd_e cmd )
{
	if ( cmd >= POLL_CMD_END )
		return EERROR;
	if ( NULL == command_map[ cmd ].cmd_func )
		return EERROR;

	command_map[ cmd ].cmd_func = NULL;
	return EOK;
}

static int ___thread_data_init( ___thread_data_t *entry, int id )
{
	if ( NULL == entry )
		return -1;

	memset( entry, 0, sizeof( ___thread_data_t ) );
	if ( pthread_attr_init( &entry->attr ) )
		return -1;
	if ( pthread_mutex_init( &entry->thread_data_lock, NULL ) )
		return -1;
	if ( pthread_cond_init ( &entry->thread_data_cond, NULL ) )
		return -1;
	if ( pthread_create( &entry->thread, &entry->attr, ( void * )___thread, entry ) )
		return -1;

	entry->id = id;

	return 0;
}

static ___processing_ctx_t *__alloc__processing_ctx()
{
	___processing_ctx_t		*p = NULL;
	
	p = ( ___processing_ctx_t * )calloc( 1, sizeof( ___processing_ctx_t ) );
	if ( NULL == p )
	{
		p->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
		p->___data = NULL;
	}

	debug( LogLevel_Info, " %p ", p );
	return p;
}

int processing_init( unsigned int size )
{
	processing_ctx = __alloc__processing_ctx() ;

	if ( NULL == processing_ctx )
		return -1;

	if ( 0 == size )
		processing_ctx->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
	else
		processing_ctx->thread_pool_size = size;
	processing_ctx->___data = ( ___thread_data_t ** ) calloc( processing_ctx->thread_pool_size, sizeof( ___thread_data_t ) );
	for ( unsigned int idx = 0; idx < processing_ctx->thread_pool_size; idx ++ )
	{
		___thread_data_init( processing_ctx->___data[idx], idx );
	}

	memset( command_map, 0, sizeof( command_map ) );
	return 0;
}

void client_fcall( unsigned int cmd, void *buf, unsigned int buf_sz )
{
	unsigned char			*___data = NULL;

	for ( unsigned int idx = 0; idx < processing_ctx->thread_pool_size; idx ++ )
	{
		if ( 0 == pthread_mutex_trylock( &processing_ctx->___data[idx]->thread_data_lock ) )
		{
			if ( NULL  != processing_ctx->___data[idx]->data )
			{
				free( processing_ctx->___data[idx]->data );
				processing_ctx->___data[idx]->data = NULL;
			}

			___data = processing_ctx->___data[idx]->data;
			if ( NULL != ( ___data = ( unsigned char * ) calloc ( 1, sizeof( unsigned int ) + sizeof( unsigned int )+ buf_sz ) ) )
			{
				memcpy( ___data, &cmd, sizeof( unsigned int ) );
				memcpy( ___data + sizeof( unsigned int ), &buf_sz, sizeof( unsigned int ) );
				memcpy( ___data + sizeof( unsigned int ) + sizeof( unsigned int ), buf, buf_sz );
			}

			pthread_mutex_unlock( &processing_ctx->___data[idx]->thread_data_lock );
			if ( 0 == pthread_cond_signal( &processing_ctx->___data[idx]->thread_data_cond ) )
			{
				break;
			}
		}
	}
}

void *___thread( void *___data )
{
	___thread_data_t		*___thread_data = ( ___thread_data_t * )___data;
	void				*___client_data;
	unsigned int			cmd;
	unsigned int			___data_sz;

	while( !___thread_data->exit_thread )
	{
		pthread_mutex_lock( &___thread_data->thread_data_lock );
		if ( 0 == pthread_cond_wait( &___thread_data->thread_data_cond, &___thread_data->thread_data_lock ) )
		{
			if ( NULL != ___thread_data->data )
			{
				memcpy( &cmd, ___thread_data->data, sizeof( unsigned int ) );
				if ( ( cmd >= POLL_CMD_END ) || ( NULL != command_map[cmd].cmd_func ) )
				{
					memcpy( &___data_sz, ___thread_data->data + sizeof( unsigned int ), sizeof( unsigned int ) );
					if ( NULL != ( ___client_data = ( void * )calloc(1, ___data_sz) ) )
					{
						memcpy( ___client_data, ___thread_data->data + sizeof( unsigned int ) + sizeof( unsigned int ), ___data_sz);
						client_fcall(cmd, ___client_data, ___data_sz);
					}
				}

				free( ___thread_data->data );
				___thread_data->data = NULL;
			}
			pthread_mutex_unlock( &___thread_data->thread_data_lock );
		}
	}

	return NULL;
}





