#include "ToggleSprint/Profile.h"
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#include <algorithm>
#include <cstdio>
namespace sprint {
namespace {
#include "NativeProfile.inl"
bool Fits(std::size_t start,std::size_t n,std::size_t total){return start<=total && n<=total-start;}
struct Pe {IMAGE_NT_HEADERS64 const* nt{};std::span<IMAGE_SECTION_HEADER const> sections;};
bool Parse(std::span<unsigned char const> b,Pe& pe){
    if(b.size()<sizeof(IMAGE_DOS_HEADER))return false;
    auto d=reinterpret_cast<IMAGE_DOS_HEADER const*>(b.data());
    if(d->e_magic!=IMAGE_DOS_SIGNATURE || d->e_lfanew<0 || !Fits(d->e_lfanew,sizeof(IMAGE_NT_HEADERS64),b.size()))return false;
    auto n=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(b.data()+d->e_lfanew);
    if(n->Signature!=IMAGE_NT_SIGNATURE || n->FileHeader.Machine!=0x8664 || n->OptionalHeader.Magic!=0x20b ||
       n->FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64) || !n->FileHeader.NumberOfSections || n->FileHeader.NumberOfSections>96)return false;
    auto offset=static_cast<std::size_t>(d->e_lfanew)+sizeof(IMAGE_NT_HEADERS64);
    if(!Fits(offset,n->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER),b.size()))return false;
    pe={n,{reinterpret_cast<IMAGE_SECTION_HEADER const*>(b.data()+offset),n->FileHeader.NumberOfSections}};
    for(auto const& s:pe.sections){
        if(!Fits(s.PointerToRawData,s.SizeOfRawData,b.size()) ||
           !Fits(s.VirtualAddress,std::max(s.Misc.VirtualSize,s.SizeOfRawData),n->OptionalHeader.SizeOfImage))return false;
    }
    return true;
}
IMAGE_SECTION_HEADER const* Executable(Pe const& p,std::uint32_t rva,std::uint32_t end){
    if(end<=rva)return nullptr;
    for(auto const& s:p.sections)
        if((s.Characteristics&IMAGE_SCN_MEM_EXECUTE) && rva>=s.VirtualAddress &&
            Fits(rva-s.VirtualAddress,end-rva,s.Misc.VirtualSize) && Fits(rva-s.VirtualAddress,end-rva,s.SizeOfRawData))return &s;
    return nullptr;
}
std::string GuardError(char const* message,std::uint32_t rva){
    char address[16]{};std::snprintf(address,sizeof address,"0x%08X",static_cast<unsigned>(rva));
    return std::string(message)+" RVA="+address;
}
bool LoadedGuards(std::uintptr_t base,Profile const& p,std::string& error) {
    __try {
        auto d=reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
        if(d->e_magic!=IMAGE_DOS_SIGNATURE || d->e_lfanew<=0 || d->e_lfanew>0x100000){error="Invalid loaded DOS header";return false;}
        auto n=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(base+d->e_lfanew);
        if(n->Signature!=IMAGE_NT_SIGNATURE || n->FileHeader.Machine!=0x8664 || n->FileHeader.TimeDateStamp!=p.timestamp ||
            (n->OptionalHeader.SizeOfImage!=p.imageSize && n->OptionalHeader.SizeOfImage!=p.launcherSize)){error="Unsupported loaded image metadata";return false;}
        return ValidateLoadedImage({reinterpret_cast<unsigned char const*>(base),n->OptionalHeader.SizeOfImage},p,error);
    }__except(EXCEPTION_EXECUTE_HANDLER){error="Unreadable loaded image guard";return false;}
}
}
Profile const& ExactProfile(){static Profile p=MakeExactProfile();return p;}
bool ValidateLoadedImage(std::span<unsigned char const> b,Profile const& p,std::string& error){
    Pe pe;
    if(!Parse(b,pe)){error="Invalid loaded AMD64 PE bounds";return false;}
    auto n=pe.nt;
    if(n->FileHeader.TimeDateStamp!=p.timestamp || (n->OptionalHeader.SizeOfImage!=p.imageSize && n->OptionalHeader.SizeOfImage!=p.launcherSize) ||
       b.size()!=n->OptionalHeader.SizeOfImage){error="Unsupported loaded image metadata";return false;}
    if(!Executable(pe,p.hookRva,p.hookEnd) || p.guards.empty()){error="Unbounded loaded executable target";return false;}
    for(auto const& g:p.guards){
        if(!Executable(pe,g.rva,g.end) || g.end-g.rva<g.bytes.size() || !Fits(g.rva,g.bytes.size(),b.size()) ||
           (g.validation!=GuardValidation::Runtime && g.validation!=GuardValidation::DiskEvidence)){
            error=GuardError("Invalid loaded guard range or role",g.rva);return false;
        }
        // Disk-only rows prove layouts during analysis, but are never called or
        // hooked by this plugin. The immutable file still checks every row.
        if(g.validation==GuardValidation::Runtime && memcmp(b.data()+g.rva,g.bytes.data(),g.bytes.size())){
            error=GuardError("Loaded native guard mismatch or already hooked",g.rva);return false;
        }
    }
    error.clear();return true;
}
std::string Sha256(std::span<unsigned char const> b){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    DWORD size{},got{};std::string result;
    if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof size,&got,0)>=0){
        std::vector<unsigned char> object(size);std::array<unsigned char,32> digest{};
        if(BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0){
            bool ok=true;
            for(std::size_t i=0;i<b.size();){
                auto n=static_cast<ULONG>(std::min<std::size_t>(b.size()-i,1u<<20));
                if(BCryptHashData(hash,const_cast<PUCHAR>(b.data()+i),n,0)<0){ok=false;break;}i+=n;
            }
            if(ok && BCryptFinishHash(hash,digest.data(),32,0)>=0){
                constexpr char hex[]="0123456789ABCDEF";
                for(auto v:digest){result+=hex[v>>4];result+=hex[v&15];}
            }
            BCryptDestroyHash(hash);
        }
    }
    BCryptCloseAlgorithmProvider(algorithm,0);return result;
}
bool ValidateImage(std::span<unsigned char const> b,Profile const& p,std::string& error){
    Pe pe;
    if(!Parse(b,pe)){error="Invalid AMD64 PE bounds";return false;}
    auto n=pe.nt;
    if(n->FileHeader.TimeDateStamp!=p.timestamp || (n->OptionalHeader.SizeOfImage!=p.imageSize && n->OptionalHeader.SizeOfImage!=p.launcherSize)){
        error="Unsupported image metadata";return false;
    }
    if(!Executable(pe,p.hookRva,p.hookEnd) || p.guards.empty()){error="Unbounded executable target";return false;}
    for(auto const& g:p.guards){
        auto s=Executable(pe,g.rva,g.end);
        if(!s || g.end-g.rva<g.bytes.size() || (g.validation!=GuardValidation::Runtime && g.validation!=GuardValidation::DiskEvidence)){
            error=GuardError("Invalid guard range or role",g.rva);return false;
        }
        auto raw=static_cast<std::size_t>(s->PointerToRawData)+g.rva-s->VirtualAddress;
        if(memcmp(b.data()+raw,g.bytes.data(),g.bytes.size())){error=GuardError("Native byte guard mismatch",g.rva);return false;}
    }
    auto digest=Sha256(b);
    if(digest.empty() || (digest!=p.hashes[0] && digest!=p.hashes[1])){error="Unrecognized complete image SHA256";return false;}
    error.clear();return true;
}
bool ValidateHost(Profile const& p,std::uintptr_t& base,std::string& error){
    base=0;wchar_t path[32768]{};
    if(!GetModuleFileNameW(nullptr,path,32768)){error="Cannot resolve host";return false;}
    auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){error="Cannot read immutable host image";return false;}
    LARGE_INTEGER size{};HANDLE mapping{};void* view{};bool accepted=false;
    if(GetFileSizeEx(file,&size) && size.QuadPart>0 && size.QuadPart<0x40000000){
        mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY,0,0,nullptr);
        if(mapping)view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);
        if(view)accepted=ValidateImage({static_cast<unsigned char const*>(view),static_cast<std::size_t>(size.QuadPart)},p,error);
    }
    if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);CloseHandle(file);
    if(accepted){
        auto candidate=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        if(LoadedGuards(candidate,p,error)){base=candidate;return true;}
    }
    if(error.empty())error="Cannot map supported host image";
    return false;
}
}
