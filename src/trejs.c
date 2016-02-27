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
#include <libgen.h>

static int EXIT_NO_MATCH=1;
static int EXIT_OTHER_ERROR=2;

void *alloc(size_t size);
void *alloc(size_t size) {
  void* m = malloc(size);
  if (!m) {
    error(EXIT_OTHER_ERROR, errno, "failed to allocate memory");
  }
  return m;
}

char* strdup2(const char* str);
char* strdup2(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  char* duped = strdup(str);
  if (duped == NULL) {
    error(EXIT_OTHER_ERROR, errno, "Unable to duplicate string '%s'", str);
  } 
  return duped;
}


typedef struct file_match {
  char* path;
  char* link_to;
} file_match;

void free_file_match(file_match* fm);
void free_file_match(file_match* fm) {
  if(fm == NULL) {
    return;
  }
  free(fm->path);
  free(fm->link_to);
  free(fm);
}

typedef struct path_match {
  file_match *fm;
  char* path_segment;
} path_match;

void free_path_match(path_match* pm);
void free_path_match(path_match* pm) {
  if (pm == NULL) {
    return; 
  }
  free_file_match(pm->fm);
  free(pm->path_segment);
  free(pm);
}

file_match* mk_file_match(char* path, char* link_to);
file_match* mk_file_match(char* path, char* link_to) {
  file_match* fm = alloc(sizeof(file_match));
  fm->path = path;
  fm->link_to = link_to;
  return fm;
}

path_match* mk_path_match(file_match* fm, char* path_segment);
path_match* mk_path_match(file_match* fm, char* path_segment) {
  path_match* pm = alloc(sizeof(path_match));
  pm->fm = fm; 
  pm->path_segment = strdup2(path_segment);
  return pm;
}


char* canonicalize(char* path, struct stat *sb);
char* canonicalize(char* path, struct stat *sb) {
    size_t size = (unsigned long) sb->st_size;
    char* linkname = alloc(size + 1); 
    long r = readlink(path, linkname, size);
    if (r == -1) {
      error(EXIT_OTHER_ERROR, errno, "could not read link '%s'", path);
    }
    linkname[r] = '\0';
    //linkname can be absolute or relative, see "path_resolution(7)"
    assert(r > 0);
    if (linkname[0]=='/') {
      return linkname;
    } else {
      char* path2 = strdup(path);
      char* dir = dirname(path2);
      char* template = "%s/%s";
      char* abs = malloc(strlen(dir) + strlen(linkname) + strlen(template));
      sprintf(abs, template, dir, linkname);
      free(linkname); 
      free(path2);
      return abs;
    }
}

file_match* find_file(char* path);
file_match* find_file(char* path) {
  struct stat sb;
  if (lstat(path, &sb) == 0) {
    if (S_ISLNK(sb.st_mode)) {
      //link
      char* actualpath = canonicalize(path, &sb);
      return mk_file_match(strdup2(path), actualpath);
    }
    else if (S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR)) {
      //executable
      return mk_file_match(strdup2(path), NULL);
    }
  }
  return NULL;
}


path_match* find_in_path(char* command);
path_match* find_in_path(char* command) {
  const char* path = getenv("PATH");
  if (path == NULL) {
    error(EXIT_OTHER_ERROR, errno, "could not get PATH environment variable");
  } 
  // strtok modifies strings, so copy first
  char* path2 = strdup2(path);
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
    char *fpath = alloc(entry_len+command_len+2);
    strcpy(fpath, entry);
    strcat(fpath, "/");
    strcat(fpath, command);
    file_match* fm = find_file(fpath);
    free(fpath);
    if (fm != NULL) {
      match = mk_path_match(fm, entry);
      break;
    }
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

typedef struct builtin_match{
  char* shell;
} builtin_match;

typedef struct type_match{
  alias_match* alias_match;
  builtin_match* builtin_match;
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
  if (m->builtin_match != NULL) {
    builtin_match* bm = m->builtin_match;
    free(bm->shell);
    free(bm);
  }
  free(m);
}


type_match* find_type(char*);
type_match* find_type(char* command) {
  char* shell = getenv("SHELL");
  if (shell == NULL) {
    error(EXIT_OTHER_ERROR, errno, "failed to read 'SHELL' environment variable");
  }
  const char* template = "%s -ic 'type %s' 2>&1";
  char* alias_command = alloc(strlen(shell) + strlen(command) + strlen(template));
  sprintf(alias_command, template, shell, command);
  FILE *fp = popen(alias_command, "r");
  if (fp == NULL) {
    error(EXIT_OTHER_ERROR, errno, "failed to run alias command '%s'", alias_command);
  }
  free(alias_command);
  const char* alias_pattern = ".*is aliased to `(([^' ]*) [^']*)'";
  regex_t alias_r;
  if (regcomp(&alias_r, alias_pattern, REG_EXTENDED) != 0) {
    error(EXIT_OTHER_ERROR, errno, "failed to compile regex '%s'", alias_pattern);
  }
  char* line = NULL;
  size_t rowlen = 0;
  ssize_t read;
  type_match *m = NULL;
  while ((read = getline(&line, &rowlen, fp)) != -1) {
      if(strstr(line, "is a shell builtin")) {
        m = alloc(sizeof(type_match));
        builtin_match* bm = alloc(sizeof(builtin_match));
        bm->shell=strdup2(shell);
        m->builtin_match=bm;
        m->alias_match=NULL;
        break;
      }
      char* alias_for=get_group(&alias_r, line, 2);
      if (alias_for != NULL) {
        char* declaration=get_group(&alias_r, line, 1);
        alias_match *am = alloc(sizeof(alias_match));
        am->shell=strdup2(shell);
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
  file_match* file_match;
  type_match *type_match;
  path_match* path_match;
} match;

void free_match(match* m);
void free_match(match* m) {
  if (m == NULL) {
    return;
  }
  free_file_match(m->file_match);
  free_type_match(m->type_match);
  free_path_match(m->path_match);
  free(m);
}

static unsigned int FIND_FILE=1<<0;
static unsigned int FIND_TYPE=1<<1;
static unsigned int FIND_PATH=1<<2;

match *find(char* command, unsigned int bans);
match *find(char* command, unsigned int bans) {
  match* m = alloc(sizeof(match));
  m->type_match=NULL;
  m->path_match=NULL;
  if ((bans & FIND_FILE) == 0) {
    m->file_match = find_file(command);
    if (m->file_match != NULL) {
      return m;
    }
  }
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
  c->name = strdup2(name);
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


int find_recursive(char* command);
int find_recursive(char* command) {
  cmd *first = mk_cmd(command, NULL);
  cmd* current = first;
  int exit_status=EXIT_SUCCESS;
  while(current != NULL) {
    //printf("looking up '%s'\n", current->name);
    unsigned int bans = found_as(first, current->name);
    match *m = find(current->name, bans);
    if (m==NULL) {
      printf("no match\n");
      exit_status=EXIT_NO_MATCH;
      break;
    }
    char* next_name = NULL;
    if (m->type_match != NULL) {
      current->match_type=FIND_TYPE;
      type_match * tm = m->type_match;
      if (tm->alias_match != NULL) {
        alias_match* am = tm->alias_match;
        printf("'%s' is an alias for '%s' in shell '%s': '%s'\n", current->name, am->alias_for, am->shell,  am->declaration);
        next_name = am->alias_for;
      } else if(tm->builtin_match) {
        printf("'%s' is built into shell '%s'\n", current->name, tm->builtin_match->shell);
      }
    }
    else if (m->path_match != NULL) {
      //Command found in path
      current->match_type=FIND_PATH;
      path_match* pm = m->path_match;
      printf("'%s' found in PATH as '%s'\n", current->name, pm->fm->path);
      if (pm->fm->link_to == NULL) {
        printf("'%s' is an executable\n", pm->fm->path);
        next_name = NULL;
      } else {
        printf("'%s' is a symlink to '%s'\n", pm->fm->path, pm->fm->link_to);
        next_name = pm->fm->link_to;
      }
    } else if(m->file_match != NULL) {
      //Command was straight up file
      current->match_type=FIND_FILE;
      file_match* fm = m->file_match;
      assert(strcmp(current->name, fm->path) == 0);
      if (fm->link_to == NULL) {
        printf("'%s' is an executable\n", current->name);
        next_name = NULL;
      } else {
        printf("'%s' is a symlink to '%s'\n", current->name, fm->link_to);
        next_name = fm->link_to;
      }
    }
    if (next_name == NULL) {
      printf("target reached\n");
      exit_status=EXIT_SUCCESS ;
    }
    current->done=1;
    //alloc
    cmd* next = maybe_mk_cmd(next_name, NULL);
    current->next = next;
    current = next; 
    //first free
    free_match(m);
  }
  free_cmds(first);
  return exit_status;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    error(EXIT_OTHER_ERROR, errno, "missing command name");
  }
  char* command = argv[1];
  return find_recursive(command);
}
