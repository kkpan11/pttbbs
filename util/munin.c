/* munin program acts as a munin plugin providing statistics in shm. */
#include <stdio.h>
#include "bbs.h"

typedef struct {
    const char *name;
    unsigned int shm_index;
    const char *type;
} var_t;

typedef struct {
    const char *name;
    const var_t *vars;
    const char *title;
} module_t;

#define DEFINE_VAR(type, name) {#name, STAT_ ## name, #type}
#define DEFINE_VAR_END {NULL, 0, NULL}

static var_t useraction_vars[] = {
    DEFINE_VAR(COUNTER, VEDIT),
    DEFINE_VAR(COUNTER, TALKREQUEST),
    DEFINE_VAR(COUNTER, WRITEREQUEST),
    DEFINE_VAR(COUNTER, MORE),
    DEFINE_VAR(COUNTER, DOSEND),
    DEFINE_VAR(COUNTER, SEARCHUSER),
    DEFINE_VAR(COUNTER, THREAD),
    DEFINE_VAR(COUNTER, SELECTREAD),
    DEFINE_VAR(COUNTER, QUERY),
    DEFINE_VAR(COUNTER, DOTALK),
    DEFINE_VAR(COUNTER, PICKMYFRIEND),
    DEFINE_VAR(COUNTER, PICKBFRIEND),
    DEFINE_VAR(COUNTER, GAMBLE),
    DEFINE_VAR(COUNTER, DOPOST),
    DEFINE_VAR(COUNTER, READPOST),
    DEFINE_VAR(COUNTER, RECOMMEND),
    DEFINE_VAR(COUNTER, DORECOMMEND),
    DEFINE_VAR_END,
};

static module_t modules[] = {
    {"useraction", useraction_vars, "bbs user actions"},
    {NULL, NULL, NULL},
};

// find_module returns the pointer to module of the specified name.
static const module_t *find_module(const char *name) {
    for (int i = 0; modules[i].name; i++) {
	if (!strcmp(modules[i].name, name))
	    return &modules[i];
    }
    return NULL;
}

// config outputs munin plugin config according to the specified module.
static int config(const char *mname) {
    const module_t *m = find_module(mname);
    if (!m)
	return -1;
    printf("graph_title %s\n", m->title);
    printf("graph_order");
    for (int i = 0; m->vars[i].name; i++) {
	printf(" %s", m->vars[i].name);
    }
    printf("\n");
    for (int i = 0; m->vars[i].name; i++) {
	const var_t *v = &m->vars[i];
	printf("%s.name %s\n", v->name, v->name);
	printf("%s.label %s\n", v->name, v->name);
	printf("%s.type %s\n", v->name, v->type);
    }
    return 0;
}

// run outputs munin plugin values of the specified module.
static int run(const char *mname) {
    const module_t *m = find_module(mname);
    if (!m)
	return -1;
    attach_SHM();
    for (int i = 0; m->vars[i].name; i++) {
	const var_t *v = &m->vars[i];
	if (v->shm_index >= STAT_MAX) {
	    fprintf(stderr, "Var '%s' wants statistic[%d], which is greater than STAT_MAX %d.\n", v->name, v->shm_index, STAT_MAX);
	    return 1;
	}
	printf("%s.value %d\n", v->name, SHM->statistic[v->shm_index]);
    }
    return 0;
}

struct {
    const char *action;
    int (*func)(const char *mname);
} actions[] = {
    {"config", config},
    {"run", run},
    {NULL, NULL},
};

int main(int argc, char *argv[]) {
    const char *prog = strrchr(argv[0], '/');
    if (!prog)
	prog = argv[0];
    else
	prog++;

    if (strncmp(prog, "munin_", 6)) {
	fprintf(stderr, "Program/symlink must be prefixed with 'munin_'.\n");
	return 1;
    }

    const char *module = prog + 6;
    const char *action;
    switch (argc) {
	case 1:
	    action = "run";
	    break;
	case 2:
	    action = argv[1];
	    break;
	default:
	    fprintf(stderr, "Too many arguments.\n");
	    return 1;
    }

    int i;
    for (i = 0; actions[i].action; i++) {
	if (!strcmp(action, actions[i].action))
	    break;
    }
    if (actions[i].action)
	return actions[i].func(module);
    fprintf(stderr, "Unknown action '%s'.\n", argv[1]);
    return 1;
}