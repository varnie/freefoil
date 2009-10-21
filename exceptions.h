#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <stdexcept>
#include <string>

namespace Freefoil{
	namespace Runtime{
		using std::runtime_error;
		using std::string;

		class freefoil_exception : public runtime_error{
			public:
				explicit freefoil_exception(const string &str)
					:runtime_error(str)
					{}
		};
	}
}

#endif /*EXCEPTIONS_H_*/
