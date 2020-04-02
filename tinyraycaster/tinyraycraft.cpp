#include <iostream>
#include <vector>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <sstream>
#include <iomanip>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, 
		    const uint8_t a=255) {
    return (a << 24) + (b << 16) + (g << 8) + r;
}

void unpack_color(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, 
		  uint8_t &a) {
    r = (color >> 0) & 255;
    g = (color >> 8) & 255;
    b = (color >> 16) & 255;
    a = (color >> 24) & 255;
}

void drop_ppm_image(const std::string filename, 
		    const std::vector<uint32_t> &image, 
		    const size_t w, const size_t h) {
    assert(image.size() == w * h);
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for (size_t i = 0; i < h * w; i++) {
        uint8_t r, g, b, a;
        unpack_color(image[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g);
	ofs << static_cast<char>(b);
    }
    ofs.close();
}

void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, 
		    const size_t img_h, const size_t rect_w, 
		    const size_t rect_h, const size_t rect_x, 
		    const size_t rect_y, const uint32_t color) {
    assert(img.size() == img_w * img_h);
    for (size_t i = 0; i < rect_h; i++) {
        for (size_t j = 0; j < rect_w; j++){
            size_t cx = rect_x + j;
            size_t cy = rect_y + i;
            if (cx >= img_w || cy >= img_h) continue;
            img[img_w * cy + cx] = color;
        }
    }
}

bool load_texture(const std::string filename, std::vector<uint32_t> &texture, 
		  size_t &text_size, size_t &text_cnt) {
    int nchannels = -1, w, h;
    unsigned char *pixmap = stbi_load(filename.c_str(), &w, &h, &nchannels, 0);
    if (!pixmap) {
        std::cerr << "Error: can not load the textures" << std::endl;
        return false;
    }
    if (nchannels != 4) {
        std::cerr << "Error: the texture must be a 32 bit image" << std::endl;
        stbi_image_free(pixmap);
        return false;
    }

    text_cnt = w / h;
    text_size = w / text_cnt;
    if (w != h * int(text_cnt)) {
        std::cerr << "Error: the texture file must contain N square textures packed horizontally" << std::endl;
        return false;
    }

    texture = std::vector<uint32_t>(w * h);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            uint8_t r = pixmap[(i * w + j) * 4 + 0];
            uint8_t g = pixmap[(i * w + j) * 4 + 1];
            uint8_t b = pixmap[(i * w + j) * 4 + 2];
            uint8_t a = pixmap[(i * w + j) * 4 + 3];
            texture[i * w + j] = pack_color(r, g, b, a);
        }
    }
    stbi_image_free(pixmap);
    return true;
}

int main() {
    // draw the screen
    const size_t win_w = 512 * 2;
    const size_t win_h = 512;
    std::vector<uint32_t> framebuffer(win_w * win_h, pack_color(255, 255, 255));

    // hardcode a map
    const size_t map_w = 16;
    const size_t map_h = 16;
    const char map[] = "0000222222220000"\
                       "1              0"\
                       "1      11111   0"\
                       "1     0        0"\
                       "0     0  1110000"\
                       "0     3        0"\
                       "0   10000      0"\
                       "0   3   11100  0"\
                       "5   4   0      0"\
                       "5   4   1  00000"\
                       "0       1      0"\
                       "2       1      0"\
                       "0       0      0"\
                       "0 0000000      0"\
                       "0              0"\
                       "0002222222200000"; 
    assert(sizeof(map) == map_w * map_h + 1);

    // load wall textures
    std::vector<uint32_t> walltext;
    size_t walltext_size;
    size_t walltext_cnt;
    if (!load_texture("./walltext.png", walltext, walltext_size, walltext_cnt)) {
        std::cerr << "Failed to load wall textures" << std::endl;
        return -1;
    }

    // draw the map
    const size_t rect_w = win_w / (map_w * 2);
    const size_t rect_h = win_h / map_h;
    for (size_t i = 0; i < map_h; i++) {
        for (size_t j = 0; j < map_w; j++) {
            if (map[i * map_w + j] == ' ') continue;
            size_t rect_x = j * rect_w;
            size_t rect_y = i * rect_h;
            size_t texid = map[i * map_w + j] - '0';
            assert(texid < walltext_cnt);
            draw_rectangle(framebuffer, win_w, win_h, rect_w, rect_h, 
			   rect_x, rect_y, walltext[texid * walltext_size]);
        }
    }

    // draw field of view of the player
    float player_x = 3.456;  // player x position
    float player_y = 2.345;  // player y position
    float player_a = 1.523;  // player view direction
    const float fov = M_PI / 3.;  // filed of view
    for (size_t i = 0; i < win_w / 2; i++) {
        float angle = player_a - fov / 2 + fov * i / float(win_w / 2);
        for (float t = 0; t < 23; t += 0.05) {
            float cx = player_x + t * cos(angle);
            float cy = player_y + t * sin(angle);

            size_t pix_x = cx * rect_w;
            size_t pix_y = cy * rect_h;
            framebuffer[pix_x + pix_y * win_w] = pack_color(160, 160, 160);
            if (map[int(cy) * map_w + int(cx)] != ' ') {
                size_t column_height = win_h / (t * cos(angle - player_a));
                size_t texid = map[int(cy) * map_w + int(cx)] - '0';
                assert(texid < walltext_cnt);
                size_t rect_x = win_w / 2 + i;
                size_t rect_y = (win_h - column_height) / 2;
                draw_rectangle(framebuffer, win_w, win_h, 1, column_height, 
			       rect_x, rect_y, walltext[texid * walltext_cnt]);
                break;
            }
        }
    }

    // dump to a ppm file
    drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);

    return 0;
}
