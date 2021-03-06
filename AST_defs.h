#ifndef FREEFOIL_DEFS_H_
#define FREEFOIL_DEFS_H_

#include "value_descriptor.h"

#include <string>

#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

namespace Freefoil {
    namespace Private {

        using boost::scoped_ptr;

        class node_attributes {
        public:
            enum E_FUNC_KIND{
                BUILTIN_FUNC,
                USER_FUNC
            };
        private:
            E_FUNC_KIND func_kind_;
            value_descriptor::E_VALUE_TYPE value_type_;
            value_descriptor::E_VALUE_TYPE cast_type_;
            int index_;
            bool is_ref_;
            bool lvalue_;
        public:
            node_attributes():value_type_(value_descriptor::undefinedType), cast_type_(value_descriptor::undefinedType) {}

            void set_value_type(const value_descriptor::E_VALUE_TYPE value_type) {
                value_type_ = value_type;
            }
            value_descriptor::E_VALUE_TYPE get_value_type() const {
                return value_type_;
            }
            void set_index(const int index) {
                index_ = index;
            }
            int get_index() const {
                return index_;
            }
            void set_cast(const value_descriptor::E_VALUE_TYPE cast_type) {
                cast_type_ = cast_type;
            }
            void set_func_kind(const E_FUNC_KIND func_kind) {
                func_kind_ = func_kind;
            }
            E_FUNC_KIND get_func_kind() const{
                return func_kind_;
            }
            value_descriptor::E_VALUE_TYPE get_cast() const {
                return cast_type_;
            }
        };

        using std::string;
        using BOOST_SPIRIT_CLASSIC_NS::tree_match;
        using BOOST_SPIRIT_CLASSIC_NS::node_val_data_factory;
        using BOOST_SPIRIT_CLASSIC_NS::node_iter_data_factory;
        using BOOST_SPIRIT_CLASSIC_NS::tree_parse_info;
        using BOOST_SPIRIT_CLASSIC_NS::ast_parse;
        using BOOST_SPIRIT_CLASSIC_NS::position_iterator2;

        typedef position_iterator2<std::string::const_iterator> iterator_t;
        typedef node_iter_data_factory<node_attributes> factory_t;
        typedef tree_match<iterator_t, factory_t> tree_match_t;
        typedef tree_match_t::tree_iterator iter_t;
        typedef tree_match_t::node_t node_t;
        typedef tree_parse_info<iterator_t,factory_t> tree_parse_info_t;

        inline std::string parse_str(const iter_t &iter) {
            return std::string(iter->value.begin(), iter->value.end());
        }
    }
}

#endif /*FREEFOIL_DEFS_H_*/
