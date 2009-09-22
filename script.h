#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "AST_defs.h"

namespace Freefoil {

    using Private::tree_parse_info_t;

    class script {
        void dump_tree(const tree_parse_info_t &info) const;
    public:
        script();
        void exec();
    };
}

#endif /*SCRIPT_H_*/
