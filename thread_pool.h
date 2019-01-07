#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#define EOK					( 0 )
#define DEFAULT_THREAD_POOL_SIZE		( 50 )

typedef enum
{
        __THREAD_ENTRY_STATUS_NOT_INIT = 0,
        __THREAD_ENTRY_STATUS_FREE,
        __THREAD_ENTRY_STATUS_WORKING,
} ___entry_status_t;


void set_thread_pool_size( unsigned int size );

#endif // __THREAD_POOL_H__


