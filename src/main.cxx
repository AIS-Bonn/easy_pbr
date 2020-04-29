//My stuff
#include "easy_pbr/Viewer.h"

using namespace easy_pbr;

int main(int argc, char *argv[]) {

    std::shared_ptr<Viewer> view = Viewer::create("./config/default_params.cfg"); //need to pass the size so that the viewer also knows the size of the viewport

    while (true) {
        view->update();
    }

    return 0;
}
