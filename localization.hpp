#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

enum class TextId : uint16_t
{
#define X(id, key) id,
#include "localization_keys.inc"
#undef X

    Count
};

class Localization
{
    public:
        bool Load(std::string_view language);

        [[nodiscard]]
        const char* tr(TextId id) const
        {
            return m_table[static_cast<size_t>(id)].c_str();
        }

    private:
        static constexpr size_t Count = static_cast<size_t>(TextId::Count);
        std::array<std::string, Count> m_table;
};
