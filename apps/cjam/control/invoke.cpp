#include "invoke.h"
#include "bind.h"
#include <iostream>

const char* control_invoke(const ControlFns& fns) {
    const char* result = fns.invoke("control.run", "{}", "{}");
    std::cout << "Control plugin result: " << (result ? result : "null") << std::endl;
    return result;
}
