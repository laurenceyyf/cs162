#include <stdio.h>
#include <sys/resource.h>

int main() {
    struct rlimit stack_lim;
    struct rlimit process_lim;
    struct rlimit file_lim;
    getrlimit(RLIMIT_STACK, &stack_lim);
    getrlimit(RLIMIT_NPROC, &process_lim);
    getrlimit(RLIMIT_NOFILE, &file_lim);
    printf("stack size: %ld\n", stack_lim.rlim_cur);
    printf("process limit: %ld\n", process_lim.rlim_cur);
    printf("max file descriptors: %ld\n", file_lim.rlim_cur);
    return 0;
}
