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

char* get_group(const regex_t *r, const char* line, unsigned int group);
char* get_group(const regex_t *r, const char* line, unsigned int group) {
    unsigned int ngroups = group+1;
    regmatch_t *m=alloc(ngroups*sizeof(regmatch_t)); 
    char* match=NULL;
    if (regexec(r, line, ngroups, m, 0) == 0) {
      for(unsigned int i=0; i<ngroups; i++) {
        assert(m[i].rm_so != -1);
      }
      unsigned int len = (unsigned int) (m[group].rm_eo - m[group].rm_so);
      char* alias = alloc(len+1);
      memcpy(alias, line + m[group].rm_so, len);
      alias[len] = '\0';
      match = alias;
    }
    free(m);
    return match;
}

typedef struct alias_match{
  char* shell;
  char* declaration;
  char* alias_for;
} alias_match;

void free_alias_match(alias_match *m);
void free_alias_match(alias_match *m) {
  if (m == NULL) {
    return;
  }
  free(m->shell);
  free(m->declaration);
  free(m->alias_for);
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
  const char* alias_pattern = "alias ([^=]+)='([^ ]+)";
  regex_t r;
  if (regcomp(&r, alias_pattern, REG_EXTENDED) != 0) {
    error(1, errno, "failed to compile regex '%s'", alias_pattern);
  }
  char* line = NULL;
  size_t rowlen = 0;
  ssize_t read;
  alias_match *m = NULL;
  while ((read = getline(&line, &rowlen, fp)) != -1) {
      char* alias=get_group(&r, line, 1);
      if (alias != NULL) {
        if (strcmp(alias, command) == 0) {
          char* alias_for=get_group(&r, line, 2);
          m = alloc(sizeof(alias_match));
          m->shell= alloc(strlen(shell)+1);
          m->declaration= alloc(strlen(line)+1);
          strcpy(m->shell, shell);
          strcpy(m->declaration, line);
          m->alias_for=alias_for;
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


typedef struct cmd {
  char* name;
  struct cmd* next;
} cmd;

int already_seen(cmd* first, char* name);
int already_seen(cmd* first, char* name) {
  cmd* current = first;
  while(current != NULL) {
    if(strcmp(name, current->name)==0) {
      return 1;
    }
    current=current->next;
  }
  return 0;
}

void find_recursive(char* command);
void find_recursive(char* command) {
  cmd first = { .name = command, .next = NULL };
  cmd* current = &first;
  match *m = find(current->name);
  if (m != NULL) {
    if (m->alias_match != NULL) {
      printf("alias in shell %s: %s", m->alias_match->shell, m->alias_match->declaration);
      if (!already_seen(&first, m->alias_match->alias_for)) {
        cmd* next = alloc(sizeof(cmd));
        current->next = next;
        current = next;
      }
    }
    else if (m->path_match != NULL) {
      printf("executable %s\n", m->path_match);
    }
  }
  free_match(m);

}

int main(int argc, char** argv)
{
  if (argc < 2) {
    error(1, errno, "missing command name");
  }
  char* command = argv[1];
  find_recursive(command);
  return 0;
}
