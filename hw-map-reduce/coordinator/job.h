/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */
#include "../rpc/rpc.h"
#include <glib.h>
#include <stdbool.h>
#include <time.h>

#ifndef JOB_H__
#define JOB_H__

/* You may add definitions here */
struct job {
    int job_id;
	path* files;
	path output_dir;
	char* app;
	struct {
		u_int args_len;
		char *args_val;
	} args;
    int n_map;
    GList* map_queue;
    GList* running_map_queue;
    GHashTable* running_map_times; /* Maps task_id to start time */
    int n_reduce;
    GList* reduce_queue;
    GList* running_reduce_queue;
    GHashTable* running_reduce_times; /* Maps task_id to start time */
    bool done;
    bool failed;
};

#endif
