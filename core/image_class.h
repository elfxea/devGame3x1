//
// Created by Михаил on 01.09.2021.
//
#include "crc.h"
#include "chunk_class.h"

#ifndef GAME3X1_IMAGE_CLASS_H
#define GAME3X1_IMAGE_CLASS_H

#endif //GAME3X1_IMAGE_CLASS_H

class Image {
private:
    std::vector<std::vector<pixel>> grid_;
    std::vector<std::vector<pixel>> grayscale_grid_;

    unsigned long width_ = 0;
    unsigned long height_ = 0;

    IHDR source_info;

    bool grayscaled = false;

    static void read_std_header(std::ifstream &from) {
        unsigned char is_text_file = from.get();
        unsigned char P = from.get();
        unsigned char N = from.get();
        unsigned char G = from.get();
        unsigned char CRLF1 = from.get();
        unsigned char CRLF2 = from.get();
        unsigned char DOS_EOF = from.get();
        unsigned char LF = from.get();

        if (is_text_file != 0x89 ||
            P != 'P' ||
            N != 'N' ||
            G != 'G' ||
            CRLF1 != 0x0D ||
            CRLF2 != 0x0A ||
            DOS_EOF != 0x1A ||
            LF != 0x0A)
            throw std::runtime_error("Provided file is not a PNG.");
    }

    static bool check_frame(std::vector<unsigned char> &data, unsigned int checksum) {
        unsigned int calculated_checksum = crc(data);
        if (checksum != calculated_checksum)
            return false;
        return true;
    }

    static std::vector<unsigned char> read_chunk(std::ifstream &from) {
        unsigned int chunk_size = 0;
        std::string chunk_name;
        std::vector<unsigned char> data;
        std::vector<unsigned char> full_chunk;
        unsigned int checksum = 0;

        for (char i = 0; i < 4; ++i) {
            chunk_size = chunk_size << 8;
            chunk_size += from.get();
        }

        for (char i = 0; i < 4; ++i) {
            chunk_name.push_back(from.get());
            full_chunk.push_back(chunk_name[i]);
        }

        for (unsigned int i = 0; i < chunk_size; ++i) {
            data.push_back(from.get());
            full_chunk.push_back(data[i]);
        }

        for (char i = 0; i < 4; ++i) {
            checksum = checksum << 8;
            checksum += from.get();
        }

        if (!check_frame(full_chunk, checksum))
            throw std::runtime_error("Unable to read frame. It may be damaged.");
        return full_chunk;
    }

    void setup(std::ifstream &from) {
        std::vector<unsigned char> data = read_chunk(from);
        if (data[0] != 'I' ||
            data[1] != 'H' ||
            data[2] != 'D' ||
            data[3] != 'R')
            throw std::runtime_error("Unable to read PNG. IHDR chunk must appear first.");

        for (unsigned int i = 4; i < 8; ++i) {
            source_info.width = source_info.width << 8;
            source_info.width += data[i];
        }

        width_ = source_info.width;

        for (unsigned int i = 8; i < 12; ++i) {
            source_info.height = source_info.height << 8;
            source_info.height += data[i];
        }

        height_ = source_info.height;

        source_info.depth = data[12];
        source_info.color_type = data[13];
        source_info.compression_type = data[14];
        source_info.filter_type = data[15];
        source_info.interlace = data[16];

        if (pow(2, source_info.depth) != BITDEPTH) {
            throw std::runtime_error("Unable to read images with bit depth != 8 bit.");
        }
    }

    void grayscale() {
        for (unsigned int y = 0; y < height_; ++y) {
            for (unsigned int x = 0; x < width_; ++x) {
                unsigned int color_index = grid_[y][x].R + grid_[y][x].G + grid_[y][x].B;
                color_index /= 3;
                grayscale_grid_[y][x].R = color_index;
                grayscale_grid_[y][x].G = color_index;
                grayscale_grid_[y][x].B = color_index;
            }
        }
        grayscaled = true;
    }

    void append_grayscaled() {
        for (unsigned int y = 0; y < height_; ++y) {
            for (unsigned int x = 0; x < width_; ++x) {
                unsigned int index = grayscale_grid_[y][x].R / DEATH_INDEX;
                grid_[y][x].R = (grid_[y][x].R * index) % BITDEPTH;
                grid_[y][x].G = (grid_[y][x].G * index) % BITDEPTH;
                grid_[y][x].B = (grid_[y][x].B * index) % BITDEPTH;
            }
        }
        grayscaled = false;
    }

    static std::vector<unsigned char> transfer_to_size_t(const unsigned int needle) {

        std::vector<unsigned char> result;
        result.push_back((needle & 0xFF000000) >> 24);
        result.push_back((needle & 0x00FF0000) >> 16);
        result.push_back((needle & 0x0000FF00) >> 8);
        result.push_back((needle & 0x000000FF));

        return result;
    }

    static void build_std_header(std::ofstream &at) {
        at.put(0x89); // non-text file
        at.put('P');
        at.put('N');
        at.put('G');
        at.put(0x0D); // Line breakers
        at.put(0x0A);
        at.put(0x1A);
        at.put(0x0A);
    }

    static void build_chunk(std::ofstream &at, const std::string &chunk_name, const std::vector<unsigned char> &data) {
        Chunk tmp(chunk_name);
        tmp.push(data);
        for (unsigned char data_byte : tmp.get())
            at.put(data_byte);
    }

    static void read_png(std::ifstream &from) {
        std::ofstream tmp(TEMP_IMAGE_DATA_FILENAME);
        while (!from.eof()) {
            std::vector<unsigned char> data = read_chunk(from);
            if (
                    data[0] == 'I' &&
                    data[1] == 'D' &&
                    data[2] == 'A' &&
                    data[3] == 'T'
                    ) {
                for (unsigned int i = 0; i < 4; ++i) // remove chunk name
                    data.erase(data.begin());
                for (unsigned char i : data) {
                    tmp.put(i);
                }
                data.clear();
            } else if (
                    data[0] == 'I' &&
                    data[1] == 'E' &&
                    data[2] == 'N' &&
                    data[3] == 'D'
                    )
                break;
        }
        tmp.close();
        decode(32756);
    }

    pixel paeth_predictor(int y, int x) {
        pixel current = grid_[y][x];
        pixel left = grid_[y][x - 1];
        pixel above = grid_[y - 1][x];
        pixel above_left = grid_[y - 1][x - 1];
        
        unsigned char paeth_red = 0;
        unsigned char paeth_green = 0;
        unsigned char paeth_blue = 0;
        
        paeth_red += left.R;
        paeth_red += above.R;
        paeth_red -= above_left.R;
        
        paeth_green += left.G;
        paeth_green += above.G;
        paeth_green -= above_left.G;
        
        paeth_blue += left.B;
        paeth_blue += above.B;
        paeth_blue -= above_left.B;

        unsigned char paeth_left_red = paeth_red > left.R ? paeth_red - left.R : left.R - paeth_red;
        unsigned char paeth_left_green = paeth_green > left.G ? paeth_green - left.G : left.G - paeth_green;
        unsigned char paeth_left_blue = paeth_blue > left.B ? paeth_blue - left.B : left.B - paeth_blue;

        unsigned char paeth_above_red = paeth_red > above.R ? paeth_red - above.R : above.R - paeth_red;
        unsigned char paeth_above_green = paeth_green > above.G ? paeth_green - above.G : above.G - paeth_green;
        unsigned char paeth_above_blue = paeth_blue > above.B ? paeth_blue - above.B : above.B - paeth_blue;

        unsigned char paeth_above_left_red = paeth_red > above_left.R ? paeth_red - above_left.R : above_left.R - paeth_red;
        unsigned char paeth_above_left_green = paeth_green > above_left.G ? paeth_green - above_left.G : above_left.G - paeth_green;
        unsigned char paeth_above_left_blue = paeth_blue > above_left.B ? paeth_blue - above_left.B : above_left.B - paeth_blue;
        
        unsigned char red;
        unsigned char green;
        unsigned char blue;

        if ((paeth_left_red <= paeth_above_red) && paeth_left_red <= paeth_above_left_red)
            red = left.R;
        else if (paeth_above_red <= paeth_above_left_red)
            red = above.R;
        else
            red = above_left.R;

        if ((paeth_left_green <= paeth_above_green) && paeth_left_green <= paeth_above_left_green)
            green = left.G;
        else if (paeth_above_green <= paeth_above_left_green)
            green = above.G;
        else
            green = above_left.G;

        if ((paeth_left_blue <= paeth_above_blue) && paeth_left_blue <= paeth_above_left_blue)
            blue = left.B;
        else if (paeth_above_blue <= paeth_above_left_blue)
            blue = above.B;
        else
            blue = above_left.B;
        
        pixel tmp(red, green, blue);
        return tmp;
    }

    void filter_pixel(unsigned int y, unsigned int x, row_filter_type filter_type) {
        switch (filter_type) {
            case NONE: {
                break;
            }
            case SUB: {
                if (x == 0)
                    break;
                grid_[y][x].R += grid_[y][x - 1].R;
                grid_[y][x].R %= 256;
                grid_[y][x].G += grid_[y][x - 1].G;
                grid_[y][x].G %= 256;
                grid_[y][x].B += grid_[y][x - 1].B;
                grid_[y][x].B %= 256;
                grid_[y][x].A += grid_[y][x - 1].A;
                break;
            }
            case UP: {
                if (y == 0)
                    break;
                grid_[y][x].R += grid_[y - 1][x].R;
                grid_[y][x].R %= 256;
                grid_[y][x].G += grid_[y - 1][x].G;
                grid_[y][x].G %= 256;
                grid_[y][x].B += grid_[y - 1][x].B;
                grid_[y][x].B %= 256;
                grid_[y][x].A += grid_[y - 1][x].A;
                break;
            }
            case AVERAGE: {
                if (y == 0) {
                    if (x == 0)
                        break;
                    grid_[y][x].R += grid_[y][x - 1].R / 2;
                    grid_[y][x].R %= 256;
                    grid_[y][x].G += grid_[y][x - 1].G / 2;
                    grid_[y][x].G %= 256;
                    grid_[y][x].B += grid_[y][x - 1].B / 2;
                    grid_[y][x].B %= 256;
                    grid_[y][x].A += grid_[y][x - 1].A / 2;
                } else if (x == 0) {
                    grid_[y][x].R += grid_[y - 1][x].R / 2;
                    grid_[y][x].R %= 256;
                    grid_[y][x].G += grid_[y - 1][x].G / 2;
                    grid_[y][x].G %= 256;
                    grid_[y][x].B += grid_[y - 1][x].B / 2;
                    grid_[y][x].B %= 256;
                    grid_[y][x].A += grid_[y - 1][x].A / 2;
                } else {
                    grid_[y][x].R += (grid_[y - 1][x].R + grid_[y][x - 1].R) / 2;
                    grid_[y][x].R %= 256;
                    grid_[y][x].G += (grid_[y - 1][x].G + grid_[y][x - 1].G) / 2;
                    grid_[y][x].G %= 256;
                    grid_[y][x].B += (grid_[y - 1][x].B + grid_[y][x - 1].B) / 2;
                    grid_[y][x].B %= 256;
                    grid_[y][x].A += (grid_[y - 1][x].A + grid_[y][x - 1].A) / 2;
                }
                break;
            }
            case PAETH: {
                pixel prediction = paeth_predictor(y, x);

                if (x != 0) {
                    grid_[y][x].R += prediction.R;
                    grid_[y][x].R %= 256;
                    grid_[y][x].G += prediction.G;
                    grid_[y][x].G %= 256;
                    grid_[y][x].B += prediction.B;
                    grid_[y][x].B %= 256;
                } else {
                    grid_[y][x].R += grid_[y - 1][x].R;
                    grid_[y][x].R %= 256;
                    grid_[y][x].G += grid_[y - 1][x].G;
                    grid_[y][x].G %= 256;
                    grid_[y][x].B += grid_[y - 1][x].B;
                    grid_[y][x].B %= 256;
                }
                break;
            }
        }
    }

    void read_hexcolors(std::ifstream &from, bool read_alpha = false) {
        read_png(from);

        std::ifstream decoded_data(TEMP_IMAGE_RAW_DATA_FILENAME);
        grid_.resize(height_);

        for (unsigned int y = 0; y < height_; ++y) {
            row_filter_type filter_type;

            unsigned char byte = decoded_data.get();
            switch (byte) {
                case 0x00: {
                    filter_type = NONE;
                    break;
                }
                case 0x01: {
                    filter_type = SUB;
                    break;
                }
                case 0x02: {
                    filter_type = UP;
                    break;
                }
                case 0x03: {
                    filter_type = AVERAGE;
                    break;
                }
                case 0x04: {
                    filter_type = PAETH;
                    break;
                }
                default: {
                    throw std::runtime_error("Error");
                }
            }

            pixel temp_pixel;

            for (unsigned int x = 0; x < width_; ++x) {
                temp_pixel.R = decoded_data.get();
                temp_pixel.G = decoded_data.get();
                temp_pixel.B = decoded_data.get();
                if (read_alpha)
                    temp_pixel.A = decoded_data.get();
                grid_[y].push_back(temp_pixel);
                filter_pixel(y, x, filter_type);
            }
        }

        grayscale_grid_ = grid_;
        grayscaled = false;
    }

public:
    explicit Image(std::vector<std::vector<pixel>>
                   &pixel_grid) {
        grid_ = pixel_grid;
        grayscale_grid_ = pixel_grid;
        height_ = pixel_grid.size();
        if (height_ < 5)
            throw std::runtime_error("Min. height is 5 px.");
        else if (height_ > 3000)
            throw std::runtime_error("Max. height is 3000 px.");
        width_ = pixel_grid[0].size();
        if (width_ < 5)
            throw std::runtime_error("Min. width is 5 px.");
        else if (width_ > 3000)
            throw std::runtime_error("Max. width is 3000 px.");
    }

    explicit Image(std::ifstream
                   &file) {
        read_std_header(file);
        setup(file);

        if (height_ < 5)
            throw std::runtime_error("Min. height is 5 px.");
        else if (height_ > 3000)
            throw std::runtime_error("Max. height is 3000 px.");

        if (width_ < 5)
            throw std::runtime_error("Min. width is 5 px.");
        else if (width_ > 3000)
            throw std::runtime_error("Max. width is 3000 px.");

        switch (source_info.color_type) {
            case 2: {
                read_hexcolors(file);
                break;
            }
            case 6: {
                read_hexcolors(file, true);
                break;
            }
        }
    }

    std::vector<std::vector<pixel>> &get(const bool gray = false) {
        if (gray && !grayscaled)
            grayscale();
        return gray ? grayscale_grid_ : grid_;
    }

    void write(const std::string &to, const bool gray = false) {
        std::ofstream fout(to);
        std::vector<unsigned char> current_chunk;

        build_std_header(fout);

        std::vector<unsigned char> ihdr_data;
        std::vector<unsigned char> byte_width = transfer_to_size_t(width_);
        std::vector<unsigned char> byte_height = transfer_to_size_t(height_);

        std::vector<unsigned char> sRGB_data = {'\0'};
        std::vector<unsigned char> gAMA_data = {'\0', '\0', 0xB1, 0x8F};
        std::vector<unsigned char> pHYs_data = {0x0, 0x0, 0x12, 0x74, 0x0, 0x0, 0x12, 0x74, 0x01};
        std::vector<unsigned char> IDAT_data;
        std::vector<unsigned char> IEND_data;

        for (int i = 0; i < 4; ++i)
            ihdr_data.push_back(byte_width[i]);
        for (int i = 0; i < 4; ++i)
            ihdr_data.push_back(byte_height[i]);
        ihdr_data.push_back(8); // Bitdepth
        ihdr_data.push_back(2); // color type
        ihdr_data.push_back(0);
        ihdr_data.push_back(0);
        ihdr_data.push_back(0);

        build_chunk(fout, "IHDR", ihdr_data);
        build_chunk(fout, "sRGB", sRGB_data);
        build_chunk(fout, "gAMA", gAMA_data);
        build_chunk(fout, "pHYs", pHYs_data);

        gray ? encode(grayscale_grid_, 5, 32768) : encode(grid_, 5, 32768);
        std::ifstream image_data(TEMP_OUTPUT_IMAGE_FILENAME);
        while (!image_data.eof()) {
            IDAT_data.push_back(image_data.get());
        }
        IDAT_data.erase(IDAT_data.end() - 1);
        image_data.close();

        build_chunk(fout, "IDAT", IDAT_data);
        build_chunk(fout, "IEND", IEND_data);

        fout.close();
    }

    ~

    Image() {
        grid_.clear();
        grayscale_grid_.clear();
        grayscaled = false;
        width_ = 0;
        height_ = 0;
    }

};
