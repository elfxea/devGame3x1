//
// Created by Михаил on 04.09.2021.
//

#ifndef GAME3X1_CHUNK_CLASS_H
#define GAME3X1_CHUNK_CLASS_H

#endif //GAME3X1_CHUNK_CLASS_H

#include "crc.h"

class Chunk {
private:
    unsigned int size_ = 0;
    std::string name_;
    std::vector<unsigned char> data_ = {};
    unsigned int checksum_ = 0;
    
    std::vector<unsigned char> chunk_;
    
    static std::vector<unsigned char> transfer_to_size_t(const unsigned int needle) {

        std::vector<unsigned char> result;
        result.push_back((needle & 0xFF000000) >> 24);
        result.push_back((needle & 0x00FF0000) >> 16);
        result.push_back((needle & 0x0000FF00) >> 8);
        result.push_back((needle & 0x000000FF));

        return result;
    }
public:
    Chunk(std::string name) {
        name_ = name;
    }
    
    void push(std::vector<unsigned char> data) {
        data_ = data;
        size_ = data.size();
    }
    
    std::vector<unsigned char> get() {
        std::vector<unsigned char> encrypted_chunk;
        chunk_.clear();
        for (unsigned char size_byte : transfer_to_size_t(size_))
            chunk_.push_back(size_byte);
        for (unsigned char name_byte : name_) {
            chunk_.push_back(name_byte);
            encrypted_chunk.push_back(name_byte);
        }
        for (unsigned char data_byte : data_) {
            chunk_.push_back(data_byte);
            encrypted_chunk.push_back(data_byte);
        }
        checksum_ = crc(encrypted_chunk);
        for (unsigned char checksum_byte : transfer_to_size_t(checksum_))
            chunk_.push_back(checksum_byte);
        
        return chunk_;
    }
};