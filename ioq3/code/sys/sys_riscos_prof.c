/*
 * A sampling profiler for RISC OS, where gprof's profil() isn't usable.
 *
 * UnixLib switches threads from a timer every 2 centiseconds, on one core,
 * so while another thread runs the main thread is stopped and its saved
 * registers say where. A low-priority thread records the main thread's
 * saved pc each time it gets to run, then yields. That gives about 50
 * samples a second.
 *
 * Set the system variable Q3Prof to a file name before the program starts;
 * the pcs are written there, one hex address a line, when it exits.
 *
 * RISC OS port, copyright David Sharp 2026, GPL 2
 */
#ifdef __riscos__

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SAMPLES 200000

/* The start of UnixLib's struct __pthread_thread (libunixlib/incl-local/pthread.h) */
struct riscos_pthread_head
{
	int magic;
	int *saved_context;	/* struct __pthread_saved_context: r[16] first */
};

static pthread_t prof_main;
static unsigned int *prof_samples;
static volatile int prof_count;
static volatile int prof_stop;
static const char *prof_file;

static void *prof_thread( void *arg )
{
	(void)arg;
	while ( !prof_stop )
	{
		struct riscos_pthread_head *head = (struct riscos_pthread_head *)prof_main;

		if ( head && head->saved_context && prof_count < MAX_SAMPLES )
			prof_samples[prof_count++] = (unsigned int)head->saved_context[15];
		sched_yield();
	}
	return NULL;
}

static void prof_write( void )
{
	FILE *f;
	int i;

	prof_stop = 1;
	f = fopen( prof_file, "w" );
	if ( !f )
		return;
	for ( i = 0; i < prof_count; i++ )
		fprintf( f, "%x\n", prof_samples[i] );
	fclose( f );
}

__attribute__((constructor)) static void prof_start( void )
{
	pthread_t thread;

	prof_file = getenv( "Q3Prof" );
	if ( !prof_file || !*prof_file )
		return;
	prof_samples = malloc( MAX_SAMPLES * sizeof( *prof_samples ) );
	if ( !prof_samples )
		return;
	prof_main = pthread_self();
	if ( pthread_create( &thread, NULL, prof_thread, NULL ) == 0 )
		atexit( prof_write );
}

#endif
