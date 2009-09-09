#ifndef SYMBOL_TABLE_H_INCLUDED
#define SYMBOL_TABLE_H_INCLUDED

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "value_descriptor.h"

namespace Freefoil {
    namespace Private {

        using std::string;
        using std::vector;
        using boost::shared_ptr;

        class symbol_table {

            const static size_t SIZE;

            class binding;
            typedef shared_ptr<binding> binding_shared_ptr;
            typedef vector<binding_shared_ptr> bindings_shared_ptr_vector_t;

            class binding {
                friend class symbol_table;
                string name_;
                value_descriptor value_descriptor_;
                binding_shared_ptr next_binding_;
            public:
                binding(const string &the_name, const value_descriptor &the_value_descriptor, const binding_shared_ptr &the_next_binding)
                        :name_(the_name), value_descriptor_(the_value_descriptor), next_binding_(the_next_binding) {   }
            };

            bindings_shared_ptr_vector_t bindings_;
            static size_t hash(const string &the_string) {
                size_t result = 0;
                for (string::const_iterator iter = the_string.begin(), iter_end = the_string.end(); iter != iter_end; ++iter) {
                    result = result * 65599;
                    result += static_cast<char>(*iter);
                }
                return result;
            }
        public:
            symbol_table() {
                bindings_.reserve(SIZE);
                binding_shared_ptr null_binding;
                for (size_t i = 0; i < SIZE; ++i) {
                    bindings_.push_back(null_binding);
                }
            }
            size_t insert(const string &the_name, const value_descriptor& the_value_descriptor) {
                const size_t index = hash(the_name);
                bindings_[index] = binding_shared_ptr(new binding(the_name, the_value_descriptor, bindings_[index]));
                return index;
            }
            value_descriptor *lookup(const string &the_name) const {
                const size_t index = hash(the_name) % SIZE;
                //return bindings_[index]->value_descriptor_;
                for (binding_shared_ptr b = bindings_[index]; b != NULL; b = b->next_binding_){
                    if (the_name == b->name_){
                        return &(b->value_descriptor_);
                    }
                }
                return NULL; //not found
           }

            void pop_buckets_head(const size_t the_index){
                bindings_[the_index] = (*bindings_[the_index]).next_binding_;
            }
        };
    }
}

#endif // SYMBOL_TABLE_H_INCLUDED
