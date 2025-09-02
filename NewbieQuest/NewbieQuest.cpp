#include <iostream>
#include "Reflection/Public/Assets.h"
#include "Reflection/Public/Serialization.h"
#include "Reflection/Public/Binary.h"
#include "Reflection/Public/ObjectFactory.h"
#include "Classes/Player.h"
#include "Classes/Monster.h"

int main() {
    using namespace qreflect;
    using namespace qreflect::assets;

    auto m = NewObject<Monster>("OrcBoss");
    m->SetLevel(35);
    m->SetRage(0.9f);
    m->SetBoss(true);
    m->GetPosition().X = 100.f;
    m->GetPosition().Y = 0.f;
    m->GetPosition().Z = -50.f;

    std::cout << "=== Monster Dump ===\n";
    DumpObject(*m, std::cout);

    // text save/load
    SaveAsset(*m, "Monsters/OrcBoss");             // -> .../Contents/Monsters/OrcBoss.qasset
    auto t = LoadAsset("Monsters/OrcBoss");

    // binary save/load
    SaveAssetBinary(*m, "Monsters/OrcBoss");        // -> .../Monsters/OrcBoss.qassetb
    auto b = LoadAssetBinary("Monsters/OrcBoss");

    std::cout << "\n=== After Binary Load ===\n";
    DumpObject(*b, std::cout);
}
