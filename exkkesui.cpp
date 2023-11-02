// exkkesui.cpp, v1.0 2009/11/18
// coded by asmodean

// contact: 
// web: http://asmodean.reverse.net
// email: asmodean [at] hush.com
// irc: asmodean on efnet (irc.efnet.net)

// This tool extracts data from かのこん えすいー.

// exkkesui remake  2023/11/02
// https://github.com/Manicsteiner/exkkesui
// Ported to a modern C++ version.
// Only tested on Windows 10 x64 MSVC, considering the difference of interpretation of c++ long type by 64-bit system, it may not work on linux and mac, or 32-bit Windows.


//#include "as-util.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

struct RGBA {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

struct DATENTRY {
    unsigned int offset;
    unsigned int length;
};

static const unsigned int DATENTRY_COUNT = 4096;

struct DATHDR {
    DATENTRY entries[DATENTRY_COUNT];
};

struct CPSHDR {
    char signature[4]; // CPS
    unsigned int length;
    unsigned int unknown1;
    unsigned int original_length;
    unsigned int unknown2;
    unsigned short width;
    unsigned short height;
    unsigned int depth;
    unsigned int unknown3;

    unsigned int unknown4;
    unsigned int unknown5;
};

//as::
string get_file_prefix(const string& filename, bool include_path)
{
    if (include_path)
        return filename.substr(0, filename.rfind("."));
    string::size_type iPos = filename.find_last_of('\\') + 1;
    if (filename.rfind(".") > iPos)
        return filename.substr(iPos, filename.rfind("."));
    else
        return filename.substr(iPos, filename.length() - iPos);
}

/// <summary>
/// seems working, wrong rotation, wrong RGB status
/// </summary>
void write_bmp(const string& filename, char* buffer, unsigned int len, unsigned short width, unsigned short height)
{
    ofstream pic(filename, ios::out | ios::binary);
    unsigned short header1 = 0x4D42;
    unsigned int write_len = len + 54, write_width = width, write_height = height;
    unsigned int headerzero = 0, header36 = 0x36, header28 = 0x28, header20 = 0x200001;
    pic.write((char*)&header1, sizeof(unsigned short));
    pic.write((char*)&write_len, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&header36, sizeof(unsigned int));
    pic.write((char*)&header28, sizeof(unsigned int));
    pic.write((char*)&write_width, sizeof(unsigned int));
    pic.write((char*)&write_height, sizeof(unsigned int));
    pic.write((char*)&header20, sizeof(unsigned int));
    /*6x00*/
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    pic.write((char*)&headerzero, sizeof(unsigned int));
    /*end of header*/
    //pic.write(buffer, len);
    for (unsigned int y = width * (height - 1) * 4; y >= 0; y -= (width * 4))
    {
        for (unsigned int x = 0; x < width * 4; x += 4)
        {
            //rgba to bgra
            pic.write(buffer + y + x + 2, sizeof(unsigned char));
            pic.write(buffer + y + x + 1, sizeof(unsigned char));
            pic.write(buffer + y + x, sizeof(unsigned char));
            pic.write(buffer + y + x + 3, sizeof(unsigned char));
        }
        //pic.write(buffer + i, sizeof(unsigned char) * 4);
        if (y == 0 || y >= len) break;
    }
    pic.close();
    return;
}

/// <summary>
/// bin file of RNEIC https://github.com/Manicsteiner/RNE_image_converter
/// </summary>
void write_bin(const string& filename, char* buffer, unsigned int len, unsigned short width, unsigned short height)
{
    //write ushort w,h,32,00
    //then write all buffer
    ofstream pic(filename, ios::out | ios::binary);
    unsigned short header3 = 32;
    unsigned short header4 = 0;
    pic.write((char*)&width, sizeof(unsigned short));
    pic.write((char*)&height, sizeof(unsigned short));
    pic.write((char*)&header3, sizeof(unsigned short));
    pic.write((char*)&header4, sizeof(unsigned short));
    //rgba to argb
    //pic.write(buffer, len);
    for (unsigned int i = 0; i < len; i += 4)
    {
        unsigned char* temp = (unsigned char*)(buffer + i + 3);
        pic.write((char*)temp, sizeof(unsigned char));
        pic.write(buffer + i, sizeof(char) * 3);
    }
    pic.close();
    return;
}

void write_file(const string& filename, char* buffer, unsigned int len)
{
    ofstream filewrite(filename, ios::out | ios::binary);
    filewrite.write(buffer, len);
    filewrite.close();
    return;
}

template <typename... Args> 
string stringf(const char* pformat, Args... args)
{
    // 计算字符串长度
    int len_str = snprintf(nullptr, 0, pformat, args...);

    if (0 >= len_str)
        return string("");

    len_str++;
    //char *pstr_out = nullptr;
    //pstr_out = new(nothrow) char[len_str];
    char* pstr_out = (char*)malloc(sizeof(char) * len_str);
    
    // 申请失败
    if (NULL == pstr_out || nullptr == pstr_out)
        return string("");

    // 构造字符串
    snprintf(pstr_out, len_str, pformat, args...);

    // 保存构造结果
    string str(pstr_out);

    // 释放空间
    //delete [] pstr_out;
    //pstr_out = nullptr;
    free(pstr_out);

    return str;
}
//as end

// Don't ask
unsigned int get_pal_index(unsigned int n)
{
    if ((n & 31) >= 8) {
        if ((n & 31) < 16) {
            n += 8; // +8 - 15 to +16 - 23
        } else if ((n & 31) < 24) {
            n -= 8; // +16 - 23 to +8 - 15
        }
    }

    return n;
}

void process_cps(const string& filename, char* buff, unsigned int len)
{
    static const unsigned int KEYS[] = { 0x2623A189, 0x146FD8D7, 0x8E6F55FF, 0x1F497BCD, 
                                         0x1BB74F41, 0x0EB731D1, 0x5C031379, 0x64350881 };
    static const unsigned int KEYS_COUNT = sizeof(KEYS) / sizeof(KEYS[0]);

    CPSHDR* hdr = (CPSHDR*) buff;

    unsigned int seed_offset = *(unsigned int*) (buff + hdr->length - 4);
    seed_offset -= 0x7534682;

    unsigned int* words = (unsigned int*) buff;
    unsigned int seed_index = seed_offset / 4;
    unsigned int seed = words[seed_index] + seed_offset + 0x3786425;
    unsigned int n = min((unsigned int)1023, len / 4);

    for (unsigned int i = 8, j = 0; i < n; i++, j++) {
        if (i != seed_index) {
            words[i] -= KEYS[j % KEYS_COUNT];
            words[i] -= seed;
            words[i] -= hdr->length;
        }

        seed *= 0x41C64E6D;
        seed += 0x9B06;
    }

    unsigned int out_len = hdr->original_length;
    //char* out_buff = new char[out_len];
    char* out_buff = (char*)malloc(sizeof(char) * out_len);
    memset(out_buff, 0, out_len);

    unsigned char* in = (unsigned char*)buff + sizeof(*hdr);
    char* end = buff + hdr->length;

    char* out = out_buff;
    char* out_end = out_buff + out_len - 1;
    //unsigned long long out_end = (unsigned long long)out_buff + (unsigned long long)out_len - (unsigned long long)1;

    while (out <= out_end) {
        unsigned int c = *in++;

        if (c & 0x80) {
            if (c & 0x40) {
                unsigned int n = (c & 0x1F) + 2;

                if (c & 0x20) {
                    n += *in++ << 5;
                }

                while (n--) {
                    *out++ = *in;
                }

                in++;
            } else {
                unsigned int p = ((c & 3) << 8) | *in++;
                unsigned int n = ((c >> 2) & 0xF) + 2;

                while (n--) {
                    *out = *(out - p - 1);
                    out++;
                }
            }
        } else {
            if (c & 0x40) {
                unsigned int r = *in++ + 1;
                unsigned int n = (c & 0x3F) + 2;

                while (r--) {
                    memcpy(out, in, n);
                    out += n;
                }

                in += n;
            } else {
                unsigned int n = (c & 0x1F) + 1;

                if (c & 0x20) {
                    n += *in++ << 5;
                }

                while (n--) {
                    *out++ = *in++;
                }
            }
        }
    }

    /*if (hdr->depth == 8) {
        RGBA* pal = (RGBA*) out_buff;

        RGBA fixed_pal[256];
        for (unsigned int i = 0; i < 256; i++) {
            fixed_pal[i] = pal[get_pal_index(i)];
        }

        unsigned int temp_len = hdr->width * hdr->height * 4;
        char* temp_buff = new char[temp_len];

        for (unsigned int y = 0; y < hdr->height; y++) {
            char* src_line = out_buff + 1024 + (y * hdr->width);
            RGBA* dst_line = (RGBA*) (temp_buff + y * hdr->width * 4);

            for (unsigned int x = 0; x < hdr->width; x++) {
                dst_line[x] = fixed_pal[src_line[x]];
            }
        }
        
        //delete [] out_buff;
        free(out_buff);

        out_len = temp_len;
        out_buff = temp_buff;
    }*/

    if (hdr->depth != 24) {
        std::cout << "Not RGB 24 File!" << endl;
        free(out_buff);
        return;
    }

    for (unsigned int y = 0; y < hdr->height; y++) {
        RGBA* line = (RGBA*) (out_buff + y * hdr->width * 4);

        for (unsigned int x = 0; x < hdr->width; x++) {
            if (line[x].a < 0x80) {
                line[x].a *= 2;
            } else {
                line[x].a = 0xFF;
            }
        }
    }
    
    /*as::write_bmp(filename,
                                 out_buff,
                                 out_len,
                                 hdr->width,
                                 hdr->height,
                                 4,
                                 as::WRITE_BMP_FLIP | as::WRITE_BMP_BGR);*/
    write_bmp(filename, (char*)out_buff, out_len, hdr->width, hdr->height);

    //delete [] out_buff;
    free(out_buff);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "exkkesui v1.0 by asmodean\n\n");
        fprintf(stderr, "usage: %s <input.dat>\n", argv[0]);
        return -1;
    }

    string in_filename(argv[1]);
    string prefix(get_file_prefix(in_filename, true));

    //int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);
    ifstream fd(in_filename, ios::in | ios::binary);
    if (!fd) {
        std::cout << "File not found" << endl;
        return -1;
    }

    DATHDR hdr;
    //read(fd, &hdr, sizeof(hdr));
    fd.clear();
    fd.read((char*)&hdr, sizeof(hdr));

    for (unsigned int i = 0; i < DATENTRY_COUNT; i++) {
        if (hdr.entries[i].length == 0) {
            continue;
        }

        unsigned int offset = sizeof(hdr) + (hdr.entries[i].offset * 2048);
        unsigned int len = hdr.entries[i].length * 1024;
        //char* buff = new char[len];
        //if (len <= 0) len = 1;
        char* buff = (char*)malloc(sizeof(char) * len);
        memset(buff, 0, len);

        //lseek(fd, offset, SEEK_SET);
        fd.seekg(offset);
        //read(fd, buff, len);
        fd.read(buff, len);

        string filename = stringf("%s+%05d", prefix.c_str(), i);

        if (!memcmp(buff, "CPS", 3)) {
            process_cps(filename + ".bmp", buff, len);
        } else {
            write_file(filename, buff, len);
        }//write_file(filename, buff, len);

        //delete [] buff;
        free(buff);
    }

    return 0;
}
