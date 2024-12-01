/**
 * The MapReduce coordinator.
 */

#include "coordinator.h"

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

/* Global coordinator state. */
coordinator* state;

extern void coordinator_1(struct svc_req*, SVCXPRT*);

/* Set up and run RPC server. */
int main(int argc, char** argv) {
  register SVCXPRT* transp;

  pmap_unset(COORDINATOR, COORDINATOR_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, tcp).");
    exit(1);
  }

  coordinator_init(&state);

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* EXAMPLE RPC implementation. */
int* example_1_svc(int* argp, struct svc_req* rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

/* SUBMIT_JOB RPC implementation. */
int* submit_job_1_svc(submit_job_request* argp, struct svc_req* rqstp) {
  static int result;

  printf("Received submit job request\n");

  struct job* job = malloc(sizeof(struct job));

  /* Initialize app */
  if (get_app(argp->app).name == NULL) {
    result = -1;
    return &result;
  } else {
    job->app = strdup(argp->app);
  }

  /* Initialize job_id */
  job->job_id = state->next_job_id;
  state->next_job_id++;

  /* Initialize files */
  job->files = malloc(argp->files.files_len * sizeof(path));
  for (uint i = 0; i < argp->files.files_len; i++){
    job->files[i] = strdup(argp->files.files_val[i]);
  }

  /* Initialize output_dir */
  job->output_dir = strdup(argp->output_dir);

  /* Initialize args */
  job->args.args_len = argp->args.args_len;
  job->args.args_val = malloc(job->args.args_len * sizeof(char));
  strncpy(job->args.args_val, argp->args.args_val, job->args.args_len);

  /* Initialize map */
  job->n_map = argp->files.files_len;
  job->map_queue = NULL;
  for (int i = 0; i < job->n_map; i++) {
    job->map_queue = g_list_append(job->map_queue, GINT_TO_POINTER(i));
  }
  job->running_map_queue = NULL;
  job->running_map_times = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
   printf("Initialize map\n");

  /* Initialize reduce */
  job->n_reduce = argp->n_reduce;
  job->reduce_queue = NULL;
  for (int i = 0; i < job->n_reduce; i++) {
    job->reduce_queue = g_list_append(job->reduce_queue, GINT_TO_POINTER(i));
  }
  job->running_reduce_queue = NULL;
  job->running_reduce_times = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
   printf("Initialize reduce\n");

  /* Initialize status */
  job->done = false;
  job->failed = false;

  /* Add job to state->job_queue and state->job_table */
  state->job_queue = g_list_append(state->job_queue, GINT_TO_POINTER(job->job_id));
  g_hash_table_insert(state->job_table, GINT_TO_POINTER(job->job_id), job);

  result = job->job_id;

  /* Do not modify the following code. */
  /* BEGIN */
  struct stat st;
  if (stat(argp->output_dir, &st) == -1) {
    mkdirp(argp->output_dir);
  }

  return &result;
  /* END */
}

/* POLL_JOB RPC implementation. */
poll_job_reply* poll_job_1_svc(int* argp, struct svc_req* rqstp) {
  static poll_job_reply result;

  printf("Received poll job request\n");

  int job_id = *argp;
  struct job* job = g_hash_table_lookup(state->job_table, GINT_TO_POINTER(job_id));

  if (job == NULL) {
    result.done = true;
    result.failed = false;
    result.invalid_job_id = true;
  } else {
    result.done = job->done;
    result.failed = job->failed;
    result.invalid_job_id = false;
  }

  return &result;
}

/* GET_TASK RPC implementation. */
get_task_reply* get_task_1_svc(void* argp, struct svc_req* rqstp) {
  static get_task_reply result;

  printf("Received get task request\n");
  
  /* Initialize result */
  result.job_id = -1;
  result.task = -1;
  result.file = "";
  result.output_dir = "";
  result.app = "";
  result.n_reduce = -1;
  result.n_map = -1;
  result.reduce = false;
  result.wait = true;
  result.args.args_len = 0;
  result.args.args_val = "";

  if (state->job_queue == NULL) {
    return &result;
  }

  /* Run next job if the current job has no tasks to run */
  GList* cur_job_elem = g_list_first(state->job_queue);
  while (cur_job_elem != NULL) {
    int job_id = GPOINTER_TO_INT(cur_job_elem->data);
    struct job* job = g_hash_table_lookup(state->job_table, GINT_TO_POINTER(job_id));

    if (job->map_queue != NULL) {
      /* Pop the first map in map_queue */
      GList* first_map_elem = g_list_first(job->map_queue);
      int task_id = GPOINTER_TO_INT(first_map_elem->data);
      job->map_queue = g_list_remove(job->map_queue, GINT_TO_POINTER(task_id));

      /* Add map to the end of running_map_queue and running_map_times */
      job->running_map_queue = g_list_append(job->running_map_queue, GINT_TO_POINTER(task_id));
      time_t* cur_time = malloc(sizeof(time_t));
      *cur_time = time(NULL);
      g_hash_table_insert(job->running_map_times, GINT_TO_POINTER(task_id), cur_time);

      /* Update result */
      result.job_id = job_id;
      result.task = task_id;
      result.file = job->files[task_id];
      result.output_dir = job->output_dir;
      result.app = job->app;
      result.n_reduce = job->n_reduce;
      result.n_map = job->n_map;
      result.reduce = false;
      result.wait = false;
      result.args.args_len = job->args.args_len;
      result.args.args_val = job->args.args_val;
      break;
    } else if (job->running_map_queue != NULL) {
      /* Read the first map in running_map_queue and read its start time from running_map_times */
      GList* cur_map_elem = g_list_first(job->running_map_queue);
      while (cur_map_elem != NULL) {
        int task_id = GPOINTER_TO_INT(cur_map_elem->data);
        time_t* start_time = g_hash_table_lookup(job->running_map_times, GINT_TO_POINTER(task_id));
        time_t cur_time = time(NULL);

        /* Remove map from running_map_queue and running_map_times and add it to map_queue */
        if (cur_time >= *start_time + TASK_TIMEOUT_SECS) {
          g_hash_table_remove(job->running_map_times, GINT_TO_POINTER(task_id));
          job->running_map_queue = g_list_remove(job->running_map_queue, GINT_TO_POINTER(task_id));
          job->map_queue = g_list_append(job->map_queue, GINT_TO_POINTER(task_id));
        }
        cur_map_elem = cur_map_elem->next;
      }
      if (job->map_queue != NULL) {
        continue;
      }
    } else if (job->reduce_queue != NULL) {
      /* Pop the first reduce in reduce_queue */
      GList* first_reduce_elem = g_list_first(job->reduce_queue);
      int task_id = GPOINTER_TO_INT(first_reduce_elem->data);
      job->reduce_queue = g_list_remove(job->reduce_queue, GINT_TO_POINTER(task_id));

      /* Add reduce to the end of running_reduce_queue and running_reduce_times */
      job->running_reduce_queue = g_list_append(job->running_reduce_queue, GINT_TO_POINTER(task_id));
      time_t* cur_time = malloc(sizeof(time_t));
      *cur_time = time(NULL);
      g_hash_table_insert(job->running_reduce_times, GINT_TO_POINTER(task_id), cur_time);

      /* Update result */
      result.job_id = job_id;
      result.task = task_id;
      result.file = "";
      result.output_dir = job->output_dir;
      result.app = job->app;
      result.n_reduce = job->n_reduce;
      result.n_map = job->n_map;
      result.reduce = true;
      result.wait = false;
      result.args.args_len = job->args.args_len;
      result.args.args_val = job->args.args_val;
      break;
    } else if (job->running_reduce_queue != NULL) {
      /* Read the first reduce in running_reduce_queue and read its start time from running_reduce_times */
      GList* cur_reduce_elem = g_list_first(job->running_reduce_queue);
      while (cur_reduce_elem != NULL) {
        int task_id = GPOINTER_TO_INT(cur_reduce_elem->data);
        time_t* start_time = g_hash_table_lookup(job->running_reduce_times, GINT_TO_POINTER(task_id));
        time_t cur_time = time(NULL);

        /* Remove map from running_map_queue and running_map_times and add it to map_queue */
        if (cur_time >= *start_time + TASK_TIMEOUT_SECS) {
          g_hash_table_remove(job->running_reduce_times, GINT_TO_POINTER(task_id));
          job->running_reduce_queue = g_list_remove(job->running_reduce_queue, GINT_TO_POINTER(task_id));
          job->reduce_queue = g_list_append(job->reduce_queue, GINT_TO_POINTER(task_id));
        }
        cur_reduce_elem = cur_reduce_elem->next;
      }
      if (job->reduce_queue != NULL) {
        continue;
      }
    }
    cur_job_elem = cur_job_elem->next;
  }

  return &result;
}

/* FINISH_TASK RPC implementation. */
void* finish_task_1_svc(finish_task_request* argp, struct svc_req* rqstp) {
  static char* result;

  printf("Received finish task request\n");

  int job_id = argp->job_id;
  struct job* job = g_hash_table_lookup(state->job_table, GINT_TO_POINTER(job_id));
  int task_id = argp->task;

  if (job == NULL) {
    return (void*)&result;
  }
  
  if (argp->success) {
    if (argp->reduce) {
      job->running_reduce_queue = g_list_remove(job->running_reduce_queue, GINT_TO_POINTER(task_id));
    } else {
      job->running_map_queue = g_list_remove(job->running_map_queue, GINT_TO_POINTER(task_id));
    }

    if (!job->map_queue && !job->reduce_queue && !job->running_map_queue && !job->running_reduce_queue) {
      job->done = true;
    }
  } else {
    job->done = true;
    job->failed = true;
  }

  if (job->done) {
    state->job_queue = g_list_remove(state->job_queue, GINT_TO_POINTER(job_id));
  }

  return (void*)&result;
}

/* Initialize coordinator state. */
void coordinator_init(coordinator** coord_ptr) {
  *coord_ptr = malloc(sizeof(coordinator));

  coordinator* coord = *coord_ptr;

  coord->next_job_id = 0;
  coord->job_queue = NULL;
  coord->job_table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
}
