#ifndef WRITER_H_INCLUDED
#define WRITER_H_INCLUDED

#include "codegen.h"
#include "function_descriptor.h"

#include <string>

namespace Freefoil {
    namespace Private {
        class writer {
        public:
            writer();
            bool write(const Private::function_shared_ptr_list_t &funcs_list, const std::string &fname);
        };
    }
}

#endif // WRITER_H_INCLUDED
