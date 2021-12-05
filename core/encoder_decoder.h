//
// Created by Михаил on 27.08.2021.
//

#ifndef GAME3X1_ENCODER_DECODER_H
#define GAME3X1_ENCODER_DECODER_H

#endif //GAME3X1_ENCODER_DECODER_H

#include <zlib.h> // zlib library

/**
 *  * A function to decode zlib-compressed data.
 * @requires <zlib.h>, "core/definitions.h"
 * @param chunk_size Buffer size. The higher value is faster decoding. Default is 32756.
 * @param input Input file name. Default is TEMP_IMAGE_DATA_FILENAME (see core/definitions.h)
 * @param output Output file name. Default is TEMP_IMAGE_RAW_DATA_FILENAME (see core/definitions.h)
 * @return @b True if success. Other cases is @b false
 */
bool decode(const std::string &input = TEMP_IMAGE_DATA_FILENAME, const std::string &output = TEMP_IMAGE_RAW_DATA_FILENAME, const unsigned int chunk_size = 32756) {
    FILE *src = fopen(input.c_str(), "rb");
    FILE *dest = fopen(output.c_str(), "wb");

    uint8_t input_buffer[chunk_size];
    uint8_t output_buffer[chunk_size];
    z_stream stream = {nullptr};

    int result = inflateInit(&stream);
    if (result != Z_OK) {
        std::cerr << "InflateInit() at core/decode_idata.h failed.\n";
        return false;
    }

    do {
        stream.avail_in = fread(input_buffer, 1, chunk_size, src);
        if (ferror(src)) {
            std::cerr << "fread() at core/decode_idata.h failed.\n";
            inflateEnd(&stream);
            return false;
        }

        if (stream.avail_in == 0)
            break;

        stream.next_in = input_buffer;

        do {
            stream.avail_out = chunk_size;
            stream.next_out = output_buffer;
            result = inflate(&stream, Z_NO_FLUSH);
            if (result == Z_NEED_DICT || result == Z_DATA_ERROR ||
                result == Z_MEM_ERROR) {
                std::cerr << "Inflate() at core/decode_idata.h failed with code " << result << '\n';
                inflateEnd(&stream);
                return false;
            }

            uint32_t nbytes = chunk_size - stream.avail_out;

            if (fwrite(output_buffer, 1, nbytes, dest) != nbytes ||
                ferror(dest)) {
                std::cerr << "fwrite() at core/decode_idata.h failed.\n";
                inflateEnd(&stream);
                return false;
            }
        } while (stream.avail_out == 0);
    } while (result != Z_STREAM_END);

    fclose(src);
    fclose(dest);

    return true;
}

bool encode(std::vector<std::vector<pixel>> &grid, unsigned int compression_level, unsigned int chunk_size) {
    FILE *src = fopen(TEMP_OUTPUT_RAW_DATA_FILENAME, "wb");
    FILE *dest = fopen(TEMP_OUTPUT_IMAGE_FILENAME, "wb");

    for (unsigned int y = 0; y < grid.size(); ++y) {
        putc('\0', src);
        for (unsigned int x = 0; x < grid[0].size(); ++x) {
            putc(grid[y][x].R, src);
            putc(grid[y][x].G, src);
            putc(grid[y][x].B, src);
        }
    }

    fclose(src);
    src = fopen(TEMP_OUTPUT_RAW_DATA_FILENAME, "r");

    uint8_t inbuff[chunk_size];
    uint8_t outbuff[chunk_size];
    z_stream stream = {0};

    if (deflateInit(&stream, compression_level) != Z_OK) {
        fprintf(stderr, "deflateInit(...) failed!\n");
        return false;
    }

    int flush;
    do {
        stream.avail_in = fread(inbuff, 1, chunk_size, src);
        if (ferror(src)) {
            fprintf(stderr, "fread(...) failed!\n");
            deflateEnd(&stream);
            return false;
        }

        flush = feof(src) ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = inbuff;

        do {
            stream.avail_out = chunk_size;
            stream.next_out = outbuff;
            deflate(&stream, flush);
            uint32_t nbytes = chunk_size - stream.avail_out;

            if (fwrite(outbuff, 1, nbytes, dest) != nbytes ||
                ferror(dest)) {
                fprintf(stderr, "fwrite(...) failed!\n");
                deflateEnd(&stream);
                return false;
            }
        } while (stream.avail_out == 0);
    } while (flush != Z_FINISH);

    deflateEnd(&stream);

    fclose(src);
    fclose(dest);
    return true;
}