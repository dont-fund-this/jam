#include "handler.h"

handler_def log_write_with();

handler_list log_with() {
    return {
        log_write_with(),
    };
}
