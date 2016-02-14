#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <error.h>
#include <regex.h>
#include <assert.h>

char* find_in_path(char*);
char* find_in_alias(char*);

char* find_in_path(char* command) {
  char* path = getenv("PATH");
  size_t path_len = strlen(path);
  char *match = NULL;
  if (path == NULL) {
    error(1, errno, "could not get path");
  } 
  char *buf = malloc(path_len+1);
  strcpy(buf, path);
  strtok(buf, ":");
  while(1) {
    char* entry = strtok(NULL, ":");
    if (entry == NULL) {
      //no more path entries
      break; 
    }
    size_t command_len = strlen(command);
    size_t entry_len = strlen(entry);
    char *fname = malloc(entry_len+command_len+2);
    strcpy(fname, entry);
    strcat(fname, "/");
    strcat(fname, command);
    struct stat sb;
    if (stat(fname, &sb) == 0 && sb.st_mode & S_IXUSR) {
      //match
      match = fname;
      goto cleanup;
    }
    free(fname);
  }
  cleanup:
  free(buf);
  return match;
}

char* find_in_alias(char* command) {
  char* shell = getenv("SHELL");
  if (shell == NULL) {
    error(1, errno, "failed to read 'SHELL' environment variable");
  }
  char* alias_command = malloc(strlen(shell) + 15);
  sprintf(alias_command, "%s -ic alias", shell);
  FILE *fp = popen(alias_command, "r");
  if (fp == NULL) {
    error(1, errno, "failed to run alias command '%s'", alias_command);
  }
  free(alias_command);
  const char* alias_pattern = "alias ([^=]+)=";
  regex_t r;
  if (regcomp(&r, alias_pattern, REG_EXTENDED) != 0) {
    error(1, errno, "failed to compile regex '%s'", alias_pattern);
  }
  char* line = NULL;
  size_t rowlen = 0;
  ssize_t read;
  while ((read = getline(&line, &rowlen, fp)) != -1) {
    regmatch_t m[2]; 
    if (regexec(&r, line, 2, m, 0) == 0) {
      assert(m[0].rm_so != -1);
      assert(m[1].rm_eo != -1);
      unsigned int len = (unsigned int) (m[1].rm_eo - m[1].rm_so);
      char* alias = malloc(len+1);
      memcpy(alias, line + m[1].rm_so, len);
      alias[len] = '\0';
      if (strcmp(alias, command) == 0) {
        printf("%s: %s", shell, line);
      }
      free(alias);
    }
  }
  free(line);
  regfree(&r);
  pclose(fp);
  printf("command is '%s'\n", command);
  return NULL;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    error(1, errno, "missing command name");
  }
  char* command = argv[1];
  find_in_alias(command);
  char* match = find_in_path(command);
  if (match != NULL) {
    printf("executable %s\n", match);
    free(match);
  }
  return 0;
}
