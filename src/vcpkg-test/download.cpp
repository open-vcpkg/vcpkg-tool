#include <vcpkg-test/util.h>

#include <vcpkg/download.h>

using namespace vcpkg;

TEST_CASE("github", "[download]") {
    static constexpr StringLiteral str{
        R"({"files": [{"github": {"repo": "microsoft/vcpkg", "ref": "q", "sha512": "9", "out-var": "SOURCE_PATH"}}]})"};

    auto res = parse_download(str);

    REQUIRE(res.size() == 1);
    REQUIRE(res[0].urls.size() == 1);
    REQUIRE(res[0].urls[0] == "https://api.github.com/repos/microsoft/vcpkg/tarball/q");
    REQUIRE(res[0].sha_512 == "9");
    REQUIRE(res[0].out_var == "SOURCE_PATH");
    REQUIRE(res[0].kind == DownloadType::GitHub);
    REQUIRE(res[0].patches.empty());
    REQUIRE(res[0].headers.size() == 2);
    REQUIRE(res[0].file_name == "microsoft-vcpkg-q.tar.gz");
}
