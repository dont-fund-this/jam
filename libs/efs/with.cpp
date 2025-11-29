#include "handler.h"

extern handler_def efs_list_with();
extern handler_def efs_read_with();

handler_list efs_with() {
    return {
        efs_list_with(),
        efs_read_with(),
    };
}
