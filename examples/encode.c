#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image/stb_image_write.h"
#include "../libqct.h"
#include "File.c"

int main(int argc, char* argv[]){
    int yuv = 0;
    if((argc < 3) || argc > 4){ 
        printf("too many argument\nusage:\n%s [filename.qct] [filename.'png/jpg/bmp'] [yuv'420(default)/422/444']\n", argv[0]); return 1; 
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
                break;
            }
            else if((argv[2][i + 1] == 'j' && argv[2][i + 2] == 'p' && argv[2][i + 3] == 'g') && argv[2][i + 4] == 0x00 || 
                    (argv[2][i + 1] == 'j' && argv[2][i + 2] == 'p' && argv[2][i + 3] == 'g') && argv[2][i + 4] == 0x00){
                break;   
            }
            else if((argv[2][i + 1] == 'b' && argv[2][i + 2] == 'm' && argv[2][i + 3] == 'p') && argv[2][i + 4] == 0x00 || 
                    (argv[2][i + 1] == 'B' && argv[2][i + 2] == 'M' && argv[2][i + 3] == 'P') && argv[2][i + 4] == 0x00){
                break;
            }
            else{ printf("Filename extension is not .png/jpg/bmp '%s'\n", argv[2]); return 1; }
        }
    }
    if(argc == 4){
        if(argv[3][0] == 'y' && argv[3][1] == 'u' && argv[3][2] == 'v'){
            if(argv[3][3] == '4' && argv[3][4] == '2' && argv[3][5] == '0' && argv[3][6] == 0x00) yuv = 0;
            else if(argv[3][3] == '4' && argv[3][4] == '2' && argv[3][5] == '2' && argv[3][6] == 0x00) yuv = 1;
            else if(argv[3][3] == '4' && argv[3][4] == '4' && argv[3][5] == '4' && argv[3][6] == 0x00) yuv = 2;
            else{ printf("unkown command '%s'\n", argv[3]); return 1; }
        }
        else{ printf("unkown command '%s'\n", argv[3]); return 1; }
    }
    File* qct = open_file(argv[1], WRITE);
    int width = 0, height = 0, bit_count = 0, size_in = 0, yuv_size = 0;
    unsigned char* in = stbi_load(argv[2], &width, &height, &bit_count, 0);
    if(!in){ printf("No file or direcqctry found '%s'\n", argv[2]); return 1; }
    char* file_output;
    file_output = encode_qct((char*)in, width, height, bit_count, &size_in, (QCT_COLOR_SPACE)yuv);
    if(write_file(file_output, size_in, qct)) return 1;
    return 0;
}