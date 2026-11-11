#include "ToggleSprint/ConfigFile.h"
#include <windows.h>
namespace sprint {
ReloadResult ConfigFile::Poll(ConfigState& state,std::string& error){
    // Hold an ordinary file read handle without write sharing; atomic replacements
    // remain permitted. One Poll can never combine bytes from two versions.
    auto file=CreateFileW(path_.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE){error="Configuration unavailable; last good state retained";return ReloadResult::Rejected;}
    BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};std::string bytes;bool valid=false;
    if(GetFileInformationByHandle(file,&info) && !(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) &&
       GetFileSizeEx(file,&size) && size.QuadPart>=0 && size.QuadPart<=65536){
        bytes.resize(static_cast<std::size_t>(size.QuadPart));DWORD read{};
        valid=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr) && read==bytes.size();
    }
    CloseHandle(file);
    if(!valid){error="Configuration bounds or file type refused; last good state retained";return ReloadResult::Rejected;}
    if(observed_ && previous_==bytes)return ReloadResult::Unchanged;
    observed_=true;previous_=bytes;
    return state.Apply(bytes,error)?ReloadResult::Applied:ReloadResult::Rejected;
}
}
