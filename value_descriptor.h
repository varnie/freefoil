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
				union {
					int int_value_;
					float float_value_;
					bool bool_value_;
					char *pChar_value_;
				} value_;

			public:
				value_descriptor(const E_VALUE_TYPE value_type)
					:value_type_(value_type)
					{}
			};
	}
}

#endif // VALUE_DESCRIPTOR_H_INCLUDED
