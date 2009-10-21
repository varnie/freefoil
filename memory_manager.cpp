#include "memory_manager.h"

namespace Freefoil{
        namespace Runtime{
            memory_manager g_mm;    //global instance

            void *gcobject::operator new(std::size_t sz){
                return g_mm.alloc(sz);
            }
            void gcobject::operator delete(void *address){
                g_mm.dealloc(address);
            }

            void *gcstring::operator new(std::size_t sz){
                return g_mm.alloc(sz);
            }
            void gcstring::operator delete(void *address){
                g_mm.dealloc(address);
            }

        }
}
