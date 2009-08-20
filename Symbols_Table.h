#ifndef SYMBOLS_TABLE_H_INCLUDED
#define SYMBOLS_TABLE_H_INCLUDED

#include <string>

namespace Freefoil{

 namespace Private{
        class Symbols_Table{
            class Bucket{
                public:

            };

            public:
                void add_scope();
                void remove_scope();
                void add_name(const std::string &the_name);

        };
 }
}

#endif // SYMBOLS_TABLE_H_INCLUDED
