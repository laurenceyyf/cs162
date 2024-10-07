/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "word_count.h"
#include "word_helpers.h"

<<<<<<< HEAD
struct count_data {
  word_count_list_t* wclist;
  FILE* infile;
};

void* count_words_p(void* threadarg) {
  struct count_data* data;
  data = (struct count_data*) threadarg;
  word_count_list_t* wclist = data->wclist;
  FILE* infile = data->infile;
  count_words(wclist, infile);
  pthread_exit(NULL);
}

=======
>>>>>>> 723c789bdce9bfed94ea2a1db9e60a3690ce4bcc
/*
 * main - handle command line, spawning one thread per file.
 */
int main(int argc, char* argv[]) {
  /* Create the empty data structure. */
  word_count_list_t word_counts;
  init_words(&word_counts);

  if (argc <= 1) {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  } else {
<<<<<<< HEAD
    int nthreads = argc - 1;
    pthread_t threads[nthreads];
    long t;
    int rc;

    struct count_data* count_data_array = malloc(nthreads * sizeof(struct count_data));
    for (t = 0; t < nthreads; t++) {
      count_data_array[t].wclist = &word_counts;
      count_data_array[t].infile = fopen(argv[t + 1], "r");
      if (count_data_array[t].infile == NULL) {
        return 1;
      }
      rc = pthread_create(&threads[t], NULL, count_words_p, (void*) &count_data_array[t]);
      if (rc) {
        return 1;
      }
    }
    for (t = 0; t < nthreads; t++) {
      pthread_join(threads[t], NULL);
    }
=======
    /* TODO */
>>>>>>> 723c789bdce9bfed94ea2a1db9e60a3690ce4bcc
  }

  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}
