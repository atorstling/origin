#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <error.h>
#include <regex.h>
#include <assert.h>

void *alloc(size_t size);
void *alloc(size_t size) {
  void* m = malloc(size);
  if (!m) {
    error(1, errno, "failed to allocate memory");
  }
  return m;
}

void free2(void *d);
void free2(void *d) {
  if (d!=NULL) {
    free(d); 
  }
}


char* find_in_path(char*);

char* find_in_path(char* command) {
  char* path = getenv("PATH");
  char *match = NULL;
  if (path == NULL) {
    error(1, errno, "could not get path");
  } 
  strtok(path, ":");
  while(1) {
    char* entry = strtok(NULL, ":");
    if (entry == NULL) {
      //no more path entries
      break; 
    }
    size_t command_len = strlen(command);
    size_t entry_len = strlen(entry);
    char *fname = alloc(entry_len+command_len+2);
    strcpy(fname, entry);
    strcat(fname, "/");
    strcat(fname, command);
    struct stat sb;
    if (stat(fname, &sb) == 0 && sb.st_mode & S_IXUSR) {
      //match
      return fname;
    }
    free(fname);
  }
  return match;
}

char* get_first_group(const regex_t *r, const char* line);
char* get_first_group(const regex_t *r, const char* line) {
    regmatch_t m[2]; 
    if (regexec(r, line, 2, m, 0) == 0) {
      assert(m[0].rm_so != -1);
      assert(m[1].rm_eo != -1);
      unsigned int len = (unsigned int) (m[1].rm_eo - m[1].rm_so);
      char* alias = alloc(len+1);
      memcpy(alias, line + m[1].rm_so, len);
      alias[len] = '\0';
      return alias;
    }
    return NULL;
}

typedef struct alias_match{
  char* shell;
  char* declaration;
} alias_match;

void free_alias_match(alias_match *m);
void free_alias_match(alias_match *m) {
  if (m == NULL) {
    return;
  }
  free(m->shell);
  free(m->declaration);
  free(m);
}


alias_match* find_in_alias(char*);
alias_match* find_in_alias(char* command) {
  char* shell = getenv("SHELL");
  if (shell == NULL) {
    error(1, errno, "failed to read 'SHELL' environment variable");
  }
  char* alias_command = alloc(strlen(shell) + 15);
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
  alias_match *m = NULL;
  while ((read = getline(&line, &rowlen, fp)) != -1) {
      char* alias=get_first_group(&r, line);
      if (alias != NULL) {
        if (strcmp(alias, command) == 0) {
          m = alloc(sizeof(alias_match));
          m->shell= alloc(strlen(shell)+1);
          m->declaration= alloc(strlen(line)+1);
          strcpy(m->shell, shell);
          strcpy(m->declaration, line);
          free(alias);
          goto cleanup;
        }
        free(alias);
      }
  }
  cleanup:
  free(line);
  regfree(&r);
  pclose(fp);
  return m;
}

typedef struct match {
  alias_match *alias_match;
  char* path_match;
} match;

void free_match(match* m);
void free_match(match* m) {
  if (m == NULL) {
    return;
  }
  free_alias_match(m->alias_match);
  free2(m->path_match);
  free(m);
}

match *find(char* command);
match *find(char* command) {
  match* m = alloc(sizeof(match));
  m->alias_match=NULL;
  m->path_match=NULL;
  m->alias_match = find_in_alias(command);
  if (m->alias_match != NULL) {
    return m;
  }
  m->path_match = find_in_path(command);
  if (m->path_match != NULL) {
    return m;
  }
  return NULL;
}


int main(int argc, char** argv)
{
  if (argc < 2) {
    error(1, errno, "missing command name");
  }
  char* command = argv[1];
  match *m = find(command);
  if (m != NULL) {
    if (m->alias_match != NULL) {
      printf("alias in shell %s: %s", m->alias_match->shell, m->alias_match->declaration);
    }
    else if (m->path_match != NULL) {
      printf("executable %s\n", m->path_match);
    }
  }
  free_match(m);
  return 0;
}
