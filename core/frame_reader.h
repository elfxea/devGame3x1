//
// Created by Михаил on 27.08.2021.
//

#ifndef GAME3X1_FRAME_READER_H
#define GAME3X1_FRAME_READER_H

void read_header(std::ifstream &FILE) {
    unsigned char byte_array[8];
    for (char i = 0; i < 8; ++i)
        byte_array[i] = FILE.get();

    signature std_signature;
    signature current_signature(byte_array);

    if (std_signature != current_signature)
        throw std::runtime_error("Unable to read PNG. It may be damaged.\n");
}

void setup(std::ifstream &FILE, IHDR &needle) {
    FILE.seekg(0x10, std::ios_base::beg);
    for (char i = 0; i < 4; ++i) {
        needle.width = needle.width << 8;
        needle.width += FILE.get();
    }
    for (char i = 0; i < 4; ++i) {
        needle.height = needle.height << 8;
        needle.height += FILE.get();
    }
    needle.depth += FILE.get();
    needle.color_type = FILE.get();
    needle.compression_type = FILE.get();
    needle.filter_type = FILE.get();
    needle.interlace = FILE.get();


    FILE.seekg(4, std::ios_base::cur);
}

int seek_for_IDAT(std::ifstream &FILE) {
    unsigned int chunk_size = 0;
    unsigned char chunk_name[4];
    for (char i = 0; i < 4; ++i) {
        chunk_size = chunk_size << 8;
        chunk_size += FILE.get();
    }
    for (char i = 0; i < 4; ++i) {
        chunk_name[i] = FILE.get();
    }
    if (chunk_name[0] != 'I'
    && chunk_name[1] != 'D'
    && chunk_name[2] != 'A'
    && chunk_name[3] != 'T') {
        FILE.seekg(chunk_size + 4, std::ios_base::cur);
        return seek_for_IDAT(FILE);
    }
    return chunk_size;
}


#endif //GAME3X1_FRAME_READER_H

