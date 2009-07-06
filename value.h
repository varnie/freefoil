#ifndef VALUE_H_
#define VALUE_H_


namespace Freefoil {
	namespace Private {
		
class value{
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
			//TODO: add char  *pChar_value_;
		} value_;
		
	public:
		value(E_VALUE_TYPE value_type)
			:value_type(value_type)
			{}
		virtual ~value();
	};

	}
}
#endif /*VALUE_H_*/
