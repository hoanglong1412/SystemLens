/**
 * @file    ProcFs.h
 * @brief   Small helpers to read the text files of /proc and /sys used by the Linux providers.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace systemlens::providers::linux_os::procfs {

std::optional<std::string> readFile(const std::filesystem::path& path);
std::string readFileOrThrow(const std::filesystem::path& path);
std::optional<std::string> readTrimmed(const std::filesystem::path& path);
std::optional<std::uint64_t> readUnsigned(const std::filesystem::path& path);

std::string_view trim(std::string_view text);
std::optional<std::uint64_t> parseUnsigned(std::string_view text);
std::optional<double> parseDouble(std::string_view text);
std::vector<std::string_view> splitWhitespace(std::string_view text);
std::vector<std::string_view> splitLines(std::string_view text);
bool isAllDigits(std::string_view text);

} // namespace systemlens::providers::linux_os::procfs
