//
// Created by Михаил on 01.09.2021.
//

#ifndef GAME3X1_LIFE_GAME_H
#define GAME3X1_LIFE_GAME_H

#endif //GAME3X1_LIFE_GAME_H

#define DEATH_INDEX 128
#define ACTIVE_INDEX 200
#define BIRTH_INDEX 155

class Image {
private:
    std::vector<std::vector<pixel>> grid_;
    std::vector<std::vector<pixel>> grayscale_grid_;

    unsigned long width_ = 0;
    unsigned long height_ = 0;

    bool grayscaled = false;

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
        field[y][x].R = 0;
        field[y][x].G = 0;
        field[y][x].B = 0;
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
                if (grayscale_grid_[y][x].R == 0) {
                    grid_[y][x].R = 0;
                    grid_[y][x].G = 0;
                    grid_[y][x].B = 0;
                }
            }
        }
        grayscaled = false;
    }

public:
    explicit Image(std::vector<std::vector<pixel>> &pixel_grid) {
        grid_ = pixel_grid;
        grayscale_grid_ = pixel_grid;
        height_ = pixel_grid.size();
        if (height_ < 5)
            throw std::runtime_error("Min. height is 5 px.");
        width_ = pixel_grid[0].size();
        if (width_ < 5)
            throw std::runtime_error("Min. width is 5 px.");
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
                    if (index < DEATH_INDEX) {
                        death(grayscaled_grid_tmp, y, x);
                    }
                    else if (index < DEATH_INDEX && count_neighbours(y, x, true) == 3) {
                        alive(grayscaled_grid_tmp, y, x);
                    }
                    else if (index >= DEATH_INDEX && (count_neighbours(y, x, false) < 2 ||
                                                 count_neighbours(y, x, false) > 3)) {
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
                        grid_[y][x].R = grid_[y][x].R * 3 + 1;
                    else
                        grid_[y][x].R /= 2;

                    if (grid_[y][x].G % 2)
                        grid_[y][x].G = grid_[y][x].G * 3 + 1;
                    else
                        grid_[y][x].G /= 2;

                    if (grid_[y][x].B % 2)
                        grid_[y][x].B = grid_[y][x].B * 3 + 1;
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

    ~Image() {
        grid_.clear();
        grayscale_grid_.clear();
        grayscaled = false;
        width_ = 0;
        height_ = 0;
    }
};
