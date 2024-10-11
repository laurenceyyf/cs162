#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "print the current working directory"},
    {cmd_cd, "cd", "change the current working directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 0;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints the current working directory */
int cmd_pwd(unused struct tokens* tokens) {
  size_t size = pathconf(".", _PC_PATH_MAX);
  char* buf = malloc(size);
  printf("%s\n", getcwd(buf, size));
  return 0;
}

/* Changes the current working directory */
int cmd_cd(struct tokens* tokens) {
  char* path = tokens_get_token(tokens, 1);
  int ret =  chdir(path);
  return ret;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int run_child_pipe(char* filepath, char** argv, int* pipe_fd, int prev_pipe) {
  int status;
  pid_t p = fork();
  if (p < 0) {
    perror("fork");
    return -1;
  } else if (p == 0) {
    if (pipe_fd != NULL) {
      close(pipe_fd[0]);
      dup2(pipe_fd[1], STDOUT_FILENO);
      close(pipe_fd[1]);
    }
    if (prev_pipe != -1) {
      dup2(prev_pipe, STDIN_FILENO);
      free(argv);
      argv = NULL;
      close(prev_pipe);
    }
    execv(filepath, argv);
    exit(1);
  } else {
    if (pipe_fd != NULL) {
      close(pipe_fd[1]);
    }
    wait(&status);
  }
  return status;
}

int run_process(char* filepath, char** argv, int* pipe_fd, int prev_pipe) {
  int status;
  if (filepath[0] != '/') {
    char* path_env = getenv("PATH");
    char* save_ptr;
    char* path_env_copy = malloc((strlen(path_env) + 1) * sizeof(char));
    path_env_copy = strcpy(path_env_copy, path_env);
    char* word = strtok_r(path_env_copy, ":", &save_ptr);
    while (word != NULL) {
      int path_len = strlen(word) + 1 + strlen(filepath) + 1;
      char* path = malloc(path_len * sizeof(char));
      strncpy(path, word, strlen(word));
      path[strlen(word)] = '/';
      strncpy(path + strlen(word) + 1, filepath, strlen(filepath));
      status = run_child_pipe(path, argv, pipe_fd, prev_pipe);
      free(path);
      if (status == 0) {
        break;
      }
      word = strtok_r(NULL, ":", &save_ptr);
    }
  } else {
    status = run_child_pipe(filepath, argv, pipe_fd, prev_pipe);
  }
  return status;
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  int saved_stdin = dup(STDIN_FILENO);
  int saved_stdout = dup(STDOUT_FILENO);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      int prev_pipe = -1;
      int pipe_fd[2] = {-1, -1};
      char* filepath = tokens_get_token(tokens, 0);
      int tokens_len = tokens_get_length(tokens);
      char** argv = malloc((tokens_len + 1) * sizeof(char*));
      argv[tokens_len] = NULL;
      int j = 0;
      for (int i = 0; i < tokens_len; i++) {
        char* token = tokens_get_token(tokens, i);
        if (strcmp(token, ">") == 0) {
          i++;
          char* filename = tokens_get_token(tokens, i);;
          int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          dup2(fd, STDOUT_FILENO);
          continue;
        }
        if (strcmp(token, "<") == 0) {
          i++;
          char* filename = tokens_get_token(tokens, i);;
          int fd = open(filename, O_RDONLY);
          dup2(fd, STDIN_FILENO);
          continue;
        }
        if (strcmp(token, "|") == 0) {
          if (pipe(pipe_fd) == -1) {
            perror("pipe");
            return -1;
          }
          argv[j] = NULL;
          int status = run_process(filepath, argv, pipe_fd, prev_pipe);
          if (status == -1) {
            return -1;
          }
          prev_pipe = dup(pipe_fd[0]);
          close(pipe_fd[0]);
          free(argv);
          argv = malloc((tokens_len - i) * sizeof(char*));
          argv[tokens_len - i - 1] = NULL;
          j = 0;
          filepath = tokens_get_token(tokens, i + 1);
          continue;
        }
        argv[j] = token;
        j++;
      }
      int status = run_process(filepath, argv, NULL, prev_pipe);
      if (prev_pipe != -1) {
        close(prev_pipe);
      }
      if (status == -1) {
        return -1;
      }
      free(argv);
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      dup2(saved_stdin, STDIN_FILENO);
      dup2(saved_stdout, STDOUT_FILENO);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
