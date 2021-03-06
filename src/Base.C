/* 
 * This file is part of the pebil project.
 * 
 * Copyright (c) 2010, University of California Regents
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Base.h>
#include <BasicBlock.h>
#include <ElfFile.h>
#include <ElfFileInst.h>
#include <X86Instruction.h>

FILE* pebilOutp = stdout;
uint64_t warnCount = 0;

FILE* GetTempFile(char** sfn){
    FILE *sfp;
    int fd = -1;

    ASSERT(*sfn == NULL);
    *sfn = new char[__MAX_STRING_SIZE];
    strncpy(*sfn, "/tmp/tmp.XXXXXX\0", __MAX_STRING_SIZE);
    if ((fd = mkstemp(*sfn)) == -1 ||
        (sfp = fdopen(fd, "rw+")) == NULL) {
        PRINT_ERROR("%s: %s", sfn, strerror(errno));
        return (NULL);
    }
    return (sfp);
}

void FileList::appendLine(Vector<char*>* v){
    fileTokens.append(v);
}

void FileList::setSeparator(char c){
    sep = c;
}

void FileList::setFileName(const char* name){
    fname = name;
}

void FileList::init(const char* filename, uint32_t w, char s, char comm){
    ASSERT(filename);

    fname = filename;
    width = w;
    sep = s;

    FILE* inFile = fopen(filename, "r");
    if(!inFile){
        PRINT_ERROR("File cannot be opened [%s]", filename);
    }

    Vector<char*> lines;

    char* inBuffer = new char[__MAX_STRING_SIZE];
    while (fgets(inBuffer, __MAX_STRING_SIZE, inFile) != NULL) {
        char* line = new char[strlen(inBuffer)+1];
        sprintf(line, "%s", inBuffer);
        line[strlen(inBuffer)-1] = '\0';
        if (strlen(line) && line[0] == comm){
            delete[] line;
        } else {
            lines.append(line);
        }
    }
    delete[] inBuffer;
    fclose(inFile);

    for (uint32_t i = 0; i < lines.size(); i++){
        uint32_t linelen = strlen(lines[i]);
        uint32_t toks = 1;
        for (uint32_t c = 0; c < linelen; c++){
            if (lines[i][c] == sep){
                toks++;
            }
        }

        if (width && toks != width){
            PRINT_ERROR("Input file line has incorrect number of '%c'-seperated tokens: %s", sep, lines[i]);
        }

        Vector<char*>* tokens = new Vector<char*>();

        uint32_t beg = 0;
        for (uint32_t c = 0; c < linelen; c++){
            if (lines[i][c] == sep){
                lines[i][c] = '\0';

                char* tokbeg = &(lines[i])[beg];
                char* newtok = new char[strlen(tokbeg) + 1];
                bzero(newtok, strlen(tokbeg) + 1);
                memcpy(newtok, tokbeg, strlen(tokbeg));

                tokens->append(newtok);

                beg = c + 1;
            }
        }

        char* tokbeg = &(lines[i])[beg];
        char* newtok = new char[strlen(tokbeg) + 1];
        bzero(newtok, strlen(tokbeg) + 1);
        memcpy(newtok, tokbeg, strlen(tokbeg));

        tokens->append(newtok);

        fileTokens.append(tokens);
        delete[] lines[i];
    }

    verify();
}

bool FileList::verify(){
    regex_t regex;
    int err;
    char err_msg[__MAX_STRING_SIZE];
    
    for (uint32_t i = 0; i < fileTokens.size(); i++){
        for (uint32_t j = 0; j < fileTokens[i]->size(); j++){
            if ((err = regcomp(&regex, getToken(i, j), REG_EXTENDED)) != 0){
                regerror(err, &regex, err_msg, __MAX_STRING_SIZE);
                PRINT_ERROR("Error analyzing regular expression '%s': %s.\n", getToken(i, j), err_msg);
                return false;
            }
            regfree(&regex);
        }
    }

    return true;
}

FileList::FileList(const char* filename, uint32_t w, char sep){
    init(filename, w, sep, '#');
}

FileList::FileList(const char* filename){
    init(filename, 1, ' ', '#');
}

FileList::~FileList(){
    for (uint32_t i = 0; i < fileTokens.size(); i++){
        for (uint32_t j = 0; j < fileTokens[i]->size(); j++){
            delete[] getToken(i, j);
        }
        delete fileTokens[i];
    }
}

bool regexMatch(char *string, char* reg){
    regmatch_t match;
    regex_t regex;
    int err;
    char err_msg[__MAX_STRING_SIZE];

    if ((err = regcomp(&regex, reg, REG_EXTENDED)) != 0){
        regerror(err, &regex, err_msg, __MAX_STRING_SIZE);
        PRINT_ERROR("Error analyzing regular expression '%s': %s.\n", reg, err_msg);
    }

    if ((regexec(&regex, string, 0, NULL, 0)) == 0){
        regfree(&regex);
        return true;
    }
    regfree(&regex);
    return false;
}

bool FileList::matches(char* str, uint32_t tok){
    for (uint32_t i = 0; i < fileTokens.size(); i++){
        if (regexMatch(str, getToken(i, tok))){
            return true;
        }
    }
    return false;
}

char* FileList::getToken(uint32_t idx, uint32_t tok){
    ASSERT(idx < fileTokens.size());
    ASSERT(!width || tok < width);

    return (*fileTokens[idx])[tok];
}

void FileList::print(){
    PRINT_INFOR("Processed contents of file %s:", fname);
    for (uint32_t i = 0; i < fileTokens.size(); i++){
        PRINT_INFO();
        PRINT_OUT("\t");
        uint32_t s = fileTokens[i]->size();
        for (uint32_t j = 0; j < s; j++){
            PRINT_OUT("%s", getToken(i, j));
            if (j != s - 1){
                PRINT_OUT("%c", sep);
            }
        }
        PRINT_OUT("\n");
    }
}

#define SHA1SUM_BYTES 20
char* sha1sum(char* buffer, uint32_t size, uint64_t* first64){
    unsigned char* allbytes = new unsigned char[size];
    int end = size;
    char *line;
    char* hexstring = new char[2*SHA1SUM_BYTES + 1];
    unsigned char hash[SHA1SUM_BYTES];

    bzero(hexstring, 2*SHA1SUM_BYTES + 1);
    bzero(hash, SHA1SUM_BYTES);
    memcpy(allbytes, buffer, size);

    calc(allbytes, end, hash);
    toHexString(hash, hexstring);

    delete[] allbytes;                                                                                               

    // pick out the first 64 bits for use as a unique id
    if (first64 != NULL){
        uint64_t tmp = 0;

        uint32_t s = sizeof(uint64_t) * 2;
        for (uint32_t i = 0; i < s; i += 2){
            ASSERT(i < SHA1SUM_BYTES);

            char v1 = getHexValue(hexstring[i + 1]);
            char v2 = getHexValue(hexstring[i]);
            char v = v1 | (v2 << 4);

            memcpy(&((char*)(&tmp))[(s-i-2)/2], &v, 1);
        }
        *first64 = tmp;
    }
                                  
    return hexstring;
}

bool allSpace(char* str){
    int32_t len = strlen(str);
    for(int32_t i=0;i<len;i++){
        switch(str[i]){
        case ' ' :
        case '\n':
        case '\t':
            break;
        default:
            return false;
        }
    }
    return true;
}


uint32_t searchFileList(Vector<char*>* list, char* name){
    if (!list) return 1;

    for (uint32_t i = 0; i < (*list).size(); i++){
        if (!strcmp((*list)[i], name)){
            return i;
        }
    }
    return (*list).size();
}

uint32_t initializeFileList(char* fileName, Vector<char*>* list){
    ASSERT(!(*list).size());

    FILE* inFile = NULL;
    inFile = fopen(fileName, "r");
    if(!inFile){
        PRINT_ERROR("Input file can not be opened [%s]", fileName);
    }

    char* inBuffer = new char[__MAX_STRING_SIZE];
    while (fgets(inBuffer, __MAX_STRING_SIZE, inFile) != NULL) {
        char* line = new char[strlen(inBuffer)+1];
        sprintf(line, "%s", inBuffer);
        line[strlen(inBuffer)-1] = '\0';
        if (strlen(line) && line[0] == '#'){
            delete[] line;
        } else {
            (*list).append(line);
        }
    }
    delete[] inBuffer;
    fclose(inFile);

    return (*list).size();
}

void printBufferPretty(char* buff, uint32_t sizeInBytes, uint64_t baseAddress, uint32_t bytesPerWord, uint32_t bytesPerLine){
    if (bytesPerWord <= 0){
        bytesPerWord = 8;
    }
    if (bytesPerLine <= 0){
        bytesPerLine = 64;
    }
    
    uint32_t currByte = 0;
    
    for (currByte = 0; currByte < sizeInBytes; currByte++){
        if (currByte % bytesPerLine == 0){
            if (currByte){
                fprintf(stdout, "\n");
            }
            fprintf(stdout, "(%16llx) %8x: ", baseAddress + currByte, currByte);
        } else if (currByte && currByte % bytesPerWord == 0){
            fprintf(stdout, " ");
        }
        char* currBuff = (char*)(buff + currByte);
        fprintf(stdout, "%02hhx", *currBuff);
    }
    fprintf(stdout, "\n");
}

bool isHexNumeral(char c1){
    if (c1 >= '0' && c1 <= '9'){
        return true;
    }
    return false;
}

bool isHexUpper(char c1){
    if (c1 >= 'A' && c1 <= 'F'){
        return true;
    }
    return false;
}

bool isHexLower(char c1){
    if (c1 >= 'a' && c1 <= 'f'){
        return true;
    }
    return false;
}

bool isHexDigit(char c1){
    if (isHexNumeral(c1) || isHexUpper(c1) || isHexLower(c1)){
        return true;
    }
    return false;
}

char getHexValue(char c1){
    ASSERT(isHexDigit(c1));
    if (isHexNumeral(c1)){
        return c1-'0';
    } else if (isHexUpper(c1)){
        return c1-'A'+0xa;
    } else if (isHexLower(c1)){
        return c1-'a'+0xa;
    }
    __SHOULD_NOT_ARRIVE;
}

char mapCharsToByte(char c1, char c2){
    ASSERT(isHexDigit(c1) && isHexDigit(c2));
    return (getHexValue(c1) << 4) + getHexValue(c2);
}


int64_t absoluteValue(uint64_t d){
    int64_t a = (int64_t)d;
    if (a < 0){
        a = a * -1;
    }
    return a;
}

uint64_t getUInt64(char* buf){
    uint64_t data;
    memcpy(&data,buf,sizeof(uint64_t));
    return data;
}

uint32_t getUInt32(char* buf){
    uint32_t data;
    memcpy(&data,buf,sizeof(uint32_t));
    return data;
}

uint16_t getUInt16(char* buf){
    uint16_t data;
    memcpy(&data,buf,sizeof(uint16_t));
    return data;
}

int compareHashCode(const void* arg1, const void* arg2){
    uint64_t vl1 = (*((HashCode**)arg1))->getValue();
    uint64_t vl2 = (*((HashCode**)arg2))->getValue();

    if      (vl1 < vl2) return -1;
    else if (vl1 > vl2) return  1;
    else                return  0;
    return 0;
}

// this function needs to be fast; it gets called -->*A LOT*<--
int compareBaseAddress(const void* arg1, const void* arg2){
    register uint64_t vl1 = (*((Base**)arg1))->baseAddress;
    register uint64_t vl2 = (*((Base**)arg2))->baseAddress;
    return vl1 - vl2;
}

int searchBaseAddressExact(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    Base* b = *((Base**)arg2);

    ASSERT(b && "Base should exist");

    uint64_t val = b->baseAddress;

    if (key < val)
        return -1;
    if (key > val)
        return 1;
    return 0;
}

int searchHashCode(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    HashCode* h = *((HashCode**)arg2);

    ASSERT(h && "HashCode should exist");

    uint64_t val = h->getValue();

    if (key < val)
        return -1;
    if (key > val)
        return 1;
    return 0;
}

int searchBasicBlockAddress(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    BasicBlock* bb = *((BasicBlock**)arg2);

    ASSERT(bb && "BB should exist");

    uint64_t val_low = bb->baseAddress;
    uint64_t val_high = val_low + bb->getNumberOfBytes();

    if (key < val_low)
        return -1;
    if (key >= val_high)
        return 1;
    return 0;
}

int searchBaseAddress(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    Base* b = *((Base**)arg2);

    ASSERT(b && "Base should exist");

    uint64_t val_low = b->baseAddress;
    uint64_t val_high = val_low + b->getSizeInBytes();

    if (key < val_low)
        return -1;
    if (key >= val_high)
        return 1;
    return 0;
}


extern uint32_t logBase2(uint32_t n){
    ASSERT(n); 
    n >>= 1;
    uint32_t l = 0;
    while (n){
        n >>= 1;
        l++;
    }
    return l;
}

extern bool isPowerOfTwo(uint32_t n){
    uint32_t currVal = 1;
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; i++){
        if (n == currVal) return true;
        currVal *= 2;
    }
    return false;
}



uint32_t align_mask_or[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};

uint32_t align_mask_and[32] = {
    0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
    0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
    0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
    0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
    0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
    0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
    0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
    0xf0000000, 0xe0000000, 0xc0000000, 0x80000000
};

extern uint64_t nextAlignAddress(uint64_t addr, uint32_t align){
    if (align == 0 || align == 1 || addr == 0){
        return addr;
    }
    if (align > addr){
        return align;
    }
    ASSERT(isPowerOfTwo(align) && "alignment must be a power of 2");
    int32_t pow = -1;
    uint32_t tmp = align;
    while (tmp > 0){
        pow++;
        tmp = tmp >> 1;
    }
    uint64_t aligned = addr & align_mask_and[pow];
    if (aligned < addr){
        aligned = aligned + align_mask_or[pow];
    }

    /*
    uint64_t ans2 = addr;
    while (ans2 % align){
        ans2++;
    }
    PRINT_INFOR("Alignment: align(%llx,%x)=%llx/%llx", addr, align, aligned, ans2);
    PRINT_INFOR("Masks align: and[%d]=0x%x, or[%d]=0x%x", pow, align_mask_and[pow], pow, align_mask_or[pow]);
    ASSERT(aligned == ans2);
    */

    return aligned;
}


extern uint64_t nextAlignAddressHalfWord(uint64_t addr){
    //    return nextAlignAddress(addr,sizeof(uint16_t));
    addr += (addr % 2 ? 2 : 0);
    addr >>= 1;
    addr <<= 1;
    return addr;
}
extern uint64_t nextAlignAddressWord(uint64_t addr){
    //    return nextAlignAddress(addr,sizeof(uint32_t));
    addr += (addr % 4 ? 4 : 0);
    addr >>= 2;
    addr <<= 2;
    return addr;
}
extern uint64_t nextAlignAddressDouble(uint64_t addr){
    //    return nextAlignAddress(addr,sizeof(uint64_t));
    addr += (addr % 8 ? 8 : 0);
    addr >>= 3;
    addr <<= 3;
    return addr;
}


HashCode::HashCode(uint32_t s){
    entry.bits = INVALID_FIELD;
    if(validSection(s)){
        entry.fields.section     = ++s;
        entry.fields.function    = INVALID_FIELD;
        entry.fields.block       = INVALID_FIELD;
        entry.fields.instruction = INVALID_FIELD;
    }
}

HashCode::HashCode(uint32_t s,uint32_t f){
    entry.bits = INVALID_FIELD;
    if(validSection(s) && validFunction(f)){
        entry.fields.section     = ++s;
        entry.fields.function    = ++f;
        entry.fields.block       = INVALID_FIELD;
        entry.fields.instruction = INVALID_FIELD;
    }
}

HashCode::HashCode(uint32_t s,uint32_t f,uint32_t b){
    entry.bits = INVALID_FIELD;
    if(validSection(s) && validFunction(f) && validBlock(b)){
        entry.fields.section     = ++s;
        entry.fields.function    = ++f;
        entry.fields.block       = ++b;
        entry.fields.instruction = INVALID_FIELD;
    }
}

HashCode::HashCode(uint32_t s,uint32_t f,uint32_t b,uint32_t m){
    entry.bits = INVALID_FIELD;
    if(validSection(s) && validFunction(f) && validBlock(b) && validInstruction(m)){
        entry.fields.section     = ++s;
        entry.fields.function    = ++f;
        entry.fields.block       = ++b;
        entry.fields.instruction = ++m;
    }
}


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

double timer()
{
    struct timeval timestr;
    struct timezone* tzp = 0;
    gettimeofday(&timestr, tzp);
    return (double)timestr.tv_sec + 1.0E-06*(double)timestr.tv_usec;
}

#include <CheckSum.h>

