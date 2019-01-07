#include <stdlib.h>
#include <fftw3.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include "debug.h"

void *___thread(void *data);


static unsigned int thread_pool_size = DEFAULT_THREAD_POOL_SIZE;


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
	___thread_data_t			___data[THREAD_POOL_SIZE];
} ___processing_ctx_t;


void set_thread_pool_size( unsigned int size )
{
	unsigned int		old_thread_pool_size = thread_pool_size;

	thread_pool_size = size;
	/////////////////////////////////
	___thread_pool_resize();
}


int ___thread_data_init(___thread_data_t *entry, int id)
{
	if ( NULL == entry )
		return -1;

	memset( entry, 0, sizeof( __fftw_thread_data_t ) );
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
	___processing_ctx_t	*p = (___processing_ctx_t *)calloc(1, sizeof(___processing_ctx_t));
	debug(LogLevel_Info, " %p ", p);
	return p;
}

int ___processing_init(void ** ctx)
{
	___processing_ctx_t		*___processing_ctx = __alloc__processing_ctx() ;

	if ( NULL == ___processing_ctx)
		return -1;
	if ( NULL == ctx )
		return -1;

	for (unsigned int idx = 0; idx < THREAD_POOL_SIZE; idx ++)
	{
		___thread_data_init( &___processing_ctx->__tr_data[idx], idx );
	}

	*ctx = ___processing_ctx;
	return 0;
}

void ___client_fcall( void *ctx, unsigned int cmd, void *buf, unsigned int buf_sz )
{
	___processing_ctx_t		*___processing_ctx = (___processing_ctx_t *)ctx;
	unsigned char			*___data = NULL;

	for ( unsigned int idx = 0; idx < THREAD_POOL_SIZE; idx ++)
	{
		if ( 0 == pthread_mutex_trylock( &___processing_ctx->__tr_data[idx].thread_data_lock ) )
		{
			___data = ( unsigned char * ) calloc ( 1, sizeof( unsigned int ) + buf_sz );
			memcpy( ___data, cmd, sizeof( unsigned int ) );
			pthread_mutex_unlock( &___processing_ctx->__tr_data[idx].thread_data_lock );
			if ( 0 == pthread_cond_signal( &___processing_ctx->__tr_data[idx].thread_data_cond ) )
			{
				break;
			}
		}
	}
	return E_OK;
}

void *___thread(void *data)
{
	___thread_data_t		*___thread_data = ( ___thread_data_t * )data;

	while( !___thread_data->exit_thread )
	{
		pthread_mutex_lock( &___thread_data->thread_data_lock );
		if ( 0 == pthread_cond_wait( &___thread_data->thread_data_cond, &___thread_data->thread_data_lock ) )
		{
			
			pthread_mutex_unlock( &___thread_data->thread_data_lock );
		}
	}

	return NULL;
}

