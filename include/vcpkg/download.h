#pragma once

#include <vcpkg/base/stringview.h>

#include <string>
#include <vector>

namespace vcpkg
{
    enum class DownloadType
    {
        GitHub,
    };
    struct DownloadedFile
    {
        std::vector<std::string> urls;
        std::string file_name;
        std::vector<std::string> headers;
        std::string sha_512;
        std::string out_var;
        std::vector<std::string> patches;
        DownloadType kind;
        bool use_head_version = false;
    };

    std::vector<DownloadedFile> parse_download(StringView str);

} // namespace vcpkg
