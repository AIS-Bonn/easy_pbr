//My stuff
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Camera.h"
#include "example_cpp/EasyPBRwrapper.h"

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

using namespace easy_pbr;

int main(int argc, char *argv[]) {

    std::string config_file= fs::canonical(fs::path(PROJECT_SOURCE_DIR) / "./config/example.cfg").string();

    std::shared_ptr<Viewer> view = Viewer::create(config_file);  //creates it with default params
    view->m_camera->from_string("-9.23705  10.9053  6.56445  -0.180991  -0.414636 -0.0845186 0.887841 0.0233841   5.76817  -1.18873 65 0.708185 708.185");
    std::shared_ptr<EasyPBRwrapper> wrapper = EasyPBRwrapper::create(config_file, view);


    while (true) {
        view->update();
    }

    return 0;
}
