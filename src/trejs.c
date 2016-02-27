#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <error.h>
#include <regex.h>
#include <assert.h>
#include <unistd.h>

void *alloc(size_t size);
void *alloc(size_t size) {
  void* m = malloc(size);
  if (!m) {
    error(1, errno, "failed to allocate memory");
  }
  return m;
}

typedef struct path_match {
  char* path;
  char* link_to;
} path_match;

void free_path_match(path_match* pm);
void free_path_match(path_match* pm) {
  if (pm == NULL) {
    return; 
  }
  free(pm->path);
  free(pm->link_to);
  free(pm);
}

path_match* mk_path_match(char* path, char* link_to);
path_match* mk_path_match(char* path, char* link_to) {
  path_match* pm = alloc(sizeof(path_match));
  pm->path = path;
  pm->link_to = link_to;
  return pm;
}

path_match* find_in_path(char*);

path_match* find_in_path(char* command) {
  const char* path = getenv("PATH");
  if (path == NULL) {
    error(1, errno, "could not get PATH environment variable");
  } 
  // strtok modifies strings, so copy first
  char* path2 = strdup(path);
  strtok(path2, ":");
  path_match* match=NULL;
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
    if (lstat(fname, &sb) == 0) {
      if (S_ISLNK(sb.st_mode)) {
        //link
        char actualpath [PATH_MAX+1];
        if (!realpath(fname, actualpath)) {
          error(1, errno, "could not read link '%s'", fname);
        }
        match = mk_path_match(fname, strdup(actualpath));
        break;
      }
      else if (S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR)) {
        //executable
        match = mk_path_match(fname, NULL);
        break;
      }
    }
    free(fname);
  }
  free(path2);
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

typedef struct type_match{
  alias_match* alias_match;
  unsigned int builtin_match;
  char pad[4];
} type_match;


void free_type_match(type_match *m);
void free_type_match(type_match *m) {
  if (m == NULL) {
    return;
  }
  if (m->alias_match != NULL) {
    alias_match* ma = m->alias_match;
    free(ma->shell);
    free(ma->declaration);
    free(ma->alias_for);
    free(ma);
  }
  free(m);
}


type_match* find_type(char*);
type_match* find_type(char* command) {
  char* shell = getenv("SHELL");
  if (shell == NULL) {
    error(1, errno, "failed to read 'SHELL' environment variable");
  }
  const char* template = "%s -ic 'type %s'";
  char* alias_command = alloc(strlen(shell) + strlen(command) + strlen(template));
  sprintf(alias_command, template, shell, command);
  FILE *fp = popen(alias_command, "r");
  if (fp == NULL) {
    error(1, errno, "failed to run alias command '%s'", alias_command);
  }
  free(alias_command);
  const char* alias_pattern = ".*is aliased to `(([^' ]*) [^']*)'";
  regex_t alias_r;
  if (regcomp(&alias_r, alias_pattern, REG_EXTENDED) != 0) {
    error(1, errno, "failed to compile regex '%s'", alias_pattern);
  }
  char* line = NULL;
  size_t rowlen = 0;
  ssize_t read;
  type_match *m = NULL;
  while ((read = getline(&line, &rowlen, fp)) != -1) {
      if(strstr(line, "is a shell builtin")) {
        m = alloc(sizeof(type_match));
        m->builtin_match=1;
        m->alias_match=NULL;
        break;
      }
      char* alias_for=get_group(&alias_r, line, 2);
      if (alias_for != NULL) {
        char* declaration=get_group(&alias_r, line, 1);
        alias_match *am = alloc(sizeof(alias_match));
        am->shell= alloc(strlen(shell)+1);
        strcpy(am->shell, shell);
        am->declaration=declaration;
        am->alias_for=alias_for;
        m = alloc(sizeof(type_match));
        m->alias_match = am;
        m->builtin_match = 0;
        break;
      }
      free(alias_for);
  }
  free(line);
  regfree(&alias_r);
  pclose(fp);
  return m;
}

typedef struct match {
  type_match *type_match;
  path_match* path_match;
} match;

void free_match(match* m);
void free_match(match* m) {
  if (m == NULL) {
    return;
  }
  free_type_match(m->type_match);
  free_path_match(m->path_match);
  free(m);
}

static unsigned int FIND_TYPE=1<<0;
static unsigned int FIND_PATH=1<<1;

match *find(char* command, unsigned int bans);
match *find(char* command, unsigned int bans) {
  match* m = alloc(sizeof(match));
  m->type_match=NULL;
  m->path_match=NULL;
  if ((bans & FIND_TYPE) == 0) {
    m->type_match = find_type(command);
    if (m->type_match != NULL) {
      return m;
    }
  }
  if ((bans & FIND_PATH) == 0) {
    m->path_match = find_in_path(command);
    if (m->path_match != NULL) {
      return m;
    }
  }
  free(m);
  return NULL;
}


typedef struct cmd {
  char* name;
  struct cmd* next;
  unsigned int done;
  unsigned int match_type;
} cmd;

unsigned int found_as(cmd* first, char* name);
unsigned int found_as(cmd* first, char* name) {
  cmd* current = first;
  unsigned int matches = 0;
  while(current != NULL) {
    if(strcmp(name, current->name)==0 && current->done) {
      matches |= current->match_type;
    }
    current=current->next;
  }
  return matches;
}

cmd* mk_cmd(char* name, cmd* next);
cmd* mk_cmd(char* name, cmd* next) {
  cmd* c = alloc(sizeof(cmd));
  c->name = strdup(name);
  c->next = next;
  c->done = 0;
  return c;
}

cmd* maybe_mk_cmd(char* name, cmd* next);
cmd* maybe_mk_cmd(char* name, cmd* next) {
  if (name == NULL) {
    return NULL;
  }
  return mk_cmd(name, next);
}

void free_cmds(cmd* first);
void free_cmds(cmd* first) {
  cmd* current = first;
  while(current != NULL) {
    free(current->name);
    cmd* next = current->next;
    free(current);
    current = next;
  } 
}

void find_recursive(char* command);
void find_recursive(char* command) {
  cmd *first = mk_cmd(command, NULL);
  cmd* current = first;
  while(current != NULL) {
    printf("looking for '%s'\n", current->name);
    unsigned int bans = found_as(first, current->name);
    match *m = find(current->name, bans);
    if (m==NULL) {
      printf("no match\n");
      break;
    }
    char* next_name = NULL;
    if (m->type_match != NULL) {
      current->match_type=FIND_TYPE;
      type_match * tm = m->type_match;
      if (tm->alias_match != NULL) {
        alias_match* am = tm->alias_match;
        printf("'%s' is an alias for '%s' in shell %s: %s\n", current->name, am->alias_for, am->shell,  am->declaration);
        next_name = am->alias_for;
      } else if(tm->builtin_match) {
        printf("'%s' is a shell builtin\n", current->name);
      }
    }
    else if (m->path_match != NULL) {
      current->match_type=FIND_PATH;
      path_match* pm = m->path_match;
      if (pm->link_to == NULL) {
        printf("'%s' is executable '%s'\n", current->name, pm->path);
        next_name = NULL;
      } else {
        printf("'%s' is symlink '%s' to '%s'\n", current->name, pm->path, pm->link_to);
        next_name = pm->link_to;
      }
    }
    if (next_name == NULL) {
      printf("done\n");
    }
    current->done=1;
    cmd* next = maybe_mk_cmd(next_name, NULL);
    current->next = next;
    current = next; 
    free_match(m);
  }
  free_cmds(first);
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
