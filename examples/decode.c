#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image/stb_image_write.h"
#include "../libqct.h"
#include "File.c"
int main(int argc, char* argv[]){
    int f_type = 0;
    if((argc < 3) || argc > 3) { 
        printf("too many argument\nusage:\n%s [filename.qct] [filename.'png/jpg/bmp']\n", argv[0]); return 1; 
    }
    for(int i = 0; i < 80; i++){
        if(argv[1][i] == 0x00){ printf("Filename extension is not .qct '%s'\n", argv[1]); return 1; }
        if(argv[1][i] == '.'){
            if((argv[1][i + 1] == 'q' && argv[1][i + 2] == 'c' && argv[1][i + 3] == 't' && argv[1][i + 4] == 0x00) || 
               (argv[1][i + 1] == 'Q' && argv[1][i + 2] == 'C' && argv[1][i + 3] == 'T' && argv[1][i + 4] == 0x00)){
                break;
            }
            else{ printf("Filename extension is not .qct '%s'\n", argv[1]); return 1; }
        }
    }
    for(int i = 0; i < 80; i++){
        if(argv[2][i] == 0x00){ printf("Filename extension is not .png/jpg/bmp '%s'\n", argv[1]); return 1; }
        if(argv[2][i] == '.'){
            if((argv[2][i + 1] == 'p' && argv[2][i + 2] == 'n' && argv[2][i + 3] == 'g') && argv[2][i + 4] == 0x00 || 
               (argv[2][i + 1] == 'P' && argv[2][i + 2] == 'P' && argv[2][i + 3] == 'G') && argv[2][i + 4] == 0x00){
                f_type = 0;
                break;
            }
            else if((argv[2][i + 1] == 'j' && argv[2][i + 2] == 'p' && argv[2][i + 3] == 'g') && argv[2][i + 4] == 0x00 || 
                    (argv[2][i + 1] == 'j' && argv[2][i + 2] == 'p' && argv[2][i + 3] == 'g') && argv[2][i + 4] == 0x00){
                f_type = 1;
                break;   
            }
            else if((argv[2][i + 1] == 'b' && argv[2][i + 2] == 'm' && argv[2][i + 3] == 'p') && argv[2][i + 4] == 0x00 || 
                    (argv[2][i + 1] == 'B' && argv[2][i + 2] == 'M' && argv[2][i + 3] == 'P') && argv[2][i + 4] == 0x00){
                f_type = 2;
                break;
            }
            else{ printf("Filename extension is not .png/jpg/bmp '%s'\n", argv[2]); return 1; }
        }
    }
    File* yuv = open_file(argv[1], READ);
    if(!yuv){ printf("No file or direcqctry found '%s'\n", argv[2]); return 1; }
    unsigned char* in, *out;
    in = alloc_mem(yuv->file_size);
    if(read_file(in, yuv->file_size, yuv)) return 1;
    int bit_c;
    short w, h;
    out = decode_qct(in, &w, &h, yuv->file_size, &bit_c);
    if(f_type == 0) stbi_write_png(argv[2], w, h, bit_c, out, w * bit_c);
    if(f_type == 1) stbi_write_jpg(argv[2], w, h, bit_c, out, 0);
    if(f_type == 2) stbi_write_bmp(argv[2], w, h, bit_c, out);
    return 0;
}