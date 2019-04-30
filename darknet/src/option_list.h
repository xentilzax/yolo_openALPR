#ifndef OPTION_LIST_H
#define OPTION_LIST_H
#include "list.h"

#ifdef YOLODLL_EXPORTS
#if defined(_MSC_VER)
#define YOLODLL_API __declspec(dllexport) 
#else
#define YOLODLL_API __attribute__((visibility("default")))
#endif
#else
#if defined(_MSC_VER)
#define YOLODLL_API
#else
#define YOLODLL_API
#endif
#endif

typedef struct{
    char *key;
    char *val;
    int used;
} kvp;


list *read_data_cfg(const char *filename);
int read_option(char *s, list *options);
void option_insert(list *l, char *key, char *val);
char *option_find(list *l, const char *key);
const char *option_find_str(list *l, const char *key, const char *def);
int option_find_int(list *l, const char *key, int def);
int option_find_int_quiet(list *l, const char *key, int def);
float option_find_float(list *l, const char *key, float def);
float option_find_float_quiet(list *l, const char *key, float def);
void option_unused(list *l);

typedef struct {
	int classes;
	char **names;
} metadata;

YOLODLL_API metadata get_metadata(char *file);


#endif
