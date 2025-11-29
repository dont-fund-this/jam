#include "handler.h"

handler_def register_with();
handler_def list_with();

handler_list bag_with() {
    return {
        register_with(),
        list_with(),
    };
}
