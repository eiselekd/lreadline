/* BSD-license */

#include "lua.h"
#include "lauxlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <locale.h>
#include <stdio.h>
#include <setjmp.h>

#include <readline/readline.h>
#include <readline/history.h>

#define DEBUG
#ifdef DEBUG
static FILE *fout = 0;
#define DBGP(fmt,...) if (fout) { fprintf(fout , fmt, ##__VA_ARGS__); fflush(fout); }
#define DBGPSTR(a) do { int i=0, c; while(a && (c = a[i++])) { if (isprint(c) && (c!= '\n' || c != '\r')) { DBGP("%c",c) } else { DBGP("\\%02x",c);} } } while (0)
#else
#define DBGP(fmt,...)
#define DBGPSTR(a)
#endif
#define USESIG
#define RESTORE_LOCALE(sl) { setlocale(LC_CTYPE, sl); free(sl); }

#define MYNAME "lreadline"
#define MYVERSION MYNAME " library for " LUA_VERSION
#define MYTYPE "lreadline"
static int completion_func_ref = -1;
static char *logfile = ".lreadline.txt";

static char *
on_completion(const char *text, int state)
{
    char *result = NULL;

    DBGP("[+] on_completion %d: '", state);
    DBGPSTR(text);
    DBGP("'\n");

    if (completion_func_ref != -1) {
        return result;
    }
    return result;
}

static char **
lreadline_complete(const char *text, int start, int end)
{
    DBGP("[+] lreadline_complete %d-%d: '", start, end);
    DBGPSTR(text);
    DBGP("'\n");
    rl_completion_append_character ='\0';
    rl_completion_suppress_append = 0;

    rl_attempted_completion_over = 1;

    return rl_completion_matches(text, on_completion);
}

static int colorize = 1;
static const char *colors[] = {"\033[0m",
                               "\033[0;31m",
                               "\033[1;31m",
                               "\033[0;32m",
                               "\033[1;32m",
                               "\033[0;33m",
                               "\033[1;33m",
                               "\033[1m",
                               "\033[22m"};

#define print_output(...) fprintf (stdout, __VA_ARGS__), fflush(stdout)
#define COLOR(i) (colorize ? colors[i] : "")

static void display_matches (char **matches, int num_matches, int max_length)
{
    print_output ("%s", COLOR(7));
    rl_display_match_list (matches, num_matches, max_length);
    print_output ("%s", COLOR(0));
    rl_on_new_line ();
}

static char *generator (const char *text, int state)
{
    static int which;
    char *match = NULL;

    DBGP("[+] generator %d: '", state);
    DBGPSTR(text);
    DBGP("'\n");

    if (state == 0) {
        which = 0;
    }
    return 0;
}

static void
setup_readline(void)
{
    char *sl = strdup(setlocale(LC_CTYPE, NULL));
    DBGP("[+] setup_readline\n");

    using_history();

    rl_readline_name = "lua";
    rl_bind_key('\t', rl_complete);
    /* Bind both ESC-TAB and ESC-ESC to the completion function */
    rl_bind_key_in_map ('\t', rl_complete, emacs_meta_keymap);
    rl_bind_key_in_map ('\033', rl_complete, emacs_meta_keymap);

    /* Set our completion function */
    //rl_attempted_completion_function = lreadline_complete;

    rl_completion_entry_function = generator;
    rl_completion_display_matches_hook = display_matches;

    /* Set word break characters */
    rl_completer_word_break_characters =
        strdup(" \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?");
    /* All nonalphanums except '.' */

    /*
    begidx = PyInt_FromLong(0L);
    endidx = PyInt_FromLong(0L);
    */

    if (!isatty(STDOUT_FILENO)) {
        rl_variable_bind ("enable-meta-key", "off");
    }

    rl_initialize();

    if (logfile) {
        read_history (logfile);
    }

    if (sl) {
        setlocale(LC_CTYPE, sl);
        free(sl);
    }
}

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
/*** Adapted from Lua 5.2.0 */
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup)
{
    luaL_checkstack(L, nup+1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup+1));
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3));
    }
    lua_pop(L, nup);  /* remove upvalues */
}
#endif

static int
lreadline_log(lua_State* L)
{
    size_t l;
    const char *b = luaL_checklstring(L, 1, &l);
    if (b)
        DBGP("%s", b);
    return 1;
}

typedef void (*sighandler_t)(int);
sighandler_t
setsig(int sig, sighandler_t handler)
{
#ifdef USESIG
    struct sigaction context, ocontext;
    context.sa_handler = handler;
    sigemptyset(&context.sa_mask);
    context.sa_flags = 0;
    if (sigaction(sig, &context, &ocontext) == -1)
        return SIG_ERR;
    return ocontext.sa_handler;
#endif
}

static jmp_buf jbuf;

/* ARGSUSED */
static void
onintr(int sig)
{
    longjmp(jbuf, 1);
}

static int
lreadline_input(lua_State* L)
{
    size_t l; int sig = 0;
    const char *b = luaL_checklstring(L, 1, &l);
    char *i = 0;
    //if (b)
    //    DBGP("%s", b);
    if (!b)
        b = (char *)">";

    size_t n;
    char *p, *q;
    int signal;

    char *saved_locale = strdup(setlocale(LC_CTYPE, NULL));
    setlocale(LC_CTYPE, "");

    /*
    if (stdin != rl_instream || stdout != rl_outstream) {
        rl_instream = stdin;
        rl_outstream = stdout;
        rl_prep_terminal (1);
        }*/

    sighandler_t old_inthandler = setsig(SIGINT, onintr);
    if (setjmp(jbuf)) {
        RESTORE_LOCALE(saved_locale);
        lua_pushnil(L);
        lua_pushnumber(L, 1);
        return 2;
    }
    p = readline(b);
    setsig(SIGINT, old_inthandler);

    if (p == NULL) {
        RESTORE_LOCALE(saved_locale);
        lua_pushstring(L, "");
        lua_pushnumber(L, 0);
        return 2;
    }

    n = strlen(p);
    if (n > 0) {
        const char *line;
        HISTORY_STATE *hist_st = history_get_history_state();
        int length = hist_st->length;
        free(hist_st);

        if (length > 0) {
            HIST_ENTRY *hist_ent;
            hist_ent = history_get(length);
            line = hist_ent ? hist_ent->line : "";
        } else
            line = "";
        if (strcmp(p, line))
            add_history(p);
    }
    lua_pushstring(L, p);
    lua_pushnumber(L, 0);

    free(p);

    RESTORE_LOCALE(saved_locale);
    return 2;

err:
    return luaL_error(L, "Cannot get line");
}

static int
lreadline_set_completer(lua_State* L)
{
    if (!lua_isfunction(L, 1)) {
        return luaL_error(L, "Expecting function");
    }
    if (completion_func_ref != -1) {
        luaL_unref(L, LUA_REGISTRYINDEX, completion_func_ref);
    }
    completion_func_ref = -1;
    if (!lua_isnil (L, 1)) {
        lua_pushvalue(L, 1);
        completion_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

static int
lreadline_read_history(lua_State* L)
{
    size_t l;
    const char *s = ".readline_history.txt";
    const char *b = luaL_checklstring(L, 1, &l);
    if (b)
        s = b;
    int r = read_history(s);
    return 0;
}

static int
lreadline_write_history(lua_State* L)
{
    size_t l;
    const char *s = ".readline_history.txt";
    const char *b = luaL_checklstring(L, 1, &l);
    if (b)
        s = b;
    int r = write_history(s);
    return 0;
}

static const luaL_Reg lreadlinelib[] = {
    {"log", lreadline_log},
    {"_read_history", lreadline_read_history},
    {"_write_history", lreadline_write_history},
    {"_input", lreadline_input},
    {"_set_completer", lreadline_set_completer},
    {NULL, NULL}
};

static void finish ()
{
    if (logfile) {
        write_history (logfile);
    }
}

LUALIB_API int luaopen_lreadline_core (lua_State *L)
{

#ifdef DEBUG
    //fout = fopen("/tmp/lreadline.txt", "w");
    fout = stdout;
#endif

    luaL_newmetatable(L,MYTYPE);
    luaL_setfuncs(L,lreadlinelib,0);
    lua_pushliteral(L,"version");/** version */
    lua_pushliteral(L,MYVERSION);
    lua_settable(L,-3);
    lua_pushliteral(L,"__index");
    lua_pushvalue(L,-2);
    lua_settable(L,-3);

    setup_readline();

    atexit (finish);

    return 1;
}

/*
  Local Variables:
  c-basic-offset:4
  c-file-style:"bsd"
  indent-tabs-mode:nil
  End:
*/
