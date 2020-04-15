//My stuff
#include "easy_pbr/Viewer.h"
#include "example_cpp/EasyPBRwrapper.h"

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

using namespace easy_pbr;

int main(int argc, char *argv[]) {

    std::string config_file= fs::canonical(fs::path(PROJECT_SOURCE_DIR) / "./config/example.cfg").string();

    std::shared_ptr<Viewer> view = Viewer::create(config_file);  //creates it with default params
    std::shared_ptr<EasyPBRwrapper> wrapper = EasyPBRwrapper::create(config_file, view);

    while (true) {
        view->update();
    }

    return 0;
}
