#include <vcpkg/base/fmt.h>
#include <vcpkg/base/jsonreader.h>
#include <vcpkg/base/strings.h>

#include <vcpkg/download.h>

namespace vcpkg
{
    struct GitLikeInfo
    {
        std::string repo;
        std::string ref;
        std::string sha_512;
        std::string out_var;
        std::string file_name;
        std::vector<std::string> patches;
    };

    struct GitLikeDeserializer final : Json::IDeserializer<GitLikeInfo>
    {
        LocalizedString type_name() const override { return LocalizedString::from_raw("GitLike"); }

        Optional<type> visit_object(Json::Reader& r, const Json::Object& obj) const override
        {
            GitLikeInfo result;

            r.required_object_field(type_name(), obj, "repo", result.repo, Json::UntypedStringDeserializer::instance);
            r.required_object_field(type_name(), obj, "ref", result.ref, Json::UntypedStringDeserializer::instance);
            r.required_object_field(
                type_name(), obj, "sha512", result.sha_512, Json::UntypedStringDeserializer::instance);
            r.required_object_field(
                type_name(), obj, "out-var", result.out_var, Json::UntypedStringDeserializer::instance);

            r.optional_object_field(obj, "patches", result.patches, Json::IdentifierArrayDeserializer::instance);

            result.file_name = fmt::format("{}-{}.tar.gz", result.repo, result.ref);
            Strings::inplace_replace_all(result.file_name, '/', '-');

            return result;
        }

        static const GitLikeDeserializer instance;
    };

    const GitLikeDeserializer GitLikeDeserializer::instance;

    struct GitHubDeserializer final : Json::IDeserializer<DownloadedFile>
    {
        LocalizedString type_name() const override { return LocalizedString::from_raw("GitHub"); }

        View<StringView> valid_fields() const override
        {
            static constexpr StringView valid_fields[] = {
                StringLiteral{"repo"},
                StringLiteral{"ref"},
                StringLiteral{"sha512"},
                StringLiteral{"out-var"},
                StringLiteral{"host"},
                StringLiteral{"authorization-token"},
                StringLiteral{"patches"},
            };
            return valid_fields;
        }

        Optional<type> visit_object(Json::Reader& r, const Json::Object& obj) const override
        {
            DownloadedFile result;

            result.kind = DownloadType::GitHub;
            result.headers.emplace_back("Accept: application/vnd.github+json");
            result.headers.emplace_back("X-GitHub-Api-Version: 2022-11-28");

            auto maybe_git_result = r.visit(obj, GitLikeDeserializer::instance);
            if (!maybe_git_result.has_value())
            {
                return nullopt;
            }

            auto& git_result = *maybe_git_result.get();

            std::string gh_gost = "github.com";
            r.optional_object_field(obj, "host", gh_gost, Json::UntypedStringDeserializer::instance);
            std::string auth_token;
            if (r.optional_object_field(
                    obj, "authorization-token", auth_token, Json::UntypedStringDeserializer::instance))
            {
                result.headers.push_back("Authorization: Bearer " + auth_token);
            }

            r.optional_object_field(obj, "patches", result.patches, Json::IdentifierArrayDeserializer::instance);

            result.urls.emplace_back(
                fmt::format("https://api.{}/repos/{}/tarball/{}", gh_gost, git_result.repo, git_result.ref));

            result.sha_512 = std::move(git_result.sha_512);
            result.out_var = std::move(git_result.out_var);
            result.patches = std::move(git_result.patches);
            result.file_name = std::move(git_result.file_name);

            return result;
        }

        static const GitHubDeserializer instance;
    };

    const GitHubDeserializer GitHubDeserializer::instance;

    struct DownloadDeserializer final : Json::IDeserializer<DownloadedFile>
    {
        LocalizedString type_name() const override { return LocalizedString::from_raw("download.json"); }

        View<StringView> valid_fields() const override
        {
            static constexpr StringView valid_fields[] = {
                StringLiteral{"github"},
                StringLiteral{"gitlab"},
                StringLiteral{"git"},
                StringLiteral{"bitbucket"},
                StringLiteral{"sourceforge"},
                StringLiteral{"distfile"},
            };
            return valid_fields;
        }

        Optional<type> visit_object(Json::Reader& r, const Json::Object& obj) const override
        {
            DownloadedFile result;
            if (auto value = obj.get("github"))
            {
                r.visit_in_key(*value, "github", result, GitHubDeserializer::instance);
            }
            else if (auto value = obj.get("gitlab"))
            {
                r.visit_in_key(*value, "gitlab", result, GitHubDeserializer::instance);
            }
            else if (auto value = obj.get("git"))
            {
                r.visit_in_key(*value, "git", result, GitHubDeserializer::instance);
            }
            else if (auto value = obj.get("bitbucket"))
            {
                r.visit_in_key(*value, "bitbucket", result, GitHubDeserializer::instance);
            }
            else if (auto value = obj.get("sourceforge"))
            {
                r.visit_in_key(*value, "sourceforge", result, GitHubDeserializer::instance);
            }
            else if (auto value = obj.get("distfile"))
            {
                r.visit_in_key(*value, "distfile", result, GitHubDeserializer::instance);
            }
            else
            {
                Checks::unreachable(VCPKG_LINE_INFO);
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
        auto res =
            reader.array_elements(obj.get()->get("files")->array(VCPKG_LINE_INFO), DownloadDeserializer::instance);
        return *res.get();
    }
} // namespace vcpkg
