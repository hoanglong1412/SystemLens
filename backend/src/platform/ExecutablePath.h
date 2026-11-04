/**
 * @file    ExecutablePath.h
 * @brief   Location of the running executable, independent of the current working directory.
 *
 * OS-independent declaration; the implementation is platform/<os>/ExecutablePath.cpp, selected by CMake.
 */
#pragma once

#include <filesystem>

namespace systemlens::platform {

std::filesystem::path executableDirectory();

} // namespace systemlens::platform
