#include "Test.h"
#include "ToggleSprint/Profile.h"
#include <cstring>
#include <windows.h>
using namespace sprint;
int wmain(int argc,wchar_t** argv){
    // Breaks caught: accepting malformed PE before bounds, wrong identity, unsafe sections/guards.
    std::vector<unsigned char> b(0x400);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b.data());dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(b.data()+0x80);
    nt->Signature=IMAGE_NT_SIGNATURE;nt->FileHeader.Machine=0x8664;nt->FileHeader.TimeDateStamp=123;
    nt->FileHeader.NumberOfSections=1;nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
    nt->OptionalHeader.Magic=0x20b;nt->OptionalHeader.SizeOfImage=0x2000;nt->OptionalHeader.SizeOfHeaders=0x200;
    auto s=IMAGE_FIRST_SECTION(nt);memcpy(s->Name,".text",5);s->VirtualAddress=0x1000;s->Misc.VirtualSize=0x200;
    s->SizeOfRawData=0x200;s->PointerToRawData=0x200;s->Characteristics=0x60000020;
    for(unsigned i=0;i<32;++i)b[0x200+i]=static_cast<unsigned char>(i);
    // Expected fixture digest is calculated outside the validator: identity is not a PE label.
    auto digest=Sha256(b);
    Profile p{"fixture",{digest.c_str(),digest.c_str()},123,0x2000,0x2000,0x1000,0x1020,{{0x1000,0x1020,{}}}};
    for(unsigned i=0;i<32;++i)p.guards[0].bytes[i]=static_cast<unsigned char>(i);
    std::string error;
    Check(Sha256({})=="E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855","SHA256 independent empty vector");
    Check(ValidateImage(b,p,error),"well-formed fixture accepted");
    auto reject=[&](auto change,char const* label){auto copy=b;change(copy);auto fixture=p;auto hash=Sha256(copy);fixture.hashes={hash.c_str(),hash.c_str()};Check(!ValidateImage(copy,fixture,error),label);};
    reject([](auto& v){v.resize(63);},"truncated DOS header");
    reject([](auto& v){v[0]=0;},"invalid signature");
    reject([](auto& v){reinterpret_cast<IMAGE_DOS_HEADER*>(v.data())->e_lfanew=INT32_MAX;},"overflow NT offset");
    reject([](auto& v){reinterpret_cast<IMAGE_NT_HEADERS64*>(v.data()+0x80)->FileHeader.Machine=0x14c;},"x86 refused");
    reject([](auto& v){v[0x200]^=1;},"changed guard and identity");
    auto wrong=p;wrong.hashes={"bad","bad"};Check(!ValidateImage(b,wrong,error),"wrong full hash");
    wrong=p;wrong.imageSize=0x3000;wrong.launcherSize=0x3000;Check(!ValidateImage(b,wrong,error),"wrong image size");
    wrong=p;wrong.guards[0].rva=0xffffffff;Check(!ValidateImage(b,wrong,error),"overflow guard");
    wrong=p;wrong.guards[0].bytes[0]=1;Check(!ValidateImage(b,wrong,error),"guard separately checked");
    wrong=p;wrong.hookEnd=0x2001;Check(!ValidateImage(b,wrong,error),"hook must fit executable range");
    reject([](auto& v){auto n=reinterpret_cast<IMAGE_NT_HEADERS64*>(v.data()+0x80);IMAGE_FIRST_SECTION(n)->Characteristics=0x40000040;},"non executable target");
    reject([](auto& v){auto n=reinterpret_cast<IMAGE_NT_HEADERS64*>(v.data()+0x80);IMAGE_FIRST_SECTION(n)->PointerToRawData=0xfffffff0;},"truncated section");
    std::uintptr_t base{};Check(!ValidateHost(ExactProfile(),base,error),"test process is unsupported host");
    // Break caught: unrelated plugins detouring layout evidence must not disable
    // sprint; changed real hook targets and runtime dependencies must still refuse.
    auto const& exact=ExactProfile();
    auto mapped=static_cast<unsigned char*>(VirtualAlloc(nullptr,exact.launcherSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(mapped!=nullptr,"mapped exact-profile fixture allocated");
    if(mapped){
        memcpy(mapped,b.data(),0x200);
        auto header=reinterpret_cast<IMAGE_NT_HEADERS64*>(mapped+0x80);
        header->FileHeader.TimeDateStamp=exact.timestamp;
        header->OptionalHeader.SizeOfImage=exact.launcherSize;
        auto section=IMAGE_FIRST_SECTION(header);
        section->Misc.VirtualSize=section->SizeOfRawData=exact.launcherSize-0x1000;
        for(auto const& guard:exact.guards)memcpy(mapped+guard.rva,guard.bytes.data(),guard.bytes.size());
        std::span<unsigned char const> loaded{mapped,exact.launcherSize};
        Check(ValidateLoadedImage(loaded,exact,error),"unmodified loaded profile accepted");
        mapped[0x476DB90]=0xe9; // AutoJumpReset hooks this analysis-only event.
        Check(ValidateLoadedImage(loaded,exact,error),"analysis-only event detour does not disable sprint");
        for(auto const& guard:exact.guards){
            if(guard.rva==0x476DB90)continue;
            mapped[guard.rva]^=1;
            Check(!ValidateLoadedImage(loaded,exact,error),"actual runtime guard detour is refused");
            char address[16]{};std::snprintf(address,sizeof address,"0x%08X",static_cast<unsigned>(guard.rva));
            Check(error.find(address)!=std::string::npos,"loaded conflict reports exact failing RVA");
            mapped[guard.rva]^=1;
        }
        auto broken=exact;broken.guards[0].validation=static_cast<GuardValidation>(999);
        Check(!ValidateLoadedImage(loaded,broken,error),"unknown guard role refused");
        Check(!ValidateLoadedImage(loaded.first(0x200),exact,error),"truncated loaded image refused");
        header->OptionalHeader.Magic=0x10b;
        Check(!ValidateLoadedImage(loaded,exact,error),"loaded x86 optional header refused");
        header->OptionalHeader.Magic=0x20b;
        section->Characteristics=0x40000040;
        Check(!ValidateLoadedImage(loaded,exact,error),"loaded nonexecutable target refused");
        VirtualFree(mapped,0,MEM_RELEASE);
    }
    auto evidence=p;evidence.guards.push_back({0x1040,0x1060,{},GuardValidation::DiskEvidence});
    Check(ValidateImage(b,evidence,error),"disk-only evidence still validates immutable image");
    auto changed=b;changed[0x240]=0xe9;auto changedHash=Sha256(changed);
    evidence.hashes={changedHash.c_str(),changedHash.c_str()};
    Check(!ValidateImage(changed,evidence,error),"disk-only evidence byte changes refused even with matching SHA");
    for(int index=1;index<argc;++index){
        auto file=CreateFileW(argv[index],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        Check(file!=INVALID_HANDLE_VALUE,"real image opened read-only");
        if(file!=INVALID_HANDLE_VALUE){
            LARGE_INTEGER size{};GetFileSizeEx(file,&size);auto mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY,0,0,nullptr);
            auto view=mapping?MapViewOfFile(mapping,FILE_MAP_READ,0,0,0):nullptr;
            Check(view && ValidateImage({static_cast<unsigned char const*>(view),static_cast<std::size_t>(size.QuadPart)},ExactProfile(),error),"real supported file accepted by runtime validator");
            if(!error.empty())std::printf("Profile detail: %s\n",error.c_str());
            // SEC_IMAGE maps real section RVAs without starting a process or
            // calling game code. Catch disk/raw assumptions in live validation.
            auto loadedMapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE,0,0,nullptr);
            auto loadedView=loadedMapping?MapViewOfFile(loadedMapping,FILE_MAP_READ,0,0,0):nullptr;
            Check(loadedView!=nullptr,"real image mapped by Windows without execution");
            if(loadedView){
                auto first=static_cast<unsigned char const*>(loadedView);
                auto d=reinterpret_cast<IMAGE_DOS_HEADER const*>(first);
                auto n=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(first+d->e_lfanew);
                Check(ValidateLoadedImage({first,n->OptionalHeader.SizeOfImage},ExactProfile(),error),"real loaded section guards accepted");
                if(!error.empty())std::printf("Loaded profile detail: %s\n",error.c_str());
                UnmapViewOfFile(loadedView);
            }
            if(loadedMapping)CloseHandle(loadedMapping);
            if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);CloseHandle(file);
        }
    }
    return Finish("Profile");
}
