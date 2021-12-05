//
// Created by Михаил on 01.09.2021.
//
#include "crc.h" // CRC32 calculator
#include "chunk_class.h" // Chunk class

#ifndef GAME3X1_IMAGE_CLASS_H
#define GAME3X1_IMAGE_CLASS_H

#endif //GAME3X1_IMAGE_CLASS_H

class Image {
private:
    std::vector<std::vector<pixel>> pixel_grid_;

    unsigned long width_ = 0;
    unsigned long height_ = 0;
    unsigned char bitdepth_ = 8;

    IHDR source_info;

    /**
     * Reads first 8 bytes of provided file and compares it to the std PNG signature.
     * @param file an std::ifstream instance.
     * @throws runtimeError if provided file signature is not a standard PNG signature.
     */
    static void read_std_header(std::ifstream &file) {
        file.seekg(0, std::ios_base::beg);
        signature file_header, std_signature;
        unsigned char bytestream[8];
        for (unsigned char i = 0; i < 8 && !file.eof(); ++i)
            bytestream[i] = file.get();
        if (file.eof())
            throw std::runtime_error("Provided file is not a PNG.");

        file_header = bytestream;

        if (file_header != std_signature)
            throw std::runtime_error("Provided file is not a PNG.");
    }

    static bool check_frame(std::vector<unsigned char> &chunk_data, unsigned int checksum) noexcept {
        unsigned int calculated_checksum = crc(chunk_data);
        if (checksum != calculated_checksum)
            return false;
        return true;
    }

    /**
     * Reads PNG chunks (excluding IHDR chunk).
     * @param file An std::ifstream instance. Must point to first letter of chunk name.
     * @throws runtimeError If chunk data is damaged (data CRC32 != provided at the end of chunk CRC32).
     * @return Chunk name + data vector.
     */
    static std::vector<unsigned char> read_chunk(std::ifstream &file) {
        unsigned int chunk_size = 0;
        std::string chunk_name;
        std::vector<unsigned char> data;
        std::vector<unsigned char> full_chunk;
        unsigned int checksum = 0;

        for (char i = 0; i < 4; ++i) {
            chunk_size = chunk_size << 8;
            chunk_size += file.get();
        }

        for (char i = 0; i < 4; ++i) {
            chunk_name.push_back((char) file.get());
            full_chunk.push_back(chunk_name[i]);
        }

        for (unsigned int i = 0; i < chunk_size; ++i) {
            data.push_back(file.get());
            full_chunk.push_back(data[i]);
        }

        for (char i = 0; i < 4; ++i) {
            checksum = checksum << 8;
            checksum += file.get();
        }

        if (!check_frame(full_chunk, checksum))
            throw std::runtime_error("Unable to read frame. It may be damaged.");
        return full_chunk;
    }

    /**
     * Reads source image data (IHDR chunk)
     * @param file An std::ifstream instance. Must point to first letter of chunk name.
     * @throws runtimeError If IHDR chunk is not first.
     */
    void setup(std::ifstream &file) {
        std::vector<unsigned char> data = read_chunk(file);
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

        source_info.bitdepth = data[12];
        source_info.color_type = data[13];
        source_info.compression_type = data[14];
        source_info.filter_type = data[15];
        source_info.interlace = data[16];

        if (pow(2, source_info.bitdepth) != BITDEPTH) {
            throw std::runtime_error("Unable to read images with bit bitdepth != 8 bit.");
        }
    }

    static std::vector<unsigned char> transfer_to_size_t(const unsigned int needle) noexcept {
        std::vector<unsigned char> result;
        result.push_back((needle & 0xFF000000) >> 24);
        result.push_back((needle & 0x00FF0000) >> 16);
        result.push_back((needle & 0x0000FF00) >> 8);
        result.push_back((needle & 0x000000FF));
        return result;
    }

    static void build_std_header(std::ofstream &file) {
        file.put(0x89); // non-text file
        file.put('P');
        file.put('N');
        file.put('G');
        file.put(0x0D); // Line breakers
        file.put(0x0A);
        file.put(0x1A);
        file.put(0x0A);
    }

    static void
    build_chunk(std::ofstream &file, const std::string &chunk_name, const std::vector<unsigned char> &data) noexcept {
        Chunk tmp(chunk_name);
        tmp.push(data);
        for (unsigned char data_byte : tmp.get())
            file.put((char) data_byte);
    }

    static void read_png(std::ifstream &file) {
        std::ofstream tmp(TEMP_IMAGE_DATA_FILENAME);
        while (!file.eof()) {
            std::vector<unsigned char> data = read_chunk(file);
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
        decode();
    }

    /**
     * Paeth predictor function for filter_type = 4.
     * @param y Current pixel row.
     * @param x Current pixel column.
     * @return Pixel filtered by Paeth predictor
     */
    pixel paeth_predictor(int y, int x) {
        pixel left = pixel_grid_[y][x - 1];
        pixel above = pixel_grid_[y - 1][x];
        pixel above_left = pixel_grid_[y - 1][x - 1];

        unsigned char paeth_red = 0;
        unsigned char paeth_green = 0;
        unsigned char paeth_blue = 0;
        unsigned char paeth_alpha = 0;

        paeth_red += left.R;
        paeth_red += above.R;
        paeth_red -= above_left.R;

        paeth_green += left.G;
        paeth_green += above.G;
        paeth_green -= above_left.G;

        paeth_blue += left.B;
        paeth_blue += above.B;
        paeth_blue -= above_left.B;

        paeth_alpha += left.A;
        paeth_alpha += above.A;
        paeth_alpha -= above_left.A;

        unsigned char paeth_left_red = paeth_red > left.R ? paeth_red - left.R : left.R - paeth_red;
        unsigned char paeth_left_green = paeth_green > left.G ? paeth_green - left.G : left.G - paeth_green;
        unsigned char paeth_left_blue = paeth_blue > left.B ? paeth_blue - left.B : left.B - paeth_blue;
        unsigned char paeth_left_alpha = paeth_alpha > left.A ? paeth_alpha - left.A : left.A - paeth_alpha;

        unsigned char paeth_above_red = paeth_red > above.R ? paeth_red - above.R : above.R - paeth_red;
        unsigned char paeth_above_green = paeth_green > above.G ? paeth_green - above.G : above.G - paeth_green;
        unsigned char paeth_above_blue = paeth_blue > above.B ? paeth_blue - above.B : above.B - paeth_blue;
        unsigned char paeth_above_alpha = paeth_alpha > above.A ? paeth_alpha - above.A : above.A - paeth_alpha;

        unsigned char paeth_above_left_red =
                paeth_red > above_left.R ? paeth_red - above_left.R : above_left.R - paeth_red;
        unsigned char paeth_above_left_green =
                paeth_green > above_left.G ? paeth_green - above_left.G : above_left.G - paeth_green;
        unsigned char paeth_above_left_blue =
                paeth_blue > above_left.B ? paeth_blue - above_left.B : above_left.B - paeth_blue;
        unsigned char paeth_above_left_alpha =
                paeth_alpha > above_left.A ? paeth_alpha - above_left.A : above_left.A - paeth_alpha;

        unsigned char red;
        unsigned char green;
        unsigned char blue;
        unsigned char alpha;

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

        if ((paeth_left_alpha <= paeth_above_alpha) && paeth_left_alpha <= paeth_above_left_alpha)
            alpha = left.A;
        else if (paeth_above_alpha <= paeth_above_left_alpha)
            alpha = above.A;
        else
            alpha = above_left.A;

        pixel tmp(red, green, blue, alpha);
        return tmp;
    }

    /**
     * A function to choose filter function for pixel at (x; y).
     * @param y Pixel row.
     * @param x Pixel column.
     * @param filter_type Row filter type.
     */
    void filter_pixel(unsigned int y, unsigned int x, row_filter_type filter_type) {
        unsigned int max_color_index = pow(2, source_info.bitdepth);
        switch (filter_type) {
            case NONE: {
                break;
            }
            case SUB: {
                if (x == 0)
                    break;
                pixel_grid_[y][x].R += pixel_grid_[y][x - 1].R;
                pixel_grid_[y][x].G += pixel_grid_[y][x - 1].G;
                pixel_grid_[y][x].B += pixel_grid_[y][x - 1].B;
                pixel_grid_[y][x].A += pixel_grid_[y][x - 1].A;
                break;
            }
            case UP: {
                if (y == 0)
                    break;
                pixel_grid_[y][x].R += pixel_grid_[y - 1][x].R;
                pixel_grid_[y][x].G += pixel_grid_[y - 1][x].G;
                pixel_grid_[y][x].B += pixel_grid_[y - 1][x].B;
                pixel_grid_[y][x].A += pixel_grid_[y - 1][x].A;
                break;
            }
            case AVERAGE: {
                if (y == 0) {
                    if (x == 0)
                        break;
                    pixel_grid_[y][x].R += pixel_grid_[y][x - 1].R / 2;
                    pixel_grid_[y][x].G += pixel_grid_[y][x - 1].G / 2;
                    pixel_grid_[y][x].B += pixel_grid_[y][x - 1].B / 2;
                    pixel_grid_[y][x].A += pixel_grid_[y][x - 1].A / 2;
                } else if (x == 0) {
                    pixel_grid_[y][x].R += pixel_grid_[y - 1][x].R / 2;
                    pixel_grid_[y][x].G += pixel_grid_[y - 1][x].G / 2;
                    pixel_grid_[y][x].B += pixel_grid_[y - 1][x].B / 2;
                    pixel_grid_[y][x].A += pixel_grid_[y - 1][x].A / 2;
                } else {
                    pixel_grid_[y][x].R += (pixel_grid_[y - 1][x].R + pixel_grid_[y][x - 1].R) / 2;
                    pixel_grid_[y][x].G += (pixel_grid_[y - 1][x].G + pixel_grid_[y][x - 1].G) / 2;
                    pixel_grid_[y][x].B += (pixel_grid_[y - 1][x].B + pixel_grid_[y][x - 1].B) / 2;
                    pixel_grid_[y][x].A += (pixel_grid_[y - 1][x].A + pixel_grid_[y][x - 1].A) / 2;
                }
                break;
            }
            case PAETH: {
                pixel prediction = paeth_predictor(y, x);
                if (x != 0) {
                    pixel_grid_[y][x].R += prediction.R;
                    pixel_grid_[y][x].G += prediction.G;
                    pixel_grid_[y][x].B += prediction.B;
                    pixel_grid_[y][x].A += prediction.A;
                } else {
                    pixel_grid_[y][x].R += pixel_grid_[y - 1][x].R;
                    pixel_grid_[y][x].G += pixel_grid_[y - 1][x].G;
                    pixel_grid_[y][x].B += pixel_grid_[y - 1][x].B;
                    pixel_grid_[y][x].A += pixel_grid_[y - 1][x].A;
                }
                break;
            }
        }

        pixel_grid_[y][x].R %= max_color_index;
        pixel_grid_[y][x].G %= max_color_index;
        pixel_grid_[y][x].B %= max_color_index;
        pixel_grid_[y][x].A %= max_color_index;
    }

    void read_hexcolors(std::ifstream &from, bool read_alpha = false) {
        read_png(from);

        std::ifstream decoded_data(TEMP_IMAGE_RAW_DATA_FILENAME);
        pixel_grid_.resize(height_);

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
                pixel_grid_[y].push_back(temp_pixel);
                filter_pixel(y, x, filter_type);
            }
        }
    }

public:
    explicit Image(std::vector<std::vector<pixel>>
                   &pixel_grid) {
        pixel_grid_ = pixel_grid;
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

    std::vector<std::vector<pixel>> &get() {
        return pixel_grid_;
    }

    void write(const std::string &to) {
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

        encode(pixel_grid_, 5, 32768);
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

    void stretch_range() {
        double max_light = 0;
        double min_light = 255;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                if ((pixel_grid_[y][x].R + pixel_grid_[y][x].G + pixel_grid_[y][x].B) / 3 < min_light)
                    min_light = (pixel_grid_[y][x].R + pixel_grid_[y][x].G + pixel_grid_[y][x].B) / 3;
                if ((pixel_grid_[y][x].R + pixel_grid_[y][x].G + pixel_grid_[y][x].B) / 3 > max_light)
                    max_light = (pixel_grid_[y][x].R + pixel_grid_[y][x].G + pixel_grid_[y][x].B) / 3;
            }
        }
        double alpha = 255 * (min_light / (min_light - max_light));
        double beta = 255 / (max_light - min_light);

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                if (alpha + beta * pixel_grid_[y][x].R > 0)
                    pixel_grid_[y][x].R =
                            alpha + beta * pixel_grid_[y][x].R < 256 ? alpha + beta * pixel_grid_[y][x].R : 255;
                else
                    pixel_grid_[y][x].R = 0;

                if (alpha + beta * pixel_grid_[y][x].G > 0)
                    pixel_grid_[y][x].G =
                            alpha + beta * pixel_grid_[y][x].G < 256 ? alpha + beta * pixel_grid_[y][x].G : 255;
                else
                    pixel_grid_[y][x].G = 0;

                if (alpha + beta * pixel_grid_[y][x].B > 0)
                    pixel_grid_[y][x].B =
                            alpha + beta * pixel_grid_[y][x].B < 256 ? alpha + beta * pixel_grid_[y][x].B : 255;
                else
                    pixel_grid_[y][x].B = 0;
            }
        }
        return;
    }

    void denoise(unsigned int threshold = 5) {
        unsigned int median_r = 0;
        unsigned int median_g = 0;
        unsigned int median_b = 0;

        std::vector<unsigned int> coord;

        unsigned int counter;

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                counter = 1;
                median_r = pixel_grid_[y][x].R;
                median_g = pixel_grid_[y][x].G;
                median_b = pixel_grid_[y][x].B;
                if (x < width_ - 1
                    && pixel_grid_[y][x + 1].R - pixel_grid_[y][x].R < threshold &&
                    pixel_grid_[y][x + 1].R - pixel_grid_[y][x].R > 0 - threshold
                    && pixel_grid_[y][x + 1].G - pixel_grid_[y][x].G < threshold &&
                    pixel_grid_[y][x + 1].G - pixel_grid_[y][x].G > 0 - threshold
                    && pixel_grid_[y][x + 1].B - pixel_grid_[y][x].B < threshold &&
                    pixel_grid_[y][x + 1].B - pixel_grid_[y][x].B > 0 - threshold) {
                    //median_r +=
                }
            }
        }
    }

    ~

    Image() {
        pixel_grid_.clear();
        width_ = 0;
        height_ = 0;
    }

};
