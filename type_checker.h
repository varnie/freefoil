#ifndef TYPE_CHECKER_H_INCLUDED
#define TYPE_CHECKER_H_INCLUDED

#include "AST_defs.h"
#include "value_descriptor.h"

namespace Freefoil {

    namespace Private {

        class type_checker {

            bool is_assignable(const value_descriptor::E_VALUE_TYPE the_type1, const value_descriptor::E_VALUE_TYPE the_type2) const {
                //TODO: return that the_type2 is a "subtype" of the_type1

            }




            value_descriptor::E_VALUE_TYPE get_bool_term_type(const iter_t &iter) const {


            }

            value_descriptor::E_VALUE_TYPE get_bool_expr_type(const iter_t& iter) const {
                assert(iter->value.id() == freefoil_grammar::bool_expr_ID);

                const value_descriptor::E_VALUE_TYPE type1 = parse_bool_term(iter->children.begin());
                if (iter->children.begin() + 1 != iter->children.end()) {
                    const value_descriptor::E_VALUE_TYPE type2 = parse_or_tail(iter->children.begin() + 1);

                }else{
                    return type1;
                }
            }

        public:

            bool is_subtype(const value_descriptor::E_VALUE_TYPE the_type1, const value_descriptor::E_VALUE_TYPE the_type2) const{
                //TODO:
            }

            static bool check_type(const value_descriptor::E_VALUE_TYPE the_type, const iter_t& iter) const {

                switch (iter->value.id().to_long()) {
                case freefoil_grammar::bool_expr_ID: {
                    return is_assignable(the_type, get_bool_expr_type());
                }

                }

                //TODO:
            }
        };
    }
}

#endif // TYPE_CHECKER_H_INCLUDED
