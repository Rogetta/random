#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <limits>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace {

std::uint32_t get_random_uint32()
{
    std::uint32_t value = 0;

    const NTSTATUS status = BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(&value),
        sizeof(value),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (!BCRYPT_SUCCESS(status))
        throw std::runtime_error("BCryptGenRandom failed");

    return value;
}

}

// 返回 [0, upperBound) 的无偏随机数。
// 使用项目原有的 BCryptGenRandom 作为随机源。
std::uint32_t get_random_bounded(std::uint32_t upperBound)
{
    if (upperBound == 0)
        throw std::invalid_argument(
            "upperBound must be greater than zero");

    constexpr std::uint64_t range =
        static_cast<std::uint64_t>(
            std::numeric_limits<std::uint32_t>::max()) + 1ULL;

    const std::uint64_t limit =
        range - (range % upperBound);

    std::uint32_t value;

    do {
        value = get_random_uint32();
    } while (static_cast<std::uint64_t>(value) >= limit);

    return value % upperBound;
}
