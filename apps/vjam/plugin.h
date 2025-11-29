#ifndef PLUGIN_H
#define PLUGIN_H

#include <stdint.h>
#include <stdbool.h>

struct LibsInfo {
    const char* plugin_type;
    const char* product;
    const char* description_long;
    const char* description_short;
    uint64_t plugin_id;
};

#endif // PLUGIN_H
