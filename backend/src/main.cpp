/**
 * @file    main.cpp
 * @brief   Entry point of systemlens-backend.exe: parse configuration and run the Application.
 */
#include "app/AppConfig.h"
#include "app/Application.h"

#include <exception>
#include <iostream>

namespace {

constexpr int kExitOk = 0;
constexpr int kExitUsageError = 2;
constexpr int kExitRuntimeError = 1;

} // namespace

/**
 * @brief Program entry point.
 * @param argc Argument count.
 * @param argv Arguments (see AppConfig.h for the supported options).
 * @return 0 on normal shutdown, 2 on invalid usage, 1 on runtime failure.
 */
int main(int argc, char* argv[])
{
    systemlens::app::AppConfig config;
    try
    {
        config = systemlens::app::parseAppConfig(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n" << systemlens::app::kUsage;
        return kExitUsageError;
    }

    if (config.showHelp)
    {
        std::cout << systemlens::app::kUsage;
        return kExitOk;
    }

    try
    {
        systemlens::app::Application application(std::move(config));
        return application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return kExitRuntimeError;
    }
}
