#include <random>
#include <chrono>
#include <windows.h>
#include <bcrypt.h>
#include <type_traits>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")


template<typename T,
         typename = std::enable_if_t<
             std::is_arithmetic_v<T> || std::is_trivially_copyable_v<T>>>
void get_random(T& value) {
    static_assert(!std::is_pointer_v<T>, "T must not be a pointer type");
    static_assert(sizeof(T) > 0, "T must be a complete type");

    NTSTATUS status = BCryptGenRandom(
        nullptr,                           // 使用系统首选 RNG
        reinterpret_cast<PUCHAR>(&value),  // 指向变量的内存
        sizeof(T),                         // 字节数
        BCRYPT_USE_SYSTEM_PREFERRED_RNG    // 使用系统首选随机数生成器
    );

    if (!BCRYPT_SUCCESS(status)) {
        throw std::runtime_error("BCryptGenRandom failed");
    }
}