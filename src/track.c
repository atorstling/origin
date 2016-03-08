#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <regex.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>
#include <stdarg.h>

static int EXIT_NO_MATCH=1;
static int EXIT_OTHER_ERROR=2;
static char* program_name;

void error(int exit_code, int errnum, char* format, ...)__attribute__((noreturn));
void error(int exit_code, int errnum, char* format, ...) {
  fflush(stdout);
  fputs(program_name, stderr);
  fputs(": ", stderr);
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  if (errnum != 0) {
    fputs(": ", stderr);
    fputs(strerror(errnum), stderr);
  }
  fputs("\n", stderr);
  exit(exit_code);
}

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

char* mk_path(char* dir, char* filename);
char* mk_path(char* dir, char* filename) {
  char* abs;
  if (asprintf(&abs, "%s/%s", dir, filename) == -1) {
    error(EXIT_OTHER_ERROR, errno, "could not format dirname");
  }
  return abs;
}


typedef struct file_match {
  char* path;
  char* link_to;
  int executable; 
  char pad[4];
} file_match;

file_match* mk_file_match(char* path, int executable, char* link_to);
file_match* mk_file_match(char* path, int executable, char* link_to) {
  file_match* fm = alloc(sizeof(file_match));
  fm->path = path;
  fm->link_to = link_to;
  fm->executable = executable;
  return fm;
}

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
  char *path;
  char* path_segment;
} path_match;

path_match* mk_path_match(char* path, char* path_segment);
path_match* mk_path_match(char* path, char* path_segment) {
  path_match* pm = alloc(sizeof(path_match));
  pm->path = strdup2(path); 
  pm->path_segment = strdup2(path_segment);
  return pm;
}

void free_path_match(path_match* pm);
void free_path_match(path_match* pm) {
  if (pm == NULL) {
    return; 
  }
  free(pm->path);
  free(pm->path_segment);
  free(pm);
}

char* canonicalize(char* path, struct stat *sb);
char* canonicalize(char* path, struct stat *sb) {
  size_t content_size = (unsigned long) sb->st_size;
  size_t str_size = content_size + 1;
  char* linkname = alloc(str_size); 
  long r = readlink(path, linkname, str_size);
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
    char* abs = mk_path(dir, linkname);
    free(linkname); 
    free(path2);
    return abs;
  }
}

int exists_as_executable(char* path);
int exists_as_executable(char* path) {
  struct stat sb;
  return stat(path, &sb) == 0 && (sb.st_mode & S_IXUSR);
}

file_match* find_file(char* path);
file_match* find_file(char* path) {
  struct stat sb;
  if (lstat(path, &sb) == 0) {
    if (S_ISLNK(sb.st_mode)) {
      //link
      char* actualpath = canonicalize(path, &sb);
      return mk_file_match(strdup2(path), 0, actualpath);
    }
    else if (S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR)) {
      //executable
      return mk_file_match(strdup2(path), 1, NULL);
    } else {
      return mk_file_match(strdup2(path), 0, NULL);
    }
  }
  return NULL;
}


path_match* find_in_path(char* command);
path_match* find_in_path(char* command) {
  if (strchr(command, '/') != NULL) {
    //don't match "/a/b" + "/" = "/a/b/"
    return NULL;
  } 
  const char* path = getenv("PATH");
  if (path == NULL) {
    error(EXIT_OTHER_ERROR, errno, "could not get PATH environment variable");
  } 
  // strtok modifies strings, so copy first
  char* path2 = strdup2(path);
  path_match* match=NULL;
  for (char* e=strtok(path2, ":"); e!=NULL; e = strtok(NULL, ":")) {
    char *fpath = mk_path(e, command);
    int exists = exists_as_executable(fpath);
    if (exists) {
      match = mk_path_match(fpath, e);
      free(fpath);
      break;
    }
    free(fpath);
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

typedef struct resolve_match {
  char* full_name;
  int unchanged;
  char pad[4];
} resolve_match;

resolve_match* mk_resolve_match(char* full_name);
resolve_match* mk_resolve_match(char* full_name) {
  resolve_match* m = malloc(sizeof(resolve_match));
  m->full_name = full_name;
  m->unchanged = 0;
  return m;
}

void free_resolve_match(resolve_match* m);
void free_resolve_match(resolve_match* m) {
  if(m == NULL) {
    return;
  }
  free(m->full_name);
  free(m);
}

type_match* find_type(char*);
type_match* find_type(char* command) {
  char* shell = getenv("SHELL");
  if (shell == NULL) {
    error(EXIT_OTHER_ERROR, errno, "failed to read 'SHELL' environment variable");
  }
  char* type_command;
  asprintf(&type_command, "%s -ic 'type %s' 2>&1", shell, command);
  FILE *fp = popen(type_command, "r");
  if (fp == NULL) {
    error(EXIT_OTHER_ERROR, errno, "failed to run shell command '%s'", type_command);
  }
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
  int child_status = pclose(fp);
  if (child_status == -1) {
    error(EXIT_OTHER_ERROR, errno, "could not wait for command '%s' to finish", type_command);
  }
  if (!WIFEXITED(child_status)) {
    error(EXIT_OTHER_ERROR, 0, "child process '%s' did no exit cleanly", type_command);
  }
  int exit_code = WEXITSTATUS(child_status);
  if (exit_code != 0 && exit_code != 1) {
    error(EXIT_OTHER_ERROR, 0, "failed to run shell command '%s', got exit code '%d'", type_command, exit_code);
  }
  free(type_command);
  return m;
}

typedef struct match {
  file_match* file_match;
  type_match *type_match;
  path_match* path_match;
  resolve_match* resolve_match;
} match;

match* mk_match(void);
match* mk_match() {
  match* m = alloc(sizeof(match));
  m->file_match=NULL;
  m->type_match=NULL;
  m->path_match=NULL;
  m->resolve_match=NULL;
  return m;
}

void free_match(match* m);
void free_match(match* m) {
  if (m == NULL) {
    return;
  }
  free_file_match(m->file_match);
  free_type_match(m->type_match);
  free_path_match(m->path_match);
  free_resolve_match(m->resolve_match);
  free(m);
}


resolve_match* resolve(char* command);
resolve_match* resolve(char* command) {
  if (command==NULL) {
    error(EXIT_OTHER_ERROR, 0, "NULL command");
  }
  char *resolved_path = realpath(command, NULL); 
  if (resolved_path == NULL) {
    free(resolved_path);
    return NULL;
  }
  resolve_match* m = mk_resolve_match(resolved_path);
  if(strcmp(command, resolved_path) == 0) {
    m->unchanged=1; 
  }
  return m;
}

static unsigned int FIND_FILE=1<<0;
static unsigned int FIND_TYPE=1<<1;
static unsigned int FIND_PATH=1<<2;
static unsigned int FIND_RESOLVE=1<<3;

match *find(char* command, unsigned int bans);
match *find(char* command, unsigned int bans) {
  match* m = mk_match();
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
  if ((bans & FIND_RESOLVE) == 0) {
    m->resolve_match = resolve(command);
    if (m->resolve_match != NULL) {
      return m;
    }
  }
  free(m);
  return NULL;
}


typedef struct cmd {
  char* name;
  struct cmd* next;
  //Fully populated?
  unsigned int done;
  unsigned int match_type;
  match* match;
} cmd;

cmd* mk_cmd(char* name, cmd* next);
cmd* mk_cmd(char* name, cmd* next) {
  cmd* c = alloc(sizeof(cmd));
  c->name = strdup2(name);
  c->next = next;
  c->done = 0;
  c->match_type = 0;
  c->match = NULL;
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
    free_match(current->match);
    cmd* next = current->next;
    free(current);
    current = next;
  } 
}

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

int find_loop(char* command);
int find_loop(char* command) {
  cmd *first = mk_cmd(command, NULL);
  cmd* current = first;
  int exit_status=EXIT_SUCCESS;
  while(current != NULL) {
    unsigned int bans = found_as(first, current->name);
    current->match = find(current->name, bans);
    match* m = current->match;
    if (m==NULL) {
      //no match, abort
      printf("no match\n");
      exit_status=EXIT_NO_MATCH;
      break;
    }
    char* next_name = NULL;
    type_match* tm = m->type_match;
    path_match* pm = m->path_match;
    file_match* fm = m->file_match;
    resolve_match* rm = m->resolve_match;
    if (tm != NULL) {
      //Command found through 'type' command in shell
      current->match_type=FIND_TYPE;
      alias_match* am = tm->alias_match;
      builtin_match* bm = tm->builtin_match;
      if (am != NULL) {
        //Alias, follow
        printf("'%s' is an alias for '%s' in shell '%s': '%s'\n", current->name, am->alias_for, am->shell,  am->declaration);
        next_name = am->alias_for;
      } else if(bm != NULL) {
        //Builtin, end
        printf("'%s' is built into shell '%s'\n", current->name, bm->shell);
        next_name = NULL;
      }
    } else if (pm != NULL) {
      //Command found in path
      printf("'%s' found in PATH as '%s'\n", current->name, pm->path);
      next_name = pm->path;
    } else if(fm != NULL) {
      //Command was straight up file
      current->match_type=FIND_FILE;
      assert(strcmp(current->name, fm->path) == 0);
      if (fm->link_to != NULL) {
        //Link, follow
        printf("'%s' is a symlink to '%s'\n", current->name, fm->link_to);
        next_name = fm->link_to;
      } else if (fm->executable) {
        //Executable, continue in case of canonical pathname 
        printf("'%s' is an executable\n", current->name);
        next_name = current->name;
      } else {
        printf("'%s' is a regular file\n", current->name);
        next_name = current->name;
      }
    } else if(rm != NULL) {
      current->match_type=FIND_RESOLVE;
      if(!rm->unchanged) { 
        printf("'%s' has canonical pathname '%s'\n", current->name, rm->full_name);
      }
      next_name = NULL;
    }
    current->done=1;
    if (next_name == NULL) {
      printf("target reached\n");
      assert(current->next == NULL);
      exit_status=EXIT_SUCCESS ;
      break;
    }
    //Will be null if next_name is null
    cmd* next = maybe_mk_cmd(next_name, NULL);
    current->next = next;
    current = next; 
  }
  free_cmds(first);
  return exit_status;
}

int main(int argc, char** argv)
{
  program_name = argv[0];
  if (argc < 2) {
    error(EXIT_OTHER_ERROR, errno, "missing command name");
  }
  char* command = argv[1];
  return find_loop(command);
}
