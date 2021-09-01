//
// Created by Михаил on 30.08.2021.
//

#include "crc.h"

// IF index < 155 then dead
// IF index 155-255 then alive
// IF index < 155 AND 3 around are 3 pixels 156-200 then alive with 155
// IF index >= 155 AND 2-3 around with 100-255 then no changes
// IF index >= 155 AND 1 or > 3 around with 100-255 then death

#ifndef GAME3X1_FILTER_IMAGE_H
#define GAME3X1_FILTER_IMAGE_H

#endif //GAME3X1_FILTER_IMAGE_H

void write_tmp_img(std::vector<std::vector<pixel>> &grid, unsigned int compression_level, unsigned int chunk_size) {
    signature std_sig;
    IHDR header;

    compress(grid, compression_level, chunk_size);

    std::ofstream fout("tmp.png");
    unsigned char byte_array[8];
    fout.put(std_sig.is_text_file);
    fout.put(std_sig.PNG[0]);
    fout.put(std_sig.PNG[1]);
    fout.put(std_sig.PNG[2]);
    fout.put(std_sig.CRLF[0]);
    fout.put(std_sig.CRLF[1]);
    fout.put(std_sig.DOS_EOF);
    fout.put(std_sig.LF);

    fout << '\0' << '\0' << '\0';
    fout.put(0xD);
    fout << "IHDR";
    fout << '\0' << '\0' << '\0';
    fout.put(0x10);
    fout << '\0' << '\0' << '\0';
    fout.put(0x10);
    fout.put(0x08);
    fout.put(0x02);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x90);
    fout.put(0x91);
    fout.put(0x68);
    fout.put(0x36);

    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x1);
    fout << "sRGB";
    fout.put(0x0);
    fout.put(0xAE);
    fout.put(0xCE);
    fout.put(0x1C);
    fout.put(0xE9);

    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x9);
    fout << "pHYs";
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x12);
    fout.put(0x74);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x12);
    fout.put(0x74);
    fout.put(0x1);
    fout.put(0xDE);
    fout.put(0x66);
    fout.put(0x1F);
    fout.put(0x78);

    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x4);
    fout << "gAMA";
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0xB1);
    fout.put(0x8F);
    fout.put(0x0B);
    fout.put(0xFC);
    fout.put(0x61);
    fout.put(0x05);

    std::ifstream enc_img("encoded_image.dat");
    std::ifstream data("grayscaled_image_data.dat");
    std::vector<unsigned char> tmp_chunk = {'I', 'D', 'A', 'T'};
    std::vector<unsigned char> tmp_uncompressed_chunk = {'I', 'D', 'A', 'T'};
    unsigned char byte;

    while (enc_img >> byte) {
        tmp_chunk.push_back(byte);
    }

    while (data >> byte) {
        tmp_uncompressed_chunk.push_back(byte);
    }

    enc_img.close();

    unsigned int len = tmp_chunk.size() - 4;
    fout.put((len & 0xFF000000) >> 24);
    fout.put((len & 0x00FF0000) >> 16);
    fout.put((len & 0x0000FF00) >> 8);
    fout.put((len & 0x000000FF));
    for (unsigned int i = 0; i < tmp_chunk.size(); ++i) {
        fout.put(tmp_chunk[i]);
    }

    unsigned long check = crc(tmp_chunk, tmp_chunk.size());
    fout.put((check & 0xFF000000) >> 24);
    fout.put((check & 0x00FF0000) >> 16);
    fout.put((check & 0x0000FF00) >> 8);
    fout.put((check & 0x000000FF));

    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout.put(0x0);
    fout << "IEND";
    fout.put(0xAE);
    fout.put(0x42);
    fout.put(0x60);
    fout.put(0x82);


    fout.close();
    return;
}