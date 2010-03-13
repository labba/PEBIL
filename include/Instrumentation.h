#ifndef _Instrumentation_h_
#define _Instrumentation_h_

#include <Base.h>
#include <Instruction.h>
#include <Vector.h>

class Instruction;
class InstrumentationPoint;

#define OPTIMIZE_NONLEAF
#define SNIPPET_TRAMPOLINE_DEFAULT false

#define PLT_RETURN_OFFSET_32BIT 6
#define PLT_RETURN_OFFSET_64BIT 6

#define Size__32_bit_function_bootstrap 128
#define Size__64_bit_function_bootstrap 128
#define Size__32_bit_procedure_link 16
#define Size__64_bit_procedure_link 16
#define Size__32_bit_function_wrapper 64
#define Size__64_bit_function_wrapper 128

#define Size__trampoline_autoinc 0x4000

#define Size__32_bit_inst_function_call_support 5
#define Size__64_bit_inst_function_call_support 5

#define Size__uncond_jump 5
#define Size__flag_protect_full 2
#define Size__32_bit_flag_protect_light 12
#define Size__64_bit_flag_protect_light 18

extern int compareInstAddress(const void* arg1, const void* arg2);

class InstrumentationPoint;
extern Vector<InstrumentationPoint*>* instpointFilterAddressRange(Base* object, Vector<InstrumentationPoint*>* instPoints);

class Instrumentation : public Base {
protected:
    Vector<Instruction*> bootstrapInstructions;
    uint64_t bootstrapOffset;

    bool distinctTrampoline;
    InstrumentationPoint* point;

public:
    Instrumentation(PebilClassTypes typ);
    ~Instrumentation();

    virtual void print() { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t sizeNeeded() { __SHOULD_NOT_ARRIVE; }
    virtual uint64_t getEntryPoint() { __SHOULD_NOT_ARRIVE; }
    virtual bool verify() { return true; }
    virtual void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset) { __SHOULD_NOT_ARRIVE; } 

    virtual Instruction* removeNextCoreInstruction() { __SHOULD_NOT_ARRIVE; }
    virtual bool hasMoreCoreInstructions() { __SHOULD_NOT_ARRIVE; }

    void setRequiresDistinctTrampoline(bool rdt) { distinctTrampoline = rdt; }
    bool requiresDistinctTrampoline() { return distinctTrampoline; }

    uint32_t bootstrapSize();
    void setBootstrapOffset(uint64_t off) { bootstrapOffset = off; }

    InstrumentationPoint* getInstrumentationPoint() { return point; }
    void setInstrumentationPoint(InstrumentationPoint* pt) { point = pt; }

};

class InstrumentationSnippet : public Instrumentation {
private:
    Vector<Instruction*> snippetInstructions;
    uint64_t snippetOffset;

    uint32_t numberOfDataEntries;
    uint32_t* dataEntrySizes;
    uint64_t* dataEntryOffsets;

public:
    InstrumentationSnippet();
    ~InstrumentationSnippet();    

    void print();
    uint32_t bootstrapSize();
    uint32_t snippetSize();
    uint32_t dataSize();

    void setCodeOffsets(uint64_t btOffset, uint64_t spOffset);
    void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset);

    uint32_t reserveData(uint64_t offset, uint32_t size);

    uint32_t generateSnippetControl();

    uint64_t getEntryPoint();

    uint32_t getNumberOfCoreInstructions() { return snippetInstructions.size(); }
    Instruction* getCoreInstruction(uint32_t idx) { ASSERT(snippetInstructions[idx]); return snippetInstructions[idx]; }
    Instruction* removeNextCoreInstruction() { ASSERT(hasMoreCoreInstructions()); return snippetInstructions.remove(0); }
    bool hasMoreCoreInstructions() { return (snippetInstructions.size() != 0); }

    uint32_t addSnippetInstruction(Instruction* inst);
    void setCodeOffset(uint64_t off) { snippetOffset = off; }
    uint64_t getCodeOffset() { return snippetOffset; }

};

typedef struct {
    uint64_t offset;
} Argument;

class InstrumentationFunction : public Instrumentation {
protected:
    char* functionName;
    uint32_t index;

    bool staticLinked;
    uint64_t functionEntry;

    Vector<Instruction*> procedureLinkInstructions;
    uint64_t procedureLinkOffset;

    Vector<Instruction*> wrapperInstructions;
    uint64_t wrapperOffset;

    uint32_t globalData;
    uint64_t globalDataOffset;

    uint64_t relocationOffset;

    Vector<Argument> arguments;

public:
    InstrumentationFunction(uint32_t idx, char* funcName, uint64_t dataoffset, uint64_t fEntry);
    ~InstrumentationFunction();

    void print();

    bool isStaticLinked() { return (functionEntry != 0); }
    uint64_t getFunctionEntry() { return functionEntry; }

    uint32_t sizeNeeded();
    uint32_t wrapperSize();
    uint32_t procedureLinkSize();
    uint32_t globalDataSize();

    virtual uint32_t bootstrapReservedSize() { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t procedureLinkReservedSize() { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t wrapperReservedSize() { __SHOULD_NOT_ARRIVE; }

    char* getFunctionName() { return functionName; }

    uint32_t getNumberOfProcedureLinkInstructions() { return procedureLinkInstructions.size(); }
    uint32_t getNumberOfBootstrapInstructions() { return bootstrapInstructions.size(); }
    uint32_t getNumberOfWrapperInstructions() { return wrapperInstructions.size(); }
    uint32_t getGlobalData() { return globalData; }
    uint64_t getGlobalDataOffset() { return globalDataOffset; }

    void setRelocationOffset(uint64_t relocOffset) { relocationOffset = relocOffset; }

    Instruction* removeNextCoreInstruction() { ASSERT(hasMoreCoreInstructions()); return wrapperInstructions.remove(0); }
    bool hasMoreCoreInstructions() { return (wrapperInstructions.size() != 0); }

    virtual uint32_t generateProcedureLinkInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress, uint64_t realPLTAddress) { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t generateBootstrapInstructions(uint64_t textbaseAddress, uint64_t dataBaseAddress) { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t generateWrapperInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress) { __SHOULD_NOT_ARRIVE; }
    virtual uint32_t generateGlobalData(uint64_t textBaseAddress) { __SHOULD_NOT_ARRIVE; }

    void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset);

    uint32_t addArgument(uint64_t offset);

    uint64_t getEntryPoint();

    void setWrapperOffset(uint64_t off) { wrapperOffset = off; }
    void setProcedureLinkOffset(uint64_t off) { procedureLinkOffset = off; }
};

class InstrumentationFunction32 : public InstrumentationFunction {
public:
    InstrumentationFunction32(uint32_t idx, char* funcName, uint64_t dataoffset, uint64_t fEntry) : InstrumentationFunction(idx,funcName,dataoffset,fEntry) {}
    ~InstrumentationFunction32() {}

    uint32_t generateProcedureLinkInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress, uint64_t realPLTAddress);
    uint32_t generateBootstrapInstructions(uint64_t textbaseAddress, uint64_t dataBaseAddress);
    uint32_t generateWrapperInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress);
    uint32_t generateGlobalData(uint64_t textBaseAddress);

    uint32_t bootstrapReservedSize() { return Size__32_bit_function_bootstrap; }
    uint32_t procedureLinkReservedSize() { return Size__32_bit_procedure_link; }
    uint32_t wrapperReservedSize() { return Size__32_bit_function_wrapper; }
};

class InstrumentationFunction64 : public InstrumentationFunction {
public:
    InstrumentationFunction64(uint32_t idx, char* funcName,uint64_t dataoffset, uint64_t fEntry) : InstrumentationFunction(idx,funcName,dataoffset,fEntry) {}
    ~InstrumentationFunction64() {}

    uint32_t generateProcedureLinkInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress, uint64_t realPLTAddress);
    uint32_t generateBootstrapInstructions(uint64_t textbaseAddress, uint64_t dataBaseAddress);
    uint32_t generateWrapperInstructions(uint64_t textBaseAddress, uint64_t dataBaseAddress);
    uint32_t generateGlobalData(uint64_t textBaseAddress);

    uint32_t bootstrapReservedSize() { return Size__64_bit_function_bootstrap; }
    uint32_t procedureLinkReservedSize() { return Size__64_bit_procedure_link; }
    uint32_t wrapperReservedSize() { return Size__64_bit_function_wrapper; }
};

typedef enum {
    InstPriority_undefined = 0,
    InstPriority_sysinit,
    InstPriority_userinit,
    InstPriority_regular,
    InstPriority_Total_Types
} InstPriorities;

class InstrumentationPoint : public Base {
protected:
    Instruction* point;
    Instrumentation* instrumentation;

    uint32_t numberOfBytes;
    InstLocations instLocation;

    Vector<Instruction*> trampolineInstructions;
    uint64_t trampolineOffset;

    InstPriorities priority;
    FlagsProtectionMethods protectionMethod;
    InstrumentationModes instrumentationMode;

    Vector<Instruction*> precursorInstructions;
    Vector<Instruction*> postcursorInstructions;

public:

    InstrumentationPoint(Base* pt, Instrumentation* inst, InstrumentationModes instMode, FlagsProtectionMethods flagsMethod, InstLocations loc);
    ~InstrumentationPoint();

    Vector<Instruction*>* swapInstructionsAtPoint(bool isChain, Vector<Instruction*>* replacements);

    void print();
    void dump(BinaryOutputFile* binaryOutputFile, uint32_t offset);

    void setPriority(InstPriorities p) { ASSERT(p && p < InstPriority_Total_Types); priority = p; }
    InstPriorities getPriority() { return priority; }

    uint64_t getTargetOffset() { ASSERT(instrumentation); return instrumentation->getEntryPoint(); }
    Instrumentation* getInstrumentation() { return instrumentation; }

    Instruction* getSourceObject() { return point; }
    PebilClassTypes getPointType() { ASSERT(getSourceObject()); return getSourceObject()->getType(); }
    uint64_t getInstSourceAddress();
    uint64_t getInstBaseAddress();

    uint32_t getNumberOfBytes() { return numberOfBytes; }
    InstrumentationModes getInstrumentationMode() { return instrumentationMode; }
    FlagsProtectionMethods getFlagsProtectionMethod() { return protectionMethod; }

    uint32_t sizeNeeded();
    virtual uint32_t generateTrampoline(Vector<Instruction*>* insts, uint64_t textBaseAddress, uint64_t offset, uint64_t returnOffset, bool doReloc, uint64_t regStorageOffset, bool stackIsSafe)
         { __SHOULD_NOT_ARRIVE; }
    uint64_t getTrampolineOffset() { return trampolineOffset; }

    Instruction* removeNextPrecursorInstruction() { ASSERT(hasMorePrecursorInstructions()); return precursorInstructions.remove(0); }
    bool hasMorePrecursorInstructions() { return (precursorInstructions.size() != 0); }
    uint32_t addPrecursorInstruction(Instruction* inst);
    Instruction* removeNextPostcursorInstruction() { ASSERT(hasMorePostcursorInstructions()); return postcursorInstructions.remove(0); }
    bool hasMorePostcursorInstructions() { return (postcursorInstructions.size() != 0); }

    bool verify();
};

class InstrumentationPoint32 : public InstrumentationPoint {
public:
    InstrumentationPoint32(Base* pt, Instrumentation* inst, InstrumentationModes instMode, FlagsProtectionMethods flagsMethod, InstLocations loc);
    uint32_t generateTrampoline(Vector<Instruction*>* insts, uint64_t textBaseAddress, uint64_t offset, uint64_t returnOffset, bool doReloc, uint64_t regStorageOffset, bool stackIsSafe);
};
class InstrumentationPoint64 : public InstrumentationPoint {
public:
    InstrumentationPoint64(Base* pt, Instrumentation* inst, InstrumentationModes instMode, FlagsProtectionMethods flagsMethod, InstLocations loc);
    uint32_t generateTrampoline(Vector<Instruction*>* insts, uint64_t textBaseAddress, uint64_t offset, uint64_t returnOffset, bool doReloc, uint64_t regStorageOffset, bool stackIsSafe);
};


#endif /* _Instrumentation_h_ */

