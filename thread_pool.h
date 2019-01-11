#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#define EOK					( 0 )
#define EERROR					( 1 )
#define DEFAULT_THREAD_POOL_SIZE		( 50 )

typedef void ( *pollfunc )( void *, unsigned int );

typedef enum 
{
	POLL_CMD_NONE = 0,
	// ...
	POLL_CMD_END
} poll_cmd_e;

int add_command_map( poll_cmd_e cmd, pollfunc cmd_func );

int remove_command_map( poll_cmd_e cmd );

int processing_init( unsigned int size );

void client_fcall( unsigned int cmd, void *buf, unsigned int buf_sz );


#endif // __THREAD_POOL_H__


