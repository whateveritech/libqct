#include "../multimedia.c"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image/stb_image_write.h"

struct qct_header{
    char magic[4];
    unsigned int version;
    unsigned int file_size;
//                        encode RLE:         uniform quantization:
//                        0 - yes 1 - no      00 - (none) 01 - (by 1) 10 - (by 2) 11 - (by 3) 
//                          |  _______________|
//                          | |
    short flag;     // 00 0 0 00 0000000000
//                     |  |______________________
//                     |                         |       
//                   color sampling type:       extended header:
//                   00 - yuv420 01 - yuv422      0 - no  1 - yes  
    unsigned short width;
    unsigned short height;
    unsigned int dc24_header_size;
    unsigned int dc24_header_reference_size;
};

void Bitmap2Yuv420p(unsigned char* destination, unsigned char* rgb, int width, int height, int* size) {
    int image_size = width * height;
    int upos = image_size;
    int vpos = upos + upos / 4;
    for(int i = 0; i < image_size; ++i ) {
        unsigned char r = rgb[3 * i];
        unsigned char g = rgb[3 * i + 1];
        unsigned char b = rgb[3 * i + 2];
        destination[i] = ((( ( 66 * r + 129 * g + 25 * b ) >> 8 ) + 16));
        if (!((i / width) % 2) && !(i % 2)) {
            destination[upos++] = ( (((( -38 * r + -74 * g + 112 * b ) >> 8) + 128)));
            destination[vpos++] = ( (((( 112 * r + -94 * g + -18 * b ) >> 8) + 128)));
        }
    }
    *size = vpos;
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

int main(int argc, char* argv[]){
    File* to = open_file("New.qci", WRITE), *tt = open_file("New.tt", WRITE) ;
    File* yuv = open_file("out.yuv", READ);
    if((argc < 2)) {printf("no name %d\n", argc); while(1);}
    int width = 0, height = 0, bit_count = 0, size_in = 0, yuv_size = 0;
    unsigned char* in = stbi_load(argv[1], &width, &height, &bit_count, 3);
    unsigned char* out = alloc_mem(width * height * 3),
                 * head = alloc_mem(width * height * 3),
                 * clasps = alloc_mem(width * height * 3), *file_output;
    Bitmap2Yuv420p(out, in, width, height, &yuv_size);
    size_in = yuv_size;
    unsigned char previous_chain, chain_size, first_chain, clasp, chain[256], *input_data = out, *output_data = in, bit_side = 0, 
    pre = 0, ppos = 0, _3, sign_bits[16], n_bit, *uncompressed_size;
    unsigned int pos_input = 0, input_size = size_in, output_size = 0, com1, com2, p, sign_bits_size = 0;
    int head_s = 0, i, clasp_s = 0;
    struct qct_header header = {0};
    header.width = width;
    header.height = height;
    header.magic[0] = 'q', header.magic[1] = 'c', header.magic[2] = 't', header.magic[3] = 'f'; 
    header.version = 0;
    for(;;){
        uncompressed_size = &head[head_s];
        if(pos_input == input_size) break;
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
            if(previous_chain > input_data[pos_input]){ pre = 1; com1 = previous_chain; com2 = input_data[pos_input]; }
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
                    printf("%x\n", chain[chain_size]);
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
    char* headers = input_data;
    char* header_ref = &input_data[head_s];
    char* deltas = &input_data[head_s + clasp_s];
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
    if(write_file(input_data, output_size, tt)) return 1;
    encode_RLE(input_data, output_data, &input_size, output_size); 
    int j = 0;
    if(input_size < output_size){
        size_in = input_size;
        for(i = sizeof(struct qct_header), j = 0; j < size_in; i++, j++){
            input_data[i] = output_data[j];
        }
        file_output = input_data;
    }
    else{
        header.flag |= 0x0100;
        size_in = output_size;
        for(i = sizeof(struct qct_header), j = 0; j < size_in; i++, j++){
            output_data[i] = input_data[j];
        }
        file_output = output_data;
    }
    char* header_ptr = (char*)&header;
    header.file_size = sizeof(struct qct_header) + size_in;
    for(i = 0; i < sizeof(struct qct_header); i++){
        file_output[i] = header_ptr[i];
    }
    if(write_file(file_output, header.file_size, to)) return 1;
    printf("%f %f\n", (float)output_size / (float)yuv_size * 100.0, (float)input_size / (float)yuv_size * 100.0);
    while(1);
    return 0;
}