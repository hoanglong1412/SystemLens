/**
 * @file    ProcFs.cpp
 * @brief   Implementation of the /proc and /sys reading helpers.
 */
#include "providers/linux/ProcFs.h"

#include "providers/ProviderError.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iterator>

namespace systemlens::providers::linux_os::procfs {

namespace {

constexpr std::string_view kWhitespace = " \t\r\n";

} // namespace

/**
 * @brief Read a whole file.
 * @param path File to read.
 * @return Content, or std::nullopt when the file cannot be opened (missing, no permission).
 */
std::optional<std::string> readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return std::nullopt;
    }
    // /proc files report size 0, so the content must be read until EOF instead of by size.
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/**
 * @brief Read a whole file that must exist.
 * @param path File to read.
 * @return Content.
 * @throws ProviderError when the file cannot be opened.
 */
std::string readFileOrThrow(const std::filesystem::path& path)
{
    auto content = readFile(path);
    if (!content)
    {
        throw ProviderError("Cannot read " + path.string());
    }
    return std::move(*content);
}

/**
 * @brief Read a file and strip surrounding whitespace (sysfs values end with a newline).
 * @param path File to read.
 * @return Trimmed content, or std::nullopt when unreadable.
 */
std::optional<std::string> readTrimmed(const std::filesystem::path& path)
{
    const auto content = readFile(path);
    if (!content)
    {
        return std::nullopt;
    }
    return std::string(trim(*content));
}

/**
 * @brief Read a file holding one unsigned decimal number.
 * @param path File to read.
 * @return The number, or std::nullopt when unreadable or not a number.
 */
std::optional<std::uint64_t> readUnsigned(const std::filesystem::path& path)
{
    const auto content = readFile(path);
    return content ? parseUnsigned(trim(*content)) : std::nullopt;
}

/**
 * @brief Strip leading and trailing whitespace.
 * @param text Input.
 * @return View into text without surrounding whitespace.
 */
std::string_view trim(std::string_view text)
{
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos)
    {
        return {};
    }
    const auto last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

/**
 * @brief Parse an unsigned decimal number; the whole text must be the number.
 * @param text Input without surrounding whitespace.
 * @return The number, or std::nullopt when text is not a number.
 */
std::optional<std::uint64_t> parseUnsigned(std::string_view text)
{
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty())
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Parse a decimal floating point number; the whole text must be the number.
 * @param text Input without surrounding whitespace.
 * @return The number, or std::nullopt when text is not a number.
 */
std::optional<double> parseDouble(std::string_view text)
{
    double value = 0.0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty())
    {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Split on runs of whitespace.
 * @param text Input.
 * @return Non-empty tokens, as views into text.
 */
std::vector<std::string_view> splitWhitespace(std::string_view text)
{
    std::vector<std::string_view> tokens;
    std::size_t position = 0;
    while ((position = text.find_first_not_of(kWhitespace, position)) != std::string_view::npos)
    {
        const auto end = std::min(text.find_first_of(kWhitespace, position), text.size());
        tokens.push_back(text.substr(position, end - position));
        position = end;
    }
    return tokens;
}

/**
 * @brief Split into lines.
 * @param text Input.
 * @return Lines without their terminator (empty lines kept), as views into text.
 */
std::vector<std::string_view> splitLines(std::string_view text)
{
    std::vector<std::string_view> lines;
    while (!text.empty())
    {
        const auto end = std::min(text.find('\n'), text.size());
        lines.push_back(text.substr(0, end));
        text.remove_prefix(std::min(end + 1, text.size()));
    }
    return lines;
}

/**
 * @brief Tell whether a text is a non-empty run of decimal digits (e.g. a PID directory name).
 * @param text Input.
 * @return True when every character is 0..9 and text is not empty.
 */
bool isAllDigits(std::string_view text)
{
    return !text.empty() && std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; });
}

} // namespace systemlens::providers::linux_os::procfs
