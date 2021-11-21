//
// Created by Михаил on 02.09.2021.
//

#ifndef GAME3X1_GARBAGE_COLLECTOR_H
#define GAME3X1_GARBAGE_COLLECTOR_H

#endif //GAME3X1_GARBAGE_COLLECTOR_H

void cleanup() {
    std::remove(TEMP_IMAGE_RAW_DATA_FILENAME);
    std::remove(TEMP_IMAGE_DATA_FILENAME);
    std::remove(TEMP_OUTPUT_IMAGE_FILENAME);
    std::remove(TEMP_OUTPUT_RAW_DATA_FILENAME);
}