typedef struct user_file{
    char name[80];
    unsigned long long file_size, offset;
    char* pos_handeler;
} File;

struct dir_c{
    char location[80];
};

struct file_c{
    char filename[80];
    unsigned long long file_size;  
};

typedef struct user_dir{
    char location[80];
    int num_dir;
    int num_file;
    struct dir_c u_d[1024];
    struct file_c u_f[1024];
} Dir;

int handeler_file = 0;

typedef enum{
    READ,
    WRITE,
    CHECK
} Permission;

typedef enum{
    BEG,
    END,
    POS,
} offsets;

#ifdef _WIN32
#include <stdio.h>
    #include <windows.h>
#endif 
#ifdef __linux__
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif
char write_file(char* data, long unsigned int size, File* file){
    #ifdef _WIN32
        long unsigned int _tmp_wrote = 0;
            if(WriteFile(*(HANDLE*)file->pos_handeler, data, size, &_tmp_wrote, NULL) == FALSE) return 0x01;
            while(_tmp_wrote != size) printf("%d\n", _tmp_wrote);
            file->offset+=size;
    #endif
    #ifdef __linux__
        if(write(*(int*)file->pos_handeler, data, size) != size) return 0x01;
        file->offset+=size;
    #endif
    return 0x00;
}

char read_file(char* data, long unsigned int size, File* file){
    #ifdef _WIN32
        long unsigned int _tmp_read = 0;
            if(ReadFile(*(HANDLE*)file->pos_handeler, data, size, &_tmp_read, NULL) == FALSE) return 0x01;
            while(_tmp_read != size);
            file->offset+=size;
    #endif
    #ifdef __linux__
        if(read(*(int*)file->pos_handeler, data, size) != size) return 0x01;
        file->offset+=size;
    #endif
    return 0x00;
}

char seek_file(File* file, unsigned long long offset, offsets off){
    #ifdef _WIN32
        unsigned long long n_offset = offset;
        long high_pos = (long)(n_offset << 32), low_pos = (long)(n_offset), low_ret = 0;
        low_ret = SetFilePointer(*(HANDLE*)file->pos_handeler, low_pos, &high_pos, (off == BEG) ? FILE_BEGIN : (off == END) ? FILE_END : FILE_CURRENT);
        if(low_ret == INVALID_SET_FILE_POINTER) return 0x01;    
        long* n_offset_ = (long*)&n_offset;
        *(long*)n_offset_ = low_ret; n_offset_+=4;
        *(long*)n_offset_ = high_pos;
        file->offset = n_offset;
    #endif
    #ifdef __linux__
        int s = lseek(*(int*)file->pos_handeler, offset, (off == BEG) ? SEEK_SET : (off == END) ? SEEK_END : SEEK_CUR);
        if(s < 0) return 0x01;
        file->offset = s;
    #endif
    return 0x00;
}


File* open_file(char* filename, Permission perm){
    #ifdef _WIN32
        File* file = (File*)alloc_mem(sizeof(File));
        file->pos_handeler = (char*)alloc_mem(sizeof(HANDLE));
        if(perm == READ){
            *(HANDLE*)file->pos_handeler = CreateFile( filename, 
                                                    GENERIC_READ, 
                                                    FILE_SHARE_READ, 
                                                    NULL,  
                                                    OPEN_EXISTING, 
                                                    FILE_ATTRIBUTE_NORMAL, 
                                                    NULL);
            if(*(HANDLE*)file->pos_handeler == INVALID_HANDLE_VALUE)  return NULL;
            if(seek_file(file, 0, END)) return NULL;
            file->file_size = file->offset;
            if(seek_file(file, 0, BEG)) return NULL;
        }
        else if(perm == WRITE){
            *(HANDLE*)file->pos_handeler = CreateFile( filename,
                                                    GENERIC_WRITE,
                                                    FILE_SHARE_READ,
                                                    NULL,                   
                                                    CREATE_ALWAYS,             
                                                    FILE_ATTRIBUTE_NORMAL,  
                                                    NULL);
            if(*(HANDLE*)file->pos_handeler == INVALID_HANDLE_VALUE) return NULL;
        }
        else{
            HANDLE hFile_s = CreateFile( filename, 
                                         GENERIC_READ,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, 
                                         NULL);
            if(hFile_s == INVALID_HANDLE_VALUE)  return NULL;
        }
    #endif
    #ifdef __linux__
        File* file = alloc_mem(sizeof(File));
        file->pos_handeler = alloc_mem(4);
        if(perm == READ || perm == CHECK) *(int*)file->pos_handeler = open(filename, O_RDONLY);
        if(perm == WRITE) *(int*)file->pos_handeler = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if(*(int*)file->pos_handeler < 0) return NULL;
        if(perm == READ){
            if(seek_file(file, 0, END)) return NULL;
            file->file_size = file->offset;
            if(seek_file(file, 0, BEG)) return NULL;
        }
    #endif
    int i = 0;
    for(;; i++){
        if(filename[i] == 0x00) break;
        file->name[i] = filename[i]; 
    }
    file->name[i] = 0x00;
    if(perm != CHECK) handeler_file++;
    return file;
}

char open_dir(char* dir_set, Dir* dir){
    #ifdef _WIN32
        WIN32_FIND_DATA ffd;
        char dir_[MAX_PATH];
        int i = 0;
        for(;dir_set[i] != 0x00; i++){
            dir_[i] = dir_set[i];
        }
        dir_[i] = '\\'; dir_[i + 1] = '*'; 
        dir_[i + 2] = 0x00;
        printf("%s\n", dir_);
        HANDLE hFind = FindFirstFile(dir_, &ffd);
        if(hFind == INVALID_HANDLE_VALUE) return 0x01;
        int c_d = 0, c_f = 0;
        do{
            if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                i = 0;
                for(;ffd.cFileName[i] != 0x00; i++) dir->u_d[c_d].location[i] = ffd.cFileName[i];
                dir->u_d[c_d].location[i] = 0x00;
                c_d++;
            }
            else{
                long low_size = ffd.nFileSizeLow, high_size = ffd.nFileSizeHigh;
                long* pSize = (long*)&dir->u_f[c_f].file_size;
                *(long*)pSize = low_size; pSize+=4;
                *(long*)pSize = high_size;
                i = 0;
                for(;ffd.cFileName[i] != 0x00; i++) dir->u_f[c_f].filename[i] = ffd.cFileName[i];
                dir->u_f[c_f].filename[i] = 0x00;
                c_f++;
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
        dir->num_dir = c_d; dir->num_file = c_f;
    #endif
    return 0x00;
}
void file_close(File* file){
    #ifdef _WIN32
        free_mem(file->pos_handeler, sizeof(HANDLE));
        CloseHandle(*(HANDLE*)file->pos_handeler);
    #endif
    #ifdef __linux__
        free_mem(file->pos_handeler, 4);
        close(*(int*)file->pos_handeler);
    #endif 
    free_mem(file, sizeof(File));
}