#include "writer.h"
#include <fstream>

#include "boost/filesystem.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace Freefoil {

    namespace Private {

        using std::ofstream;

        writer::writer() {}

        bool writer::write(const Private::function_shared_ptr_list_t  &parsed_funcs_list, const std::string &fname) {

            static const boost::filesystem::path output_directory_name("output", boost::filesystem::native);
            static const std::string signature("ffl");

            if (!boost::filesystem::exists(output_directory_name) or !boost::filesystem::is_directory(output_directory_name)) {
                boost::filesystem::create_directory(boost::filesystem::path(output_directory_name));
            }

            const boost::filesystem::path file_path(output_directory_name / fname);
            if (boost::filesystem::exists(file_path)) {
                boost::filesystem::remove(file_path);
            }

            ofstream ofs((output_directory_name.string() + "/bytecode.ffl").c_str());

          //  ofs << signature;
            ofs << parsed_funcs_list.size();

            boost::archive::binary_oarchive oa(ofs, boost::archive::no_header);
            for (Private::function_shared_ptr_list_t::const_iterator cur_iter = parsed_funcs_list.begin(), iter_end = parsed_funcs_list.end(); cur_iter != iter_end; ++cur_iter) {
                //serialize function
                oa << **cur_iter;
            }

            ofs.close();

            return true;
        }

    }
}
