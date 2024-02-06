struct qct_header{
    char magic[4];
    unsigned int version;
    unsigned int file_size;
//                        encode RLE:         uniform quantization:
//                        0 - yes 1 - no      01 - (none) 10 - (by 2) 11 - (by 3) 
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
///////////////////// only support yuv420 currently //////////////////////////////
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
char* encode_qct(char* yuv, short width, short height, int* size){
    int bit_count = 0, size_in = 0, yuv_size = (width * height) + (((width * height) / 4) * 2);
    unsigned char* out = yuv;
    unsigned char* in = alloc_mem(yuv_size),
                 * head = alloc_mem(yuv_size),
                 * clasps = alloc_mem(yuv_size), *file_output;
    unsigned char previous_chain, chain_size, first_chain, clasp, chain[256], *input_data = out, *output_data = in, bit_side = 0, 
    pre = 0, ppos = 0, _3, sign_bits[16], n_bit, *uncompressed_size;
    size_in = yuv_size;
    unsigned int pos_input = 0, input_size = size_in, output_size = 0, com1, com2, p, sign_bits_size = 0;
    int head_s = 0, i, clasp_s = 0;
    struct qct_header header = {0};
    header.width = width;
    header.height = height;
    header.magic[0] = 'q', header.magic[1] = 'c', header.magic[2] = 't', header.magic[3] = ' '; 
    header.version = 0;
    header.file_size = yuv_size;
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
    for(i = 0; i < sizeof(struct qct_header); i++){
        file_output[i] = header_ptr[i];
    }
    *size = sizeof(struct qct_header) + size_in;
    return file_output;
}

char* decode_qct(char* buffer, short* width, short* height, int size, int* osize){
    if(!buffer) return NULL;
    struct qct_header header;
    unsigned char* in = buffer, *out, *header_ptr = (unsigned char*)&header, *yuv_;
    for(int i = 0; i < sizeof(struct qct_header); i++){
        header_ptr[i] = in[i];
    }
    in+=sizeof(struct qct_header);
    out = alloc_mem(header.file_size);
    unsigned char previous_chain, chain_size, com1, com2, delta, padded;
    unsigned int p = 0, output_size = 0, bit_side = 0, size_in = size, header_pos = 0, header_reference_pos = 0, fpos = 0; 
    char* output, *input, *input_header, *input_ref, * input_delta; 
    *width = header.width;
    *height = header.height;
    if((header.flag & 0x0100)){
        output = out, input = in;
        size_in = size - sizeof(struct qct_header);
    }
    else{
        yuv_ = alloc_mem(header.file_size);
        decode_rle(in, out, &size_in, size - sizeof(struct qct_header));
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
    *osize = output_size;
    return output;
}
