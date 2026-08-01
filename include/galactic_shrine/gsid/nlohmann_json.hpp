#pragma once

#include <galactic_shrine/gsid/gsid.hpp>
#include <nlohmann/json.hpp>

namespace GalacticShrine
{
    inline void to_json(nlohmann::json& json, const GsId& value)
    {
        json = value.ToString(
            GsIdOptions::GetDefaultJsonFormat(),
            GsIdOptions::GetDefaultCase());
    }

    inline void from_json(const nlohmann::json& json, GsId& value)
    {
        value = GsId::Parse(json.get<std::string>());
    }
}
