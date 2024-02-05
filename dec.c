#include "../multimedia.c"

struct qct_header{
    char magic[4];
    unsigned int version;
    unsigned int file_size;
//                        encode RLE:
//                        0 - yes 1 - no
//                          |
//                          |
    short flag;     // 00 0 0 000000000000
//                     |  |______________________
//                     |                         |       
//                   color sampling type:       extended header:
//                   00 - yuv420 01 - yuv422      0 - no  1 - yes  
    unsigned short width;
    unsigned short height;
    unsigned int dc24_header_size;
    unsigned int dc24_header_reference_size;
};

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
    printf("%d\n", *output_size);
}

int main(){
    File* yuv = open_file("New.qci", READ);
    File* oyuv = open_file("New.yuv", WRITE);
    struct qct_header header;
    unsigned char* in = alloc_mem(536870911), *out = alloc_mem(536870911);
    if(read_file(in, yuv->file_size, yuv)) return 1;
    unsigned char previous_chain, chain_size, com1, com2, delta, padded;
    unsigned int p = 0, output_size = 0, bit_side = 0, size_in = yuv->file_size, header_pos = 0, header_reference_pos = 0, fpos = 0; 
    for(char* header_ptr = (char*)&header; p < sizeof(struct qct_header); p++){
        header_ptr[p] = in[p];
    }
    in+=sizeof(struct qct_header);
    char* output, *input, *input_header, *input_ref, * input_delta; 
    if((header.flag & 0x0100)){
        printf("%d %d\n", header.dc24_header_size, header.dc24_header_reference_size); 
        output = out, input = in; 
        size_in = yuv->file_size - sizeof(struct qct_header);         
    }
    else{
        decode_rle(in, out, &size_in, yuv->file_size - sizeof(struct qct_header));
        output = in, input = out;
    }
    if(write_file(input, size_in, oyuv)) return 1;
    input_header = input, input_ref = &input[header.dc24_header_size + 1],
    input_delta = &input[(header.dc24_header_size + header.dc24_header_reference_size) + 2];
    for(;;){
        if(fpos == size_in) break;
        chain_size = input_header[header_pos++]; fpos++;
        bit_side = 0;
        if(chain_size & 0x80){
            chain_size &= 0x7f;
            for(int i = 0; i < chain_size + 1; i++){
                output[output_size++] = input_delta[p++]; fpos++;
            }
        }
        else{
            previous_chain = input_ref[header_reference_pos++]; fpos++;
            printf("%x\n", previous_chain);
            output[output_size++] = previous_chain;
            padded = 0;
            if(chain_size % 2 == 1) {padded = 1;}
            for(int i = 0, bit_side = 0; i < chain_size; i++){
                if(!chain_size) break;
                if(!bit_side) {delta = input_delta[p++]; printf("%x\n", delta); fpos++;}
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
                    if(padded && i + 1 == (chain_size)) {p++; break;}
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
    if(write_file(output, output_size, oyuv)) return 1;
    while(1);
    return 0;
}