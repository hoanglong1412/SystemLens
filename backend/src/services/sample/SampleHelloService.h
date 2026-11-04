/**
 * @file    SampleHelloService.h
 * @brief   [SAMPLE API] Default ISampleHelloService: greets with the text supplied by the
 *          platform's hello provider (e.g. "hello from Windows").
 */
#pragma once

#include "providers/sample/IHelloProvider.h"
#include "services/sample/ISampleHelloService.h"

#include <memory>

namespace systemlens::services::sample {

class SampleHelloService final : public ISampleHelloService
{
public:
    explicit SampleHelloService(std::shared_ptr<const providers::sample::IHelloProvider> provider);

    std::string getGreeting() const override;

private:
    std::shared_ptr<const providers::sample::IHelloProvider> m_provider; ///< Empty when the platform has none.
};

} // namespace systemlens::services::sample
