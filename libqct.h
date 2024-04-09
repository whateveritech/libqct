struct qct_header{
    char magic[4];
    unsigned int version;
    unsigned int file_size;
//                        encode RLE:         alpha:                          
//                        0 - yes 1 - no      0 - no 1 - yes
//                          |  _______________|                   
//                          | |                        
    short flag;     // 00 0 0 0 00000000000 ------------------------------------------ 11 bit reserved.
//                     |  |___________________________________
//                     |                                      |       
//                   color sampling type:                     extended header (Exif data(soon)): 
//                   01 - yuv420 10 - yuv422 11 - yuv444      0 - no  1 - yes  
    unsigned short width;
    unsigned short height;
    unsigned int dc24_header_size;
    unsigned int dc24_header_reference_size;
};
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
typedef enum QCT_COLOR_SPACE_{
    QCT_COLOR_YUV420 = 0b00,
    QCT_COLOR_YUV422 = 0b01,
    QCT_COLOR_YUV444 = 0b10,
} QCT_COLOR_SPACE;
#pragma once
#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef __linux__
    #include <stdlib.h>
#endif
void* alloc_mem(int size){
    #ifdef _WIN32
        return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #endif
    #ifdef __linux__
        return malloc(size);
    #endif
}

void free_mem(void* address, int size){
    #ifdef _WIN32
        VirtualFree(address, size, MEM_RELEASE);
    #endif
    #ifdef __linux__
        free(address);
    #endif
}
void yuv_rgb(unsigned char y, unsigned char u, unsigned char v, unsigned char* red, unsigned char* green, unsigned char* blue){
    *red   = MAX(0, MIN(255, (298 * (y - 16) + 409 * (v - 128) + 128) >> 8));
    *green = MAX(0, MIN(255, (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8));
    *blue  = MAX(0, MIN(255, (298 * (y - 16) + 516 * (u - 128) + 128) >> 8));
}
void rgb_yuv(unsigned char red, unsigned char green, unsigned char blue, unsigned char* y, unsigned char* u, unsigned char* v){
    if(y != NULL) *y = (((66 * red + 129 * green + 25 * blue) >> 8 ) + 16);
    if(u != NULL) *u = ((( -38 * red + -74 * green + 112 * blue) >> 8) + 128);
    if(v != NULL) *v = ((( 112 * red + -94 * green + -18 * blue) >> 8) + 128);
}
void RGB_YUV_444(unsigned char* in, unsigned char* out, int width, int height, int bit_count, int* size){
    int y_pos = 0, u_pos = width * height, v_pos = u_pos * 2, a_pos = (bit_count == 4) ? u_pos * 3 : 0,
    h = 0, w = 0;
    if(!(bit_count == 4 || bit_count == 3)) return;
    for(;;){
        if(w >= width){ w = 0; h+=2; if(h >= height) break; }
        rgb_yuv(in[((h * width + w) * bit_count) + 0], in[((h * width + w) * bit_count) + 1], in[((h * width + w) * bit_count) + 2],
                &out[y_pos++], &out[u_pos++], &out[v_pos++]);
        if(bit_count == 4) out[a_pos++] = in[((h * width + w) * bit_count) + 3];
        if(w + 1 != width){
            rgb_yuv(in[((h * width + (w + 1)) * bit_count) + 0], in[((h * width + (w + 1)) * bit_count) + 1],
                    in[((h * width + (w + 1)) * bit_count) + 2], &out[y_pos++], &out[u_pos++], &out[v_pos++]);
            if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + w) * bit_count) + 3];
        }
        if(h + 1 != height){
            rgb_yuv(in[(((h + 1) * width + w) * bit_count) + 0], in[(((h + 1) * width + w) * bit_count) + 1],
                    in[(((h + 1) * width + w) * bit_count) + 2], &out[y_pos++], &out[u_pos++], &out[v_pos++]);
            if(bit_count == 4) out[a_pos++] = in[((h * width + (w + 1)) * bit_count) + 3];
            if(w + 1 != width){
                rgb_yuv(in[(((h + 1) * width + (w + 1)) * bit_count) + 0], in[(((h + 1) * width + (w + 1)) * bit_count) + 1],
                        in[(((h + 1) * width + (w + 1)) * bit_count) + 2], &out[y_pos++], &out[u_pos++], &out[v_pos++]);
                if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + (w + 1)) * bit_count) + 3];
            }
        }
        w+=2;
    }
    *size = (bit_count == 4) ? a_pos : v_pos;
}
void RGB_YUV_422(unsigned char* in, unsigned char* out, int width, int height, int bit_count, int* size){
    int y_pos = 0, u_pos = width * height, v_pos = u_pos + ((u_pos / 2) * 2), a_pos = (bit_count == 4) ? u_pos + ((u_pos / 2) * 3) : 0;
    int h = 0, w = 0;
    for(;;){
        if(w >= width){ w = 0; h+=2; if(h >= height) break; }
        rgb_yuv(in[((h * width + w) * bit_count) + 0], in[((h * width + w) * bit_count) + 1],
                in[((h * width + w) * bit_count) + 2], &out[y_pos++], NULL, NULL);
        rgb_yuv(in[((h * width + w) * bit_count) + 0], in[((h * width + w) * bit_count) + 1],
                in[((h * width + w) * bit_count) + 2], NULL, &out[u_pos++], &out[v_pos++]);
        if(bit_count == 4) out[a_pos++] = in[((h * width + w) * bit_count) + 3];
        if(w + 1 != width){
            rgb_yuv(in[((h * width + (w + 1)) * bit_count) + 0], in[((h * width + (w + 1)) * bit_count) + 1],
                    in[((h * width + (w + 1)) * bit_count) + 2], &out[y_pos++], NULL, NULL);
            if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + w) * bit_count) + 3];
        }
        if(h + 1 != height){
            rgb_yuv(in[(((h + 1) * width + w) * bit_count) + 0], in[(((h + 1) * width + w) * bit_count) + 1], 
                    in[(((h + 1) * width + w) * bit_count) + 2], &out[y_pos++], NULL, NULL);
            rgb_yuv(in[(((h + 1) * width + w) * bit_count) + 0], in[(((h + 1) * width + w) * bit_count) + 1], 
                    in[(((h + 1) * width + w) * bit_count) + 2], NULL, &out[u_pos++], &out[v_pos++]);
            if(bit_count == 4) out[a_pos++] = in[((h * width + (w + 1)) * bit_count) + 3];
            if(w + 1 != width){
                rgb_yuv(in[(((h + 1) * width + (w + 1)) * bit_count) + 0], in[(((h + 1) * width + (w + 1)) * bit_count) + 1], 
                        in[(((h + 1) * width + (w + 1)) * bit_count) + 2],
                        &out[y_pos++], NULL, NULL);
                if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + (w + 1)) * bit_count) + 3];
            }
        }
        w+=2;
    }
    *size = (bit_count == 4) ? a_pos : v_pos;
}
void RGB_YUV_420(unsigned char* in, unsigned char* out, int width, int height, int bit_count, int* size) {
    int h = 0, w = 0, u_pos = width * height, v_pos = (u_pos / 4) + u_pos, a_pos = (bit_count == 4) ? (u_pos / 4) + v_pos : 0, pos = 0;
    unsigned char r, g, b;
    for(h = 0; h < height;){
        for(w = 0; w < width;){
            rgb_yuv(in[((h * width + w) * bit_count) + 0], in[((h * width + w) * bit_count) + 1], 
                    in[((h * width + w) * bit_count) + 2], &out[pos++], NULL, NULL);
            if(bit_count == 4) out[a_pos++] = in[((h * width + w) * bit_count) + 3];
            if(w + 1 != width){
                rgb_yuv(in[((h * width + (w + 1)) * bit_count) + 0], in[((h * width + (w + 1)) * bit_count) + 1], 
                        in[((h * width + (w + 1)) * bit_count) + 2], &out[pos++], NULL, NULL);
                if(bit_count == 4) out[a_pos++] = in[((h * width + (w + 1)) * bit_count) + 3];
            }
            if(h + 1 != height){
                rgb_yuv(in[(((h + 1) * width + w) * bit_count) + 0], in[(((h + 1) * width + w) * bit_count) + 1], 
                        in[(((h + 1) * width + w) * bit_count) + 2], &out[pos++], NULL, NULL);
                if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + (w)) * bit_count) + 3];
                if(w + 1 != width){
                    rgb_yuv(in[(((h + 1) * width + (w + 1)) * bit_count) + 0], in[(((h + 1) * width + (w + 1)) * bit_count) + 1], 
                            in[(((h + 1) * width + (w + 1)) * bit_count) + 2], &out[pos++], NULL, NULL);
                    if(bit_count == 4) out[a_pos++] = in[(((h + 1) * width + (w + 1)) * bit_count) + 3];
                }
            }
            rgb_yuv(in[((h * width + w) * bit_count) + 0], in[((h * width + w) * bit_count) + 1], 
                    in[((h * width + w) * bit_count) + 2], NULL, &out[u_pos++], &out[v_pos++]);
            w+=2;
        }
        h+=2;
    }
    *size = (bit_count == 4) ? a_pos : v_pos;
}
void YUV_RGB_444(unsigned char* in, unsigned char* out, short width, short height, int bit_count){
    int h = 0, w = 0, y_pos = 0, u_pos = width * height, v_pos = u_pos * 2, a_pos = (bit_count == 4) ? u_pos * 3 : 0;
    for(;;){
        if(w >= width){ w = 0; h+=2; if(h >= height) break; }
        yuv_rgb(in[y_pos++], in[u_pos++], in[v_pos++], &out[((h * width + w) * bit_count) + 0], &out[((h * width + w) * bit_count) + 1], 
                                                       &out[((h * width + w) * bit_count) + 2]);
        if(bit_count == 4) out[((h * width + w) * bit_count) + 3] = in[a_pos++];
        if(w + 1 != width){
            yuv_rgb(in[y_pos++], in[u_pos++], in[v_pos++], &out[((h * width + (w + 1)) * bit_count) + 0],
                    &out[((h * width + (w + 1)) * bit_count) + 1], &out[((h * width + (w + 1)) * bit_count) + 2]);
            if(bit_count == 4) out[((h * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
        }
        if(h + 1 != height){
            yuv_rgb(in[y_pos++], in[u_pos++], in[v_pos++], &out[(((h + 1) * width + w) * bit_count) + 0], 
                    &out[(((h + 1) * width + w) * bit_count) + 1], &out[(((h + 1) * width + w) * bit_count) + 2]);
            if(bit_count == 4) out[(((h + 1) * width + w) * bit_count) + 3] = in[a_pos++];
            if(w + 1 != width){
                yuv_rgb(in[y_pos++], in[u_pos++], in[v_pos++], &out[(((h + 1) * width + (w + 1)) * bit_count) + 0], 
                                                               &out[(((h + 1) * width + (w + 1)) * bit_count) + 1], 
                                                               &out[(((h + 1) * width + (w + 1)) * bit_count) + 2]);
                if(bit_count == 4) out[(((h + 1) * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
            }
        }
        w+=2;
    }
}
void YUV_RGB_422(unsigned char* in, unsigned char* out, short width, short height, int bit_count){
    int y_pos = 0, u_pos = width * height, v_pos = u_pos + ((u_pos / 2) * 2), a_pos = (bit_count == 4) ? u_pos + ((u_pos / 2) * 3) : 0;
    int h = 0, w = 0;
    unsigned char u, v;
    for(;;){
        if(w >= width){ w = 0; h+=2; if(h >= height) break; }
        u = in[u_pos++], v = in[v_pos++]; 
        yuv_rgb(in[y_pos++], u, v,
                &out[((h * width + w) * bit_count) + 0], &out[((h * width + w) * bit_count) + 1], 
                &out[((h * width + w) * bit_count) + 2]);
        if(bit_count == 4) out[((h * width + w) * bit_count) + 3] = in[a_pos++];
        if(w + 1 != width){
            yuv_rgb(in[y_pos++], u, v,
                    &out[((h * width + (w + 1)) * bit_count) + 0], &out[((h * width + (w + 1)) * bit_count) + 1], 
                    &out[((h * width + (w + 1)) * bit_count) + 2]);
            if(bit_count == 4) out[((h * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
        }
        if(h + 1 != height){
            u = in[u_pos++], v = in[v_pos++]; 
            yuv_rgb(in[y_pos++], u, v,
                    &out[(((h + 1) * width + w) * bit_count) + 0], &out[(((h + 1) * width + w) * bit_count) + 1], 
                    &out[(((h + 1) * width + w) * bit_count) + 2]);
            if(bit_count == 4) out[(((h + 1) * width + w) * bit_count) + 3] = in[a_pos++];
            if(w + 1 != width){
                yuv_rgb(in[y_pos++], u, v,
                        &out[(((h + 1) * width + (w + 1)) * bit_count) + 0], &out[(((h + 1) * width + (w + 1)) * bit_count) + 1], 
                        &out[(((h + 1) * width + (w + 1)) * bit_count) + 2]);
                if(bit_count == 4) out[(((h + 1) * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
            }
        }
        w+=2;
    }
}

void YUV_RGB_420(unsigned char* in, unsigned char* out, short width, short height, int bit_count){
    int u_pos = width * height, v_pos = (u_pos / 4) + u_pos, w = 0, h = 0, a_pos = (bit_count == 4) ? (u_pos / 4) + v_pos : 0;
    unsigned char y, u, v;
    for(int i = 0;;){
        if(w >= width){ 
            w = 0;
            h+=2;
            if(h >= height) break;
        }
        u = in[u_pos++];
        v = in[v_pos++];
        y = in[i++];
        yuv_rgb(y, u, v, &out[((h * width + w) * bit_count) + 0], 
                            &out[((h * width + w) * bit_count) + 1], 
                            &out[((h * width + w) * bit_count) + 2]);
        if(bit_count == 4) out[((h * width + w) * bit_count) + 3] = in[a_pos++];
        if(w + 1 != width){
            y = in[i++];
            yuv_rgb(y, u, v, &out[((h * width + (w + 1)) * bit_count) + 0], 
                                &out[((h * width + (w + 1)) * bit_count) + 1], 
                                &out[((h * width + (w + 1)) * bit_count) + 2]);
            if(bit_count == 4) out[((h * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
        }
        if(h + 1 != height){
            y = in[i++];
            yuv_rgb(y, u, v, &out[(((h + 1) * width + w) * bit_count) + 0], 
                                &out[(((h + 1) * width + w) * bit_count) + 1], 
                                &out[(((h + 1) * width + w) * bit_count) + 2]);
            if(bit_count == 4) out[(((h + 1) * width + w) * bit_count) + 3] = in[a_pos++];
            if(w + 1 != width){
                y = in[i++];
                yuv_rgb(y, u, v, &out[(((h + 1) * width + (w + 1)) * bit_count) + 0], 
                                    &out[(((h + 1) * width + (w + 1)) * bit_count) + 1], 
                                    &out[(((h + 1) * width + (w + 1)) * bit_count) + 2]);
                if(bit_count == 4) out[(((h + 1) * width + (w + 1)) * bit_count) + 3] = in[a_pos++];
            }
        }
        w+=2;
    }
}
void encode_RLE(char* in, char* out, int* pos_out, int size_in){
    unsigned int j = 0, pos = 0, i = 0, l;
    *pos_out = 0;
    char found, pre = 0, byte_, *first;
    for(;;){
        if(i == size_in) break;
        found = 0x00;
        pre = in[i];
        for(l = 1; l < 128; l++){
            if(i + l == size_in) break;
            if(pre != in[i + l]){
                pre = in[i + l];
                found = 0x01;
            }
            else{
                l--;
                break;
            }
        }
        if(found){
            out[*pos_out] = (l - 1);
            *pos_out+=1;
            for(int o = 0; o < l; o++){
                out[*pos_out] = in[i++];
                *pos_out+=1;
            }
        }
        else{
            if(i == size_in) break;
            byte_ = in[i++];
            for(l = 0; l < 520093696; l++){
                if(i == size_in) break;
                if(byte_ != in[i]) break;
                i++;
            }
            if(l < 32){
                out[*pos_out] = 0x80 | l; 
                *pos_out+=1;
            }
            else if(l < 8192){
                out[*pos_out] = 0xa0 | (((l >> 8) & 0x1f));
                out[*pos_out + 1] = l & 0xff;
                *pos_out+=2;
            }
            else if(l < 2097152){
                out[*pos_out] = 0xc0 | (((l >> 16) & 0x1f));
                out[*pos_out + 1] = (l & 0xff00) >> 8;
                out[*pos_out + 2] = (l & 0xff);
                *pos_out+=3;
            }
            else if(l < 520093696){
                out[*pos_out] = 0xe0 | (((l >> 24) & 0x1f));
                out[*pos_out + 1] = (l & 0xff0000) >> 16;
                out[*pos_out + 2] = (l & 0xff00) >> 8;
                out[*pos_out + 3] = (l & 0xff);
                *pos_out+=4;
            }
            out[*pos_out] = byte_;
            *pos_out+=1;
        }
    }
}
void decode_rle(char* in, char* out, int* output_size, int input_size){
    int i = 0, s = 0;
    *output_size = 0;
    char byte = 0;
    for(i = 0;;){
        if(i == input_size) break;
        if(in[i] & 0x80){
                 if(((in[i] >> 5) & 0x03) == 0){s = in[i] & 0x1f; i++;}  
            else if(((in[i] >> 5) & 0x03) == 1){s = (((in[i] & 0x001f) << 8) | (in[i + 1] & 0x00ff)); i+=2;}
            else if(((in[i] >> 5) & 0x03) == 2){s = (((in[i] & 0x001f) << 16) | ((in[i + 1] & 0x00ff) << 8) | (in[i + 2] & 0x00ff)); i+=3;}
            else if(((in[i] >> 5) & 0x03) == 3){s = (((in[i] & 0x001f) << 24) | ((in[i + 1] & 0x00ff) << 16) | ((in[i + 2] & 0x00ff) << 8) | 
                                                    (in[i + 3] & 0x00ff)); i+=4;}
            byte = in[i++];
            for(int j = 0; j < s + 1; j++){
                out[*output_size] = byte;
                *output_size+=1;
            }
        }
        else{
            s = in[i++];
            for(int j = 0; j < s + 1; j++){
                if(i == input_size) break;
                out[*output_size] = in[i++];
                *output_size+=1;
            }
        }
    }
}
char* encode_qct(char* in_rgb, short width, short height, int b_c, int* size, QCT_COLOR_SPACE color){
    unsigned int pos_input = 0, input_size = 0, output_size = 0, com1, com2, p, size_in = 0, head_s = 0, i, clasp_s = 0, bit_count = b_c, j;
    unsigned char previous_chain, chain_size, clasp, chain[256], *input_data, *output_data, bit_side = 0, pre = 0, *uncompressed_size; 
    unsigned char* out = (unsigned char*)in_rgb,
                 * in = (unsigned char*)alloc_mem((height * width * bit_count) * 2),
                 * head = (unsigned char*)alloc_mem(height * width),
                 * clasps = (unsigned char*)alloc_mem(height * width), *file_output, 
                 * headers, *header_ref, *deltas, *header_ptr;
    struct qct_header header = {0};
    if(in == NULL || head == NULL || clasps == NULL) goto QCT_ENC_ERR;
    if(!(bit_count == 3 || bit_count == 4)) goto QCT_ENC_ERR;
    if(bit_count == 4) header.flag |= 0x0800;
    if(color == QCT_COLOR_YUV420){
        RGB_YUV_420(out, in, width, height, bit_count, (int*)&input_size);
        header.flag |= 0x4000;
    }
    else if(color == QCT_COLOR_YUV422){
        RGB_YUV_422(out, in, width, height, bit_count, (int*)&input_size);
        header.flag |= 0x8000;
    }
    else if(color == QCT_COLOR_YUV444){
        RGB_YUV_444(out, in, width, height, bit_count, (int*)&input_size);
        header.flag |= 0xc000;
    }
    else{
        goto QCT_ENC_ERR;
    }
    header.width = width;
    header.height = height;
    header.magic[0] = 'q', header.magic[1] = 'c', header.magic[2] = 't', header.magic[3] = ' '; 
    header.version = 1;
    header.file_size = input_size;
    input_data = in, output_data = out;
    for(;;){
        uncompressed_size = &head[head_s];
        if(pos_input >= input_size) break;
        previous_chain = input_data[pos_input];
        for(p = 0, chain_size = 1; chain_size < 128; p++){
            if(pos_input + chain_size == input_size) break;
            pre = 0; com1 = input_data[pos_input + chain_size]; com2 = previous_chain;
            if(previous_chain > input_data[pos_input + chain_size]){
                pre = 1; com1 = previous_chain; com2 = input_data[pos_input + chain_size]; 
            }
            if(com1 - com2 > 7){
                if(chain_size == 1){
                    head_s++;
                }
                previous_chain = input_data[pos_input + chain_size];
                chain_size++;
            }
            else break;
        }
        *uncompressed_size = (chain_size ^ 0x80) - 1;
        if(p){
            for(i = 0; i < chain_size; i++){
                output_data[output_size++] = input_data[pos_input++];
            }
        }
        else{
            if(pos_input == input_size) break; 
            bit_side = 0;
            clasp = input_data[pos_input];
            previous_chain = clasp;
            pos_input++;
            chain_size = 0;
            pre = 0;
            for(i = pos_input, p = 0; p < 127; p++){
                pre = 0; com1 = input_data[pos_input]; com2 = previous_chain;
                if(pos_input == input_size) break;
                if(previous_chain > input_data[pos_input]){ 
                    pre = 1; 
                    com1 = previous_chain; com2 = input_data[pos_input]; 
                }
                if(com1 - com2 < 8){
                    if(!bit_side){
                        chain[chain_size] = ((com1 - com2) ^ ((pre) ? 0x08 : 0x00)) << 4;
                        previous_chain = input_data[pos_input];
                        bit_side = 1;
                    }
                    else{
                        chain[chain_size] |= ((com1 - com2) ^ ((pre) ? 0x08 : 0x00));
                        previous_chain = input_data[pos_input];
                        bit_side = 0;
                        chain_size++;
                    }
                    pos_input++;
                }
                else break;
            }
            if(p && (p) % 2 == 1){ chain_size++; }
            head[head_s++] = p;
            clasps[clasp_s++] = clasp;
            for(i = 0; i < chain_size; i++){
                output_data[output_size++] = chain[i];
            }
        }
    }
    headers = input_data;
    header_ref = &input_data[head_s];
    deltas = &input_data[head_s + clasp_s];
    for(i = 0; i < head_s; i++){
        headers[i] = head[i];
    }
    for(i = 0; i < clasp_s; i++){
        header_ref[i] = clasps[i];
    }
    for(i = 0; i < output_size; i++){
        deltas[i] = output_data[i];
    }
    header.dc24_header_size = head_s - 1;
    header.dc24_header_reference_size = clasp_s - 1;
    output_size+=clasp_s + head_s;
    encode_RLE((char*)input_data, (char*)output_data, (int*)&input_size, output_size); 
    j = 0;
    if(input_size < output_size){
        size_in = input_size;
        file_output = input_data;
        for(i = sizeof(struct qct_header), j = 0; j < size_in; i++, j++){
            file_output[i] = output_data[j];
        }
    }
    else{
        header.flag |= 0x0100;
        size_in = output_size;
        file_output = output_data;
        for(i = sizeof(struct qct_header), j = 0; j < size_in; i++, j++){
            file_output[i] = input_data[j];
        }
    }
    header_ptr = (unsigned char*)&header;
    for(i = 0; i < sizeof(struct qct_header); i++){
        file_output[i] = header_ptr[i];
    }
    *size = sizeof(struct qct_header) + size_in;
    free_mem(head, input_size);
    free_mem(clasps, input_size);
    return (char*)file_output;
    QCT_ENC_ERR:
        free_mem(in, input_size);
        free_mem(head, input_size);
        free_mem(clasps, input_size);
        return NULL;
}
char* decode_qct(char* buffer, short* width, short* height, int size, int* bit_c){
    if(!buffer) return NULL;
    struct qct_header header;
    unsigned char* in = (unsigned char*)buffer, *out, *header_ptr = (unsigned char*)&header, *yuv_;
    for(int i = 0; i < sizeof(struct qct_header); i++){
        header_ptr[i] = in[i];
    }
    in+=sizeof(struct qct_header);
    unsigned char previous_chain, chain_size, com1, com2, delta, padded = 0, add = 0;
    unsigned int p = 0, output_size = 0, bit_side = 0, size_in = size, header_pos = 0, header_reference_pos = 0, fpos = 0, 
    bit_count = *bit_c = (header.version && (header.flag & 0x0800)) ? 4 : 3;
    out = (unsigned char*)alloc_mem(header.width * header.height * bit_count);
    unsigned char* output, *input, *input_header, *input_ref, * input_delta; 
    *width = header.width;
    *height = header.height;
    if((header.flag & 0x0100)){
        output = out, input = in;
        size_in = size - sizeof(struct qct_header);
    }
    else{
        yuv_ = (unsigned char*)alloc_mem(header.file_size);
        decode_rle((char*)in, (char*)out, (int*)&size_in, size - sizeof(struct qct_header));
        output = yuv_, input = out;
    }
    input_header = input, input_ref = &input[header.dc24_header_size + 1],
    input_delta = &input[(header.dc24_header_size + header.dc24_header_reference_size) + 2];
    for(p = 0;;){
        if(header_pos + p + header_reference_pos == size_in) break;
        chain_size = input_header[header_pos++];
        bit_side = 0;
        if(chain_size & 0x80){
            chain_size &= 0x7f;
            for(int i = 0; i < chain_size + 1; i++){
                output[output_size++] = input_delta[p++]; fpos++;
            }
        }
        else{
            previous_chain = input_ref[header_reference_pos++]; fpos++;
            output[output_size++] = previous_chain;
            padded = 0;
            if(chain_size % 2 == 1) {padded = 1;}
            for(int i = 0, bit_side = 0; i < chain_size; i++){
                if(!chain_size) break;
                if(!bit_side) {delta = input_delta[p++]; fpos++;}
                if(!bit_side){
                    if((delta & 0x80)){
                        com1 = previous_chain;
                        com2 = ((delta & 0x7f) >> 4);
                        output[output_size] = com1 - com2;
                    }
                    else{
                        com1 = ((delta >> 4));
                        com2 = previous_chain;
                        output[output_size] = com1 + com2;
                    }
                    previous_chain = output[output_size];
                    bit_side = 1;
                }
                else{
                    if(padded && i + 1 == (chain_size)) { p++; break; }
                    if((delta & 0x08)){
                        com1 = previous_chain;
                        com2 = (delta) & 0x07;
                        output[output_size] = com1 - com2;
                    }
                    else{
                        com1 = ((delta & 0x07));
                        com2 = previous_chain;
                        output[output_size] = com1 + com2;
                    }
                    previous_chain = output[output_size];
                    bit_side = 0;
                }
                output_size++;
            }
        }
    }
    unsigned char* yuv_inp = output,* rgb_out = input;
    if(header.version){
                 if((header.flag & 0xc000) == 0x4000) YUV_RGB_420(yuv_inp, rgb_out, header.width, header.height, bit_count);
        else if((header.flag & 0xc000) == 0x8000) YUV_RGB_422(yuv_inp, rgb_out, header.width, header.height, bit_count);
        else if((header.flag & 0xc000) == 0xc000) YUV_RGB_444(yuv_inp, rgb_out, header.width, header.height, bit_count);
        else goto QCT_DEC_ERR;
    }
    else YUV_RGB_420(yuv_inp, rgb_out, header.width, header.height, bit_count);
    return (char*)rgb_out;
    QCT_DEC_ERR:
    free_mem(out, header.width * header.height * bit_count);
    return NULL;
}
void free_mem_qct(char* buffer, int size){
    free_mem(buffer, size);
}
