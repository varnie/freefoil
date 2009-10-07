#ifndef SCRIPT_H_
#define SCRIPT_H_

#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML 1

#include "AST_defs.h"
#include "tree_analyzer.h"
#include "codegen.h"
#include "freefoil_vm.h"

#include <string>

namespace Freefoil {

    using Private::tree_parse_info_t;
    using Private::tree_analyzer;
    using Private::codegen;
    using std::string;

    class compiler {
        tree_parse_info_t the_parse_info;
        tree_analyzer the_tree_analyzer;
        codegen the_codegen;

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
        void dump_tree(const tree_parse_info_t &info) const;
#endif
        bool parse(const string &program_source, tree_parse_info_t &result_info);
    public:
        compiler();
        void exec(const string &source);
    };
}

#endif /*SCRIPT_H_*/
