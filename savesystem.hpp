#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <optional>
#include <nlohmann/json.hpp>

using QuestId = std::uint32_t;
struct DifficultyData {
    std::unordered_set<QuestId> untrackedQuests;
};

class SaveSystem
{
    public:
        static bool SaveDifficulty(const std::string& characterName, std::uint8_t difficulty, const DifficultyData& data);
        static DifficultyData LoadDifficulty(const std::string& characterName, std::uint8_t difficulty);

    private:
        template <typename T>
        static nlohmann::json toJson(const T& data);
        template <typename T>
        static T fromJson(const nlohmann::json& j);

        static bool SaveJsonFile(const std::filesystem::path& savePath, const nlohmann::json& j);
        template <typename T>
        static std::optional<T> LoadJsonFile(const std::filesystem::path& savePath);
};
