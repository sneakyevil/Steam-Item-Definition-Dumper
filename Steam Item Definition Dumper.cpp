#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "steam_api")
#include <steam_api.h>

bool WriteSteamAppID(std::string m_ID)
{
    std::ofstream m_File("steam_appid.txt");
    if (!m_File.is_open()) return false;

    m_File << m_ID;

    m_File.close();
    return true;
}

#define m_gTitle "Steam Item Definition Dumper"

struct SteamItem_Data
{
    SteamItemDef_t m_ID = 0;
    std::vector<std::string> m_PropertyNames;
    std::vector<std::string> m_PropertyData;

    SteamItem_Data() { }
    SteamItem_Data(SteamItemDef_t id)
    {
        m_ID = id;
    }
};
std::vector<SteamItem_Data> m_gItems;
std::vector<std::string> m_gPropertyFilter = { "appid" };

int main()
{
    SetConsoleTitleA(m_gTitle);

    std::cout << "Enter AppID you want to dump: ";

    std::string m_AppID;
    std::cin >> m_AppID;

    if (!WriteSteamAppID(m_AppID))
    {
        MessageBoxA(0, "Couldn't write steam_appid.txt", "WriteSteamAppID", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!SteamAPI_Init())
    {
        MessageBoxA(0, "Steam client needs to be running or there is no license for current app!", "SteamAPI_Init", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!SteamInventory()->LoadItemDefinitions())
    {
        MessageBoxA(0, "Couldn't load item defs...", "SteamInventory()->LoadItemDefinitions()", MB_OK | MB_ICONERROR);
        return 1;
    }
    Sleep(1000); // let it pre-cache

    std::vector<SteamItemDef_t> m_ItemDefs;
    uint32 m_ItemDefs_Count = 0;
    if (SteamInventory()->GetItemDefinitionIDs(nullptr, &m_ItemDefs_Count))
    {
        m_ItemDefs.resize(m_ItemDefs_Count);
        SteamInventory()->GetItemDefinitionIDs(m_ItemDefs.data(), &m_ItemDefs_Count);
    }

    if (m_ItemDefs.empty())
    {
        MessageBoxA(0, "Couldn't find item defs...", m_gTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    std::ofstream m_File("dump.json");
    if (!m_File.is_open())
    {
        MessageBoxA(0, "Couldn't open file to write dump...", m_gTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    for (SteamItemDef_t m_ItemDef : m_ItemDefs)
    {
        std::string m_Properties;
        uint32 m_PropertySize = 0;
        if (!SteamInventory()->GetItemDefinitionProperty(m_ItemDef, nullptr, nullptr, &m_PropertySize))
            continue;

        m_Properties.resize(m_PropertySize);
        if (!SteamInventory()->GetItemDefinitionProperty(m_ItemDef, nullptr, &m_Properties[0], &m_PropertySize))
            continue;

        m_gItems.emplace_back(SteamItem_Data(m_ItemDef));
        SteamItem_Data* m_Item = &m_gItems[m_gItems.size() - 1];

        size_t m_Pos = 0;
        while ((m_Pos = m_Properties.find(",")) != std::string::npos)
        {
            std::string m_PropertyName = m_Properties.substr(0, m_Pos);
            m_Properties.erase(0, m_Pos + 1);

            if (std::find(m_gPropertyFilter.begin(), m_gPropertyFilter.end(), m_PropertyName) != m_gPropertyFilter.end())
                continue;

            m_PropertySize = 0;
            if (!SteamInventory()->GetItemDefinitionProperty(m_ItemDef, m_PropertyName.c_str(), nullptr, &m_PropertySize))
                continue;

            std::string m_PropertyData(m_PropertySize, '\0');
            if (!SteamInventory()->GetItemDefinitionProperty(m_ItemDef, m_PropertyName.c_str(), &m_PropertyData[0], &m_PropertySize))
                continue;

            std::replace(m_PropertyData.begin(), m_PropertyData.end(), '\n', ' ');

            m_Item->m_PropertyNames.emplace_back(m_PropertyName);
            m_Item->m_PropertyData.emplace_back(m_PropertyData);
        }
    }

    m_File << "{" << "\n";
    m_File << "\t" << "\"appid\": " << m_AppID << "," << "\n";
    m_File << "\t" << "\"items\": [" << "\n";
    for (const SteamItem_Data m_Item : m_gItems)
    {
        size_t m_PropertyCount = m_Item.m_PropertyNames.size();
        if (m_PropertyCount == 0) continue;

        m_File << "\t" << "{" << "\n";
        
        for (size_t i = 0; m_PropertyCount > i; ++i)
        {
            m_File << "\t\t" << "\"" << m_Item.m_PropertyNames[i].c_str() << "\": " << "\"" << m_Item.m_PropertyData[i].c_str() << "\"";

            if (m_PropertyCount != i + 1)
                m_File << ",";

            m_File << "\n";
        }

        m_File << "\t" << "}," << "\n";

    }
    m_File << "\t" << "]" << "\n";
    m_File << "}";

    m_File.close();

    return 0;
}