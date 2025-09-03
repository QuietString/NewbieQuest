#include "Demo.h"

#include <filesystem>
#include <iostream>
#include "Classes/Monster.h"
#include "CoreMinimal.h"
#include "Engine/AssetManager.h"

namespace Demo
{
    void Case1()
    {
        QAssetManager& AssetManager = QAssetManager::Get();
        
        auto Monster = NewObject<QMonster>("OrcBoss");
        
        Monster->SetLevel(35);
        Monster->SetRage(0.9f);
        Monster->SetBoss(true);
        Monster->GetPosition().X = 100.f;
        Monster->GetPosition().Y = 0.f;
        Monster->GetPosition().Z = -50.f;

        std::cout << "=== Monster Created ===\n";
        AssetManager.DumpObject(*Monster, std::cout);

        // binary save/load
        AssetManager.SaveAsset(*Monster, "Monsters/OrcBoss");        // -> .../Monsters/OrcBoss.qasset
        auto MonsterFromBinary = AssetManager.LoadAssetBinary("Monsters/OrcBoss");
        
        std::cout << "\n=== After Binary Load ===\n";
        AssetManager.DumpObject(*MonsterFromBinary, std::cout);
        
        // text save/load
        AssetManager.SaveAssetByText(*Monster, "Monsters/OrcBoss");             // -> .../Contents/Monsters/OrcBoss.qasset_t
        auto MonsterFromText = AssetManager.LoadAssetFromText("Monsters/OrcBoss");

        std::cout << "\n=== After Text Load ===\n";
        AssetManager.DumpObject(*MonsterFromText, std::cout);
        
    }
}
