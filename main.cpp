#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

#include <zlib.h>

#include "core/data_structures.h"
#include "core/encoder_decoder.h"
#include "core/frame_reader.h"
#include "core/filter_image.h"
#include "core/image_class.h"

#include "core/check_setup.h"

int main() {
#ifdef GAME3X1_INSTALL_CHECKER_H
    if (!correct_setup()) return -1;
#else
    std::cerr << "Critical file core/install_checker.h is missing.\n";
    return -1;
#endif

    //std::ifstream fin("debug/test_img.png");
    std::ifstream fin("debug/test_img2.png");
    IHDR file_info;

    read_header(fin);
    setup(fin, file_info);

    int length = seek_for_IDAT(fin);

    write_temporary_data(fin, length - 4);
    decode(pow(2, 15));
    std::vector<std::vector<pixel>> grid = get_pixel_grid(file_info.width, file_info.height, file_info.depth);

    Image image(grid);
    image.play(1);
    image.mesh();
    image.play();

    grid = image.get(true);
    write_tmp_img(grid, 0, pow(2, 15));

    fin.close();

    return 0;
}
