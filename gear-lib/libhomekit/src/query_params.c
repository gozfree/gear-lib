#include <stdlib.h>
#include <string.h>
#include "query_params.h"


query_param_t *query_params_parse(const char *s) {
    query_param_t *params = NULL;

    int i = 0;
    while (1) {
        int pos = i;
        while (s[i] && s[i] != '=' && s[i] != '&' && s[i] != '#') i++;
        if (i == pos) {
            i++;
            continue;
        }

        query_param_t *param = malloc(sizeof(query_param_t));
        param->name = strndup(s+pos, i-pos);
        param->value = NULL;
        param->next = params;
        params = param;

        if (s[i] == '=') {
            i++;
            pos = i;
            while (s[i] && s[i] != '&' && s[i] != '#') i++;
            if (i != pos) {
                param->value = strndup(s+pos, i-pos);
            }
        }

        if (!s[i] || s[i] == '#')
            break;
    }

    return params;
}


query_param_t *query_params_find(query_param_t *params, const char *name) {
    while (params) {
        if (!strcmp(params->name, name))
            return params;
        params = params->next;
    }

    return NULL;
}


void query_params_free(query_param_t *params) {
    while (params) {
        query_param_t *next = params->next;
        if (params->name)
            free(params->name);
        if (params->value)
            free(params->value);
        free(params);

        params = next;
    }
}

