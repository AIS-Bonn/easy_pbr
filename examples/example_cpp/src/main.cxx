//My stuff
#include "easy_pbr/Viewer.h"
#include "example_cpp/EasyPBRwrapper.h"


int main(int argc, char *argv[]) {

    std::string config_file="./config/example.cfg";

    std::shared_ptr<Viewer> view = Viewer::create();  //creates it with default params

    std::shared_ptr<EasyPBRwrapper> wrapper = EasyPBRwrapper::create(config_file, view);

    while (true) {
        view->update();
    }

    return 0;
}
