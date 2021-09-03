//
// Created by Михаил on 27.08.2021.
//

#ifndef GAME3X1_INSTALL_CHECKER_H
#define GAME3X1_INSTALL_CHECKER_H

#endif //GAME3X1_INSTALL_CHECKER_H

bool correct_setup() {
    bool flag = true;

#ifndef GAME3X1_CRC_H
    std::cerr << "Missing required file core/check_setup.h";
    flag = false;
#endif

#ifndef GAME3X1_DATA_STRUCTURES_H
    std::cerr << "Missing required file core/data_structures.h";
    flag = false;
#endif

#ifndef GAME3X1_ENCODER_DECODER_H
    std::cerr << "Missing required file core/encoder_decoder.h";
    flag = false;
#endif

#ifndef GAME3X1_DEFINITIONS_H
    std::cerr << "Missing required file core/definitions.h";
    flag = false;
#endif

#ifndef GAME3X1_GARBAGE_COLLECTOR_H
    std::cerr << "Missing required file core/garbage_collector.h";
    flag = false;
#endif

#ifndef GAME3X1_IMAGE_CLASS_H
    std::cerr << "Missing required file core/image_class.h";
    flag = false;
#endif

    return flag;
}