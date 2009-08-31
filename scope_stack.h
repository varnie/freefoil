#ifndef SCOPE_STACK_H_INCLUDED
#define SCOPE_STACK_H_INCLUDED

#include <stack>
#include <assert.h>
#include "symbol_table.h"

namespace Freefoil{
    namespace Private{

            using std::stack;

            class scope_stack{
                stack<size_t> scopes_;
                static const size_t SCOPE_BEGIN_MARKER;
                symbol_table *psymbol_table_;
            public:
                scope_stack():psymbol_table_(NULL){
                }

                void begin_scope(){
                        scopes_.push(SCOPE_BEGIN_MARKER); //begin marker
                }
                void end_scope(){
                    assert(psymbol_table_ != NULL);

                    if (!scopes_.empty()){
                        size_t index;
                        while ((index = scopes_.top()) != SCOPE_BEGIN_MARKER){
                                //TODO: remove the head of the binding list of the indicated bucket number in symbol table
                                psymbol_table_->pop_buckets_head(index);
                                scopes_.pop();
                        }
                        assert(index == SCOPE_BEGIN_MARKER);
                        scopes_.pop();
                    }
                }
                //occures when "insert" new variable's declaration in symbol table
                void push_bucket_index(const size_t the_index){
                    scopes_.push(the_index);
                }

                void attach_symbol_table(symbol_table *psymbol_table){
                    psymbol_table_ = psymbol_table;
                }
            };
    }
}

#endif // SCOPE_STACK_H_INCLUDED
