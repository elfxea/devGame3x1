//
// Created by Михаил on 27.08.2021.
//

#ifndef GAME3X1_DATA_STRUCTURES_H
#define GAME3X1_DATA_STRUCTURES_H

#endif //GAME3X1_DATA_STRUCTURES_H

struct signature {
    unsigned char is_text_file = 0x89;
    unsigned char PNG[3] = {0x50, 0x4E, 0x47};
    unsigned char CRLF[2] = {0x0D, 0x0A};
    unsigned char DOS_EOF = 0x1A;
    unsigned char LF = 0x0A;

    bool operator==(signature &compare) {
        return (
                is_text_file == compare.is_text_file
                && PNG[0] == compare.PNG[0]
                && PNG[1] == compare.PNG[1]
                && PNG[2] == compare.PNG[2]
                && CRLF[0] == compare.CRLF[0]
                && CRLF[1] == compare.CRLF[1]
                && DOS_EOF == compare.DOS_EOF
                && LF == compare.LF
        );
    }

    bool operator!=(signature &compare) {
        return !(*this == compare);
    }

    explicit signature(const unsigned char byte_array[8]) {
        is_text_file = byte_array[0];
        PNG[0] = byte_array[1];
        PNG[1] = byte_array[2];
        PNG[2] = byte_array[3];
        CRLF[0] = byte_array[4];
        CRLF[1] = byte_array[5];
        DOS_EOF = byte_array[6];
        LF = byte_array[7];
    }

    signature() {}
};

struct IHDR {
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned char depth = 8;
    unsigned char color_type = 0;
    unsigned char compression_type = 0;
    unsigned char filter_type = 0;
    unsigned char interlace = 0;
};

struct pixel {
    unsigned int R = 0;
    unsigned int G = 0;
    unsigned int B = 0;
    unsigned int A = 0;

    pixel() {}

    pixel(unsigned int r, unsigned int g, unsigned int b, unsigned int a = 0) {
        R = r;
        G = g;
        B = b;
        A = a;
    }
};