#include "handler.h"

// Handler definitions
handler_def control_run_with();
handler_def control_discover_with();
handler_def control_load_with();
handler_def control_list_with();

handler_list control_with() {
    return {
        control_run_with(),
        control_discover_with(),
        control_load_with(),
        control_list_with(),
    };
}
