#ifndef MEMORY_MANAGER_H_INCLUDED
#define MEMORY_MANAGER_H_INCLUDED

#include "runtime.h"
#include "symbols_handler.h"
#include <string>

#include <boost/scoped_ptr.hpp>

namespace Freefoil {

    namespace Runtime {

        using Private::symbols_handler;
        using boost::scoped_ptr;
        using std::string;

        enum runtime_type {
            int_type,
            float_type,
            string_type,
            pointer,
        };

        class gcobject {
            gcobject(const gcobject &);
            gcobject &operator = (const gcobject &);
        protected:
            runtime_type rtt_;
            gcobject(runtime_type rtt):rtt_(rtt) {}
        public:
            virtual ~gcobject() {}
            runtime_type get_rtt() const {
                return rtt_;
            }
            void *operator new(std::size_t sz);
            void operator delete(void *address);

            virtual std::ostream &put(std::ostream &os) const{
                return os;
            }
        };

        class gcstring : public gcobject {
            scoped_ptr<string> body_scoped_ptr_;
        public:
            gcstring(const string &str)
                :gcobject(string_type), body_scoped_ptr_(new string(str)) {
            }

            void *operator new(std::size_t sz);
            void operator delete(void *address);
            //TODO: other operators

            virtual std::ostream &put(std::ostream &os) const{
                return os << *body_scoped_ptr_;
            }
        };

        inline std::ostream &operator <<(std::ostream &os, const gcobject &gcobj){
            return gcobj.put(os);
        }

        class memory_manager {

            memory_manager(const memory_manager&);
            memory_manager &operator=(const memory_manager&);

        public:
            typedef gcobject *gcobject_instance_t;
        private:
            typedef symbols_handler<gcobject_instance_t> instances_handler_t;
            instances_handler_t instances_handler_;
        public:

            void *alloc(std::size_t sz){
                return malloc(sz);
            }
            void dealloc(void *address){
                return free(address);
            }

            memory_manager() {}

            gcobject_instance_t sload(const std::string &str){
                //tries to estimate whether we have that string in data table or not
                //and allocate new string only if the presented string is a new one

                gcobject_instance_t *instance = instances_handler_.lookup(str);

                if (instance == NULL){
                    gcobject_instance_t instance = new gcstring(str);
                    instances_handler_.insert(str, instance);
                    return instance;
                }else{
                    return *instance;
                }
            }

            void function_begin(){
                instances_handler_.scope_begin();
            }

            void function_end(){
                instances_handler_.scope_end();
            }
        };
    }
}

#endif // GC_H_INCLUDED
