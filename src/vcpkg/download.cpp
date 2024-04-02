#include <vcpkg/base/fmt.h>
#include <vcpkg/base/jsonreader.h>
#include <vcpkg/base/strings.h>

#include <vcpkg/download.h>

namespace vcpkg
{
    struct GitHubDeserializer final : Json::IDeserializer<DownloadedFile>
    {
        LocalizedString type_name() const override { return LocalizedString::from_raw("GitHub"); }

        Optional<type> visit_object(Json::Reader& r, const Json::Object& obj) const override
        {
            DownloadedFile result;

            result.kind = DownloadType::GitHub;
            result.headers.emplace_back("Accept: application/vnd.github+json");
            result.headers.emplace_back("X-GitHub-Api-Version: 2022-11-28");

            std::string repo;
            r.required_object_field(type_name(), obj, "repo", repo, Json::UntypedStringDeserializer::instance);
            std::string ref;
            r.required_object_field(type_name(), obj, "ref", ref, Json::UntypedStringDeserializer::instance);
            r.required_object_field(type_name(), obj, "sha512", result.sha_512, Json::UntypedStringDeserializer::instance);
            r.required_object_field(type_name(), obj, "out-var", result.out_var, Json::UntypedStringDeserializer::instance);
            std::string gh_gost = "github.com";
            r.optional_object_field(obj, "host", gh_gost, Json::UntypedStringDeserializer::instance);
            std::string auth_token;
            if (r.optional_object_field(obj, "authorization-token", auth_token, Json::UntypedStringDeserializer::instance))
            {
                result.headers.push_back("Authorization: Bearer " + auth_token);
            }

            r.optional_object_field(obj, "patches", result.patches, Json::IdentifierArrayDeserializer::instance);

            result.urls.emplace_back(fmt::format("https://api.{}/repos/{}/tarball/{}", gh_gost, repo, ref));

            result.file_name = fmt::format("{}-{}.tar.gz", repo, ref);
            Strings::inplace_replace_all(result.file_name, '/', '-');

            return result;
        }

        static const GitHubDeserializer instance;
    };

    const GitHubDeserializer GitHubDeserializer::instance;

    struct DownloadDeserializer final : Json::IDeserializer<DownloadedFile>
    {
        LocalizedString type_name() const override { return LocalizedString::from_raw("download.json"); }

        Optional<type> visit_object(Json::Reader& r, const Json::Object& obj) const override
        {
            DownloadedFile result;
            if (auto value = obj.get("github"))
            {
                r.visit_in_key(*value, "github", result, GitHubDeserializer::instance);
            }
            return result;
        }

        static const DownloadDeserializer instance;
    };

    const DownloadDeserializer DownloadDeserializer::instance;

    std::vector<DownloadedFile> parse_download(StringView str)
    {
        Json::Reader reader("download.json");
        auto obj = Json::parse_object(str, "download.json");
        auto res = reader.array_elements(obj.get()->get("files")->array(VCPKG_LINE_INFO), DownloadDeserializer::instance);
        return *res.get();
    }
} // namespace vcpkg

