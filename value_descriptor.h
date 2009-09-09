#ifndef VALUE_DESCRIPTOR_H_INCLUDED
#define VALUE_DESCRIPTOR_H_INCLUDED

namespace Freefoil {
	namespace Private {

		class value_descriptor{
			public:
				enum E_VALUE_TYPE{
					intType,
					floatType,
					boolType,
					stringType
				};
			private:
				E_VALUE_TYPE value_type_;
                int stack_offset_;
			public:
				value_descriptor(const E_VALUE_TYPE value_type, const int stack_offset)
					:value_type_(value_type), stack_offset_(stack_offset)
					{}
                int get_stack_offset() const{
                    return stack_offset_;
                }
			};
	}
}

#endif // VALUE_DESCRIPTOR_H_INCLUDED
