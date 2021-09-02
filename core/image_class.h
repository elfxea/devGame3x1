//
// Created by Михаил on 01.09.2021.
//
#include "crc.h"

#ifndef GAME3X1_LIFE_GAME_H
#define GAME3X1_LIFE_GAME_H

#endif //GAME3X1_LIFE_GAME_H

class Image {
private:
    std::vector<std::vector<pixel>> grid_;
    std::vector<std::vector<pixel>> grayscale_grid_;

    unsigned long width_ = 0;
    unsigned long height_ = 0;

    IHDR source_info;

    bool grayscaled = false;

    void read_std_header(std::ifstream &from) {
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

    bool check_frame(std::vector<unsigned char> data, unsigned int checksum) {
        unsigned int calculated_checksum = crc(data, data.size());
        if (checksum != calculated_checksum)
            return false;
        return true;
    }

    std::vector<unsigned char> read_chunk(std::ifstream &from) {
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

    void death(std::vector<std::vector<pixel>> &field, const unsigned int y, const unsigned int x) {
        field[y][x].R = DEATH_INDEX;
        field[y][x].G = DEATH_INDEX;
        field[y][x].B = DEATH_INDEX;
    }

    void alive(std::vector<std::vector<pixel>> &field, const unsigned int y, const unsigned int x) {
        field[y][x].R = BIRTH_INDEX;
        field[y][x].G = BIRTH_INDEX;
        field[y][x].B = BIRTH_INDEX;
    }

    unsigned char
    count_neighbours(const unsigned int y, const unsigned int x, const bool count_active_neighbours = false) const {
        unsigned char result = 0;

        const unsigned int index_y_top = (y > 0) ? y - 1 : height_ - 1;
        const unsigned int index_x_top = x;

        const unsigned int index_y_right = y;
        const unsigned int index_x_right = (x < width_ - 1) ? x + 1 : 0;

        const unsigned int index_y_bottom = (y < height_ - 1) ? y + 1 : 0;
        const unsigned int index_x_bottom = x;

        const unsigned int index_y_left = y;
        const unsigned int index_x_left = (x > 0) ? x - 1 : width_ - 1;

        const unsigned int comparison = count_active_neighbours ? ACTIVE_INDEX : DEATH_INDEX;

        if (grayscale_grid_[index_y_top][index_x_top].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_top][index_x_right].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_right][index_x_right].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_bottom][index_x_right].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_bottom][index_x_bottom].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_bottom][index_x_left].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_left][index_x_left].R >= comparison)
            result += 1;
        if (grayscale_grid_[index_y_top][index_x_left].R >= comparison)
            result += 1;

        return result;
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

    std::vector<unsigned char> transfer_to_size_t(const unsigned int needle) {

        std::vector<unsigned char> result;
        result.push_back((needle & 0xFF000000) >> 24);
        result.push_back((needle & 0x00FF0000) >> 16);
        result.push_back((needle & 0x0000FF00) >> 8);
        result.push_back((needle & 0x000000FF));

        return result;
    }

    void build_std_header(std::ofstream &at) {
        at.put(0x89); // non-text file
        at.put('P');
        at.put('N');
        at.put('G');
        at.put(0x0D); // Line breakers
        at.put(0x0A);
        at.put(0x1A);
        at.put(0x0A);
    }

    void build_chunk(std::ofstream &at, const std::string &chunk_name, const std::vector<unsigned char> &data) {
        unsigned int d = data.size();
        std::vector<unsigned char> size = transfer_to_size_t(data.size());
        for (unsigned int i = 0; i < 4; ++i)
            at.put(size[i]);
        std::vector<unsigned char> chunk;
        for (char i : chunk_name)
            chunk.push_back(i);
        for (unsigned char i : data)
            chunk.push_back(i);
        for (unsigned char i : chunk)
            at.put(i);

        unsigned int checksum = crc(chunk, chunk.size());
        at.put((checksum & 0xFF000000) >> 24);
        at.put((checksum & 0x00FF0000) >> 16);
        at.put((checksum & 0x0000FF00) >> 8);
        at.put((checksum & 0x000000FF));
    }

    void read_png(std::ifstream &from) {
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
            }
            else if (
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

    void read_RGBA(std::ifstream &from) {
        read_png(from);

        std::ifstream decoded_data(TEMP_IMAGE_RAW_DATA);
        grid_.resize(height_);

        for (unsigned int y = 0; y < height_; ++y) {
            unsigned char filter_type;
            filter_type = decoded_data.get();

            switch (filter_type) {
                case 0x0: {
                    for (unsigned int x = 0; x < width_; ++x) {
                        unsigned char r = decoded_data.get();
                        unsigned char g = decoded_data.get();
                        unsigned char b = decoded_data.get();
                        unsigned char a = decoded_data.get();
                        pixel temp_pixel(r, g, b, a);
                        grid_[y].push_back(temp_pixel);
                    }
                    break;
                }
                case 0x01: {
                    unsigned char r = decoded_data.get();
                    unsigned char g = decoded_data.get();
                    unsigned char b = decoded_data.get();
                    unsigned char a = decoded_data.get();
                    pixel temp_pixel(r, g, b, a);
                    grid_[y].push_back(temp_pixel);
                    for (unsigned int x = 1; x < width_; ++x) {
                        r = decoded_data.get() + grid_[y][x - 1].R;
                        g = decoded_data.get() + grid_[y][x - 1].G;
                        b = decoded_data.get() + grid_[y][x - 1].B;
                        a = decoded_data.get() + grid_[y][x - 1].A;
                        temp_pixel = *(new pixel(r, g, b, a));
                        grid_[y].push_back(temp_pixel);
                    }
                    break;
                }
                case 0x02: {
                    unsigned char r = decoded_data.get();
                    unsigned char g = decoded_data.get();
                    unsigned char b = decoded_data.get();
                    unsigned char a = decoded_data.get();
                    pixel temp_pixel(r, g, b, a);
                    grid_[y].push_back(temp_pixel);
                    for (unsigned int x = 1; x < width_; ++x) {
                        unsigned char r = decoded_data.get() + grid_[y - 1][x].R;
                        unsigned char g = decoded_data.get() + grid_[y - 1][x].G;
                        unsigned char b = decoded_data.get() + grid_[y - 1][x].B;
                        unsigned char a = decoded_data.get() + grid_[y - 1][x].A;
                        pixel temp_pixel(r, g, b, a);
                        grid_[y].push_back(temp_pixel);
                    }
                    break;
                }
                case 0x03: {
                    unsigned int s = decoded_data.tellg();
                    unsigned char r = decoded_data.get() + (grid_[y - 1][0].R / 2);
                    unsigned char g = decoded_data.get() + (grid_[y - 1][0].G / 2);
                    unsigned char b = decoded_data.get() + (grid_[y - 1][0].B / 2);
                    unsigned char a = decoded_data.get() + (grid_[y - 1][0].A / 2);
                    pixel temp_pixel(r, g, b, a);
                    grid_[y].push_back(temp_pixel);
                    for (unsigned int x = 1; x < width_; ++x) {
                        r = decoded_data.get() + ((grid_[y - 1][x].R + grid_[y][x - 1].R) / 2);
                        g = decoded_data.get() + ((grid_[y - 1][x].G + grid_[y][x - 1].G) / 2);
                        b = decoded_data.get() + ((grid_[y - 1][x].B + grid_[y][x - 1].B) / 2);
                        a = decoded_data.get() + ((grid_[y - 1][x].A + grid_[y][x - 1].A) / 2);
                        pixel temp_pixel(r, g, b, a);
                        grid_[y].push_back(temp_pixel);
                    }
                    break;
                }
                default: {
                    int s = decoded_data.tellg();
                    throw std::runtime_error("Error");
                    break;
                }
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
        width_ = pixel_grid[0].size();
        if (width_ < 5)
            throw std::runtime_error("Min. width is 5 px.");
    }

    explicit Image(std::ifstream
                   &file) {
        read_std_header(file);
        setup(file);

        switch (source_info.color_type) {
            case 2: {
                break;
            }
            case 6: {
                read_RGBA(file);
                break;
            }
        }
    }

    void play(const unsigned int generations = 1) {
        std::vector<std::vector<pixel>> grayscaled_grid_tmp;

        for (unsigned int i = 0; i < generations; ++i) {
            if (!grayscaled)
                grayscale();
            grayscaled_grid_tmp = grayscale_grid_;
            for (unsigned int y = 0; y < height_; ++y) {
                for (unsigned int x = 0; x < width_; ++x) {
                    unsigned int index = grayscale_grid_[y][x].R;
                    unsigned int alive_neighbours = count_neighbours(y, x, false);
                    unsigned int active_neighbours = count_neighbours(y, x, true);
                    if (index < DEATH_INDEX) {
                        death(grayscaled_grid_tmp, y, x);
                    } else if (index < DEATH_INDEX && active_neighbours > 3) {
                        alive(grayscaled_grid_tmp, y, x);
                    } else if (index >= DEATH_INDEX && (alive_neighbours < 2 ||
                                                        alive_neighbours > 4)) {
                        death(grayscaled_grid_tmp, y, x);
                    }
                }
            }
            grayscale_grid_ = grayscaled_grid_tmp;
        }
        append_grayscaled();
    }

    void mesh(const unsigned int generations = 1) {
        for (int i = 0; i < generations; ++i) {
            for (unsigned int y = 0; y < height_; ++y) {
                for (unsigned int x = 0; x < width_; ++x) {
                    if (grid_[y][x].R % 2)
                        grid_[y][x].R = (grid_[y][x].R * 3 + 1) % BITDEPTH;
                    else
                        grid_[y][x].R /= 2;

                    if (grid_[y][x].G % 2)
                        grid_[y][x].G = (grid_[y][x].G * 3 + 1) % BITDEPTH;
                    else
                        grid_[y][x].G /= 2;

                    if (grid_[y][x].B % 2)
                        grid_[y][x].B = (grid_[y][x].B * 3 + 1) % BITDEPTH;
                    else
                        grid_[y][x].B /= 2;
                }
            }
        }
        grayscaled = false;
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

        std::vector<unsigned char> sRGB_data = {'\0'};
        build_chunk(fout, "sRGB", sRGB_data);

        std::vector<unsigned char> gAMA_data = {'\0', '\0', 0xB1, 0x8F};
        build_chunk(fout, "gAMA", gAMA_data);

        std::vector<unsigned char> pHYs_data = {0x0, 0x0, 0x12, 0x74, 0x0, 0x0, 0x12, 0x74, 0x01};
        build_chunk(fout, "pHYs", pHYs_data);

        gray ? encode(grayscale_grid_, 0, 32768) : encode(grid_, 0, 32768);
        std::vector<unsigned char> IDAT_data;
        std::ifstream image_data(TEMP_GRAYSCALE_IMAGE);
        unsigned char byte;
        while (!image_data.eof()) {
            IDAT_data.push_back(image_data.get());
        }
        IDAT_data.erase(IDAT_data.end() - 1);
        image_data.close();

        build_chunk(fout, "IDAT", IDAT_data);

        std::vector<unsigned char> IEND_data;
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
