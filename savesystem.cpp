#include "savesystem.hpp"

#include <filesystem>
#include <fstream>

#include "logger.hpp"

using json = nlohmann::json;

namespace
{
    constexpr std::uint32_t SAVE_VERSION = 1;
    const std::filesystem::path saveDirectory = "RiftgateCompanion.Save";
}

template <>
json SaveSystem::toJson<DifficultyData>(const DifficultyData& data)
{
    return json{{"_version", SAVE_VERSION}, {"untrackedQuests", data.untrackedQuests}};
}

template <>
DifficultyData SaveSystem::fromJson<DifficultyData>(const json& j)
{
    const auto version = j.value("_version", std::uint32_t{1});
    DifficultyData data;
    data.untrackedQuests = j.value("untrackedQuests", std::unordered_set<QuestId>{});
    return data;
}

bool SaveSystem::SaveDifficulty(const std::string& characterName, std::uint8_t difficulty, const DifficultyData& data)
{
    return SaveJsonFile( saveDirectory / characterName / std::to_string(difficulty) / "untrackedQuests.json", toJson<DifficultyData>(data));
}

DifficultyData SaveSystem::LoadDifficulty(const std::string& characterName, std::uint8_t difficulty)
{
    return LoadJsonFile<DifficultyData>(saveDirectory / characterName / std::to_string(difficulty) / "untrackedQuests.json").value_or(DifficultyData{});
}

bool SaveSystem::SaveJsonFile(const std::filesystem::path& savePath, const json& j)
{
    std::error_code ec;
    std::filesystem::create_directories(savePath.parent_path(), ec);
    if (ec)
    {
        Log("Error creating directory %ls", savePath.parent_path().c_str());
        return false;
    }

    std::filesystem::path tempPath = savePath;
    tempPath += ".tmp";
    {
        std::ofstream file(tempPath, std::ios::out | std::ios::trunc);
        if (!file)
        {
            Log("Unable to create temp path %ls", tempPath.c_str());
            return false;
        }
        file << j.dump(4);
        if (!file)
        {
            Log("Unable to write into temp file %ls", tempPath.c_str());
            return false;
        }
        file.flush();
        if (!file)
        {
            Log("Unable to flush temp file %ls", tempPath.c_str());
            return false;
        }
    }

    std::filesystem::remove(savePath, ec);
    if (ec)
    {
        Log("Unable to remove save file %ls", savePath.c_str());
        return false;
    }
    std::filesystem::rename(tempPath, savePath, ec);
    if (ec)
    {
        Log("Unable to rename temp file %ls to save file %ls", tempPath.c_str(), savePath.c_str());
        return false;
    }

    Log("Saved save file %ls", savePath.c_str());
    return true;
}

template <typename T>
std::optional<T> SaveSystem::LoadJsonFile(const std::filesystem::path& savePath)
{
    std::ifstream file(savePath);
    if (!file)
    {
        Log("Unable to open save file %ls", savePath.c_str());
        return std::nullopt;
    }
    try
    {
        json j;
        file >> j;
        Log("Loaded save file %ls", savePath.c_str());
        return fromJson<T>(j);
    }
    catch (const json::exception&)
    {
        Log("Exception while parsing save file %ls", savePath.c_str());
        return std::nullopt;
    }
}
