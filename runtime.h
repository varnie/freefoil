#ifndef RUNTIME_H_INCLUDED
#define RUNTIME_H_INCLUDED

#include <vector>
#include <string>
#include <cassert>

#include <boost/shared_ptr.hpp>

namespace Freefoil {

    namespace Runtime {

        using std::vector;
        using std::string;
        using boost::shared_ptr;

        typedef signed char BYTE;
        typedef unsigned short WORD;
        typedef std::size_t ULONG;

        static const BYTE max_byte_value =  std::numeric_limits<BYTE>::max();
        static const WORD max_word_value = std::numeric_limits<WORD>::max();
        static const ULONG max_long_value = std::numeric_limits<ULONG>::max();

        class constants_pool {

            typedef vector<int> int_table_t;
            typedef vector<float> float_table_t;
            typedef vector<string> string_table_t;

            int_table_t int_table_;
            float_table_t float_table_;
            string_table_t string_table_;

        public:
            std::ptrdiff_t get_index_of_string_constant(const std::string &str) const {
                assert(std::count(string_table_.begin(), string_table_.end(), str) == 1);
                return std::distance(string_table_.begin(), std::find(string_table_.begin(), string_table_.end(), str));
            }

            std::ptrdiff_t get_index_of_float_constant(const float f) const {
                assert(std::count(float_table_.begin(), float_table_.end(), f) == 1);
                return std::distance(float_table_.begin(), std::find(float_table_.begin(), float_table_.end(), f));
            }

            std::ptrdiff_t get_index_of_int_constant(const int i) const {
                assert(std::count(int_table_.begin(), int_table_.end(), i) == 1);
                return std::distance(int_table_.begin(), std::find(int_table_.begin(), int_table_.end(), i));
            }

            std::ptrdiff_t add_int_constant(const int i) {
                if (std::count(int_table_.begin(), int_table_.end(), i) == 0) {
                    int_table_.push_back(i);
                    return int_table_.size() - 1;
                } else {
                    return get_index_of_int_constant(i);
                }
            }

            std::ptrdiff_t add_float_constant(const float f) {
                if (std::count(float_table_.begin(), float_table_.end(), f) == 0) {
                    float_table_.push_back(f);
                    return float_table_.size() - 1;
                } else {
                    return get_index_of_float_constant(f);
                }
            }

            std::ptrdiff_t add_string_constant(const std::string &str) {
                if (std::count(string_table_.begin(), string_table_.end(), str) == 0) {
                    string_table_.push_back(str);
                    return string_table_.size() - 1;
                } else {
                    return get_index_of_string_constant(str);
                }
            }

            int get_int_value_from_table(const std::size_t index) const {
                return int_table_[index];
            }

            float get_float_value_from_table(const std::size_t index) const {
                return float_table_[index];
            }

            const char *get_string_value_from_table(const std::size_t index) const {
                return string_table_[index].c_str();
            }
        };

        typedef vector<BYTE> instructions_stream_t;

        class freefoil_vm;

        class function_template{
            friend class freefoil_vm;

            BYTE args_count_;
            BYTE locals_count_;
            instructions_stream_t instructions_;
            bool void_type_; //marks whether or not function returns void
        public:
            function_template(const BYTE args_count, const BYTE locals_count, const instructions_stream_t &instructions, const bool void_type)
                :args_count_(args_count), locals_count_(locals_count), instructions_(instructions), void_type_(false)
                {}
        };

        typedef vector<function_template> function_templates_vector_t;

        class program_entry{
            friend class freefoil_vm;

            function_templates_vector_t user_funcs_;
            constants_pool constants_pool_;
            ULONG entry_point_func_index_;
            BYTE args_count_; //TODO:
        public:
            program_entry(const function_templates_vector_t& user_funcs, const constants_pool &constants, const ULONG entry_point_func_index, const BYTE args_count = 0)
                :user_funcs_(user_funcs), constants_pool_(constants), entry_point_func_index_(entry_point_func_index), args_count_(args_count)
                {}
        };

        typedef shared_ptr<program_entry> program_entry_shared_ptr;
    }
}

#endif // RUNTIME_H_INCLUDED
