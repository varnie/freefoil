#ifndef SCOPE_STACK_H_INCLUDED
#define SCOPE_STACK_H_INCLUDED

#include "exceptions.h"

#include <deque>
#include <assert.h>
#include "symbol_table.h"

namespace Freefoil{
    namespace Private{

            using std::deque;

            class scope_stack{
                typedef deque<size_t> deque_t;
                deque_t scopes_;
                static const size_t SCOPE_BEGIN_MARKER;
                symbol_table *psymbol_table_;
                deque_t::iterator last_entry_start_iter_;
            public:
                scope_stack(): scopes_(), psymbol_table_(NULL){
                }

                void begin_scope(){
                        scopes_.push_back(SCOPE_BEGIN_MARKER); //begin marker
                        last_entry_start_iter_ = scopes_.end() - 1;
                }

                void end_scope(){
                    assert(psymbol_table_ != NULL);

                    if (!scopes_.empty()){
                        size_t index;
                        while ((index = scopes_.back()) != SCOPE_BEGIN_MARKER){
                                //remove the head of the binding list of the indicated bucket number in symbol table
                                psymbol_table_->pop_buckets_head(index);
                                scopes_.pop_back();
                        }
                        assert(index == SCOPE_BEGIN_MARKER);
                        scopes_.pop_back();
                    }
                }

                //occurs when "insert" new variable's declaration in symbol table
                void push_bucket_index(size_t the_index, const std::string &the_name){
                    if (std::find(last_entry_start_iter_, scopes_.end(), the_index) != scopes_.end()){
                            throw freefoil_exception("redeclaration of variable " + the_name);
                    }

                    scopes_.push_back(the_index);
                }

                void attach_symbol_table(symbol_table *psymbol_table){
                    psymbol_table_ = psymbol_table;
                }
            };
    }
}

#endif // SCOPE_STACK_H_INCLUDED
