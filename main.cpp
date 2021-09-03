#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

#include <zlib.h>

#include "core/definitions.h"
#include "core/data_structures.h"
#include "core/encoder_decoder.h"
#include "core/image_class.h"
#include "core/garbage_collector.h"

#include "core/check_setup.h"

int main() {
#ifdef GAME3X1_INSTALL_CHECKER_H
    if (!correct_setup()) return -1;
#else
    std::cerr << "Critical file core/install_checker.h is missing.\n";
    return -1;
#endif

    std::ifstream fin("debug/test_img2.png");
    Image image(fin);
    std::string filename;

    filename = "output/res.png";
    image.write(filename);

    fin.close();

    cleanup();

    return 0;
}
