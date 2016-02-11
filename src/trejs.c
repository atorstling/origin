#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>

char* find_in_path(char*);

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
    printf("at path entry '%s'\n", entry);
    size_t command_len = strlen(command);
    size_t entry_len = strlen(entry);
    char *fname = malloc(entry_len+command_len+2);
    strcpy(fname, entry);
    strcat(fname, "/");
    strcat(fname, command);
    printf("candidate is '%s'\n", fname);
    struct stat sb;
    if (stat(fname, &sb) == 0 && sb.st_mode & S_IXUSR) {
      printf("match\n");
      match = fname;
      goto cleanup;
    }
    free(fname);
  }
  cleanup:
  free(buf);
  return match;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    error(1, errno, "missing command name");
  }
  char* command = argv[1];
  printf("command name is '%s'\n", command);
  char* match = find_in_path(command);
  if (match != NULL) {
    printf("found executable %s\n", match);
    free(match);
  }
  return 0;
}
