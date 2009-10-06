#ifndef SYMBOLS_HANDLER_H_INCLUDED
#define SYMBOLS_HANDLER_H_INCLUDED

#include "symbol_table.h"
#include <deque>

#include <boost/scoped_ptr.hpp>

namespace Freefoil {

    namespace Private {

        using std::deque;

        class symbols_handler {

            static const size_t SCOPE_BEGIN_MARKER;
            typedef deque<size_t> deque_t;
            typedef deque_t::iterator deque_iter_t;

            deque_t scopes_;
            deque_iter_t last_entry_start_iter_;

            symbol_table_scoped_ptr symbol_table_;
        public:
            symbols_handler(): scopes_(), symbol_table_(new symbol_table) {
                last_entry_start_iter_ = scopes_.end();
            }

            void scope_begin() {
                scopes_.push_back(SCOPE_BEGIN_MARKER); //begin marker
                last_entry_start_iter_ = scopes_.end() - 1;
            }

            void scope_end() {

                if (!scopes_.empty()) {
                    size_t index;
                    while ((index = scopes_.back()) != SCOPE_BEGIN_MARKER) {
                        //remove the head of the binding list of the indicated bucket number in symbol table
                        symbol_table_->pop_buckets_head(index);
                        scopes_.pop_back();
                    }
                    assert(index == SCOPE_BEGIN_MARKER);
                    scopes_.pop_back();
                }
            }

            bool insert(const std::string &the_name, const value_descriptor &the_value_descriptor) {

                const std::size_t bucket_index = symbol_table_->insert(the_name, the_value_descriptor);
                if (std::find(last_entry_start_iter_, scopes_.end(), bucket_index) != scopes_.end()) {
                    return false;
                }
                scopes_.push_back(bucket_index);
                return true;
            }

            value_descriptor *lookup(const string &the_name) const{
                return symbol_table_->lookup(the_name);
            }
        };
    }
}


#endif // SYMBOLS_HANDLER_H_INCLUDED
