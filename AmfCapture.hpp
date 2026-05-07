#pragma once
#include <expected>
#include <filesystem>
#include <string_view>

namespace Capture {
    std::expected<std::filesystem::path, std::string_view> GrabScreenAMF();
}