#ifndef _ProgramHeader_h_
#define _ProgramHeader_h_

#include <Base.h>
#include <defines/ProgramHeader.d>

class ProgramHeader : public Base {
protected:
    ProgramHeader() : Base(ElfClassTypes_program_header),index(0) {}
    virtual ~ProgramHeader() {}

    uint16_t index;
public:

    PROGRAMHEADER_MACROS_BASIS("For the get_X field macros check the defines directory");

    void print();
    virtual void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset) { __SHOULD_NOT_ARRIVE; }

    const char* briefName() { return "ProgramHeader"; }
    bool verify();
};

class ProgramHeader32 : public ProgramHeader {
protected:
    Elf32_Phdr entry;

public:

    PROGRAMHEADER_MACROS_CLASS("For the get_X field macros check the defines directory");

    ProgramHeader32(uint32_t idx) { sizeInBytes = Size__32_bit_Program_Header; index = idx; }
    ~ProgramHeader32() {}
    uint32_t read(BinaryInputFile* b);

    char* charStream() { return (char*)&entry; }
    void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset);
//    uint32_t instrument(char* buffer,XCoffFileGen* xCoffGen,BaseGen* gen);
};

class ProgramHeader64 : public ProgramHeader {
protected:
    Elf64_Phdr entry;

public:

    PROGRAMHEADER_MACROS_CLASS("For the get_X field macros check the defines directory");

    ProgramHeader64(uint32_t idx) { sizeInBytes = Size__64_bit_Program_Header; index = idx; }
    ~ProgramHeader64() {}
    uint32_t read(BinaryInputFile* b);

    char* charStream() { return (char*)&entry; }
    void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset);
//    uint32_t instrument(char* buffer,XCoffFileGen* xCoffGen,BaseGen* gen);
};


#endif
