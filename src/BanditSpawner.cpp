#include "BanditSpawner.h"
#include "Settings.h"
#include <random>
#include <cmath>
#include <SKSE/SKSE.h>
#include <SimpleIni.h>
#include <sstream>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif



// ── Thread-local RNG ──────────────────────────────────────────────
std::mt19937& BanditSpawner::GetRNG() {
    static thread_local std::mt19937 rng{ std::random_device{}() };
    return rng;
}

void BanditSpawner::DumpLeveledLists() {
    if (!Settings::EnableLogging) return;
    
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    SKSE::log::info("=== DUMPING ALL RELEVANT LEVELED LISTS ===");
    for (auto form : dataHandler->GetFormArray<RE::TESLevCharacter>()) {
        if (!form) continue;
        const char* eid = form->GetFormEditorID();
        if (!eid) continue;
        
        std::string editorID(eid);
        // Sadece Bandit, Vampire, Warlock, Forsworn, Draugr kelimelerini içerenleri logla
        if (editorID.find("Bandit") != std::string::npos ||
            editorID.find("Vampire") != std::string::npos ||
            editorID.find("Warlock") != std::string::npos ||
            editorID.find("Forsworn") != std::string::npos ||
            editorID.find("Draugr") != std::string::npos) 
        {
            SKSE::log::info("  Found LvlList: '{}' -> FormID: 0x{:08X}", editorID, form->GetFormID());
        }
    }
    SKSE::log::info("=== DUMP FINISHED ===");
}

// ── Dynamic Fallback Cache ────────────────────────────────────────
std::vector<RE::TESBoundObject*>& BanditSpawner::GetCacheForFaction(FactionType faction) {
    static std::vector<RE::TESBoundObject*> cacheBandit;
    static std::vector<RE::TESBoundObject*> cacheVampire;
    static std::vector<RE::TESBoundObject*> cacheWarlock;
    static std::vector<RE::TESBoundObject*> cacheForsworn;
    static std::vector<RE::TESBoundObject*> cacheDraugr;
    static std::vector<RE::TESBoundObject*> cacheAnimal;
    static std::vector<RE::TESBoundObject*> cacheFalmer;
    static std::vector<RE::TESBoundObject*> cacheDwemer;
    static std::vector<RE::TESBoundObject*> cacheUnknown;

    switch (faction) {
        case FactionType::Bandit:   return cacheBandit;
        case FactionType::Vampire:  return cacheVampire;
        case FactionType::Warlock:  return cacheWarlock;
        case FactionType::Forsworn: return cacheForsworn;
        case FactionType::Draugr:   return cacheDraugr;
        case FactionType::Animal:   return cacheAnimal;
        case FactionType::Falmer:   return cacheFalmer;
        case FactionType::Dwemer:   return cacheDwemer;
        default:                    return cacheUnknown;
    }
}

void BanditSpawner::UpdateCache(RE::TESObjectCELL* cell, FactionType faction) {
    if (!cell || faction == FactionType::Unknown) return;

    auto& cache = GetCacheForFaction(faction);
    
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    if (Settings::EnableLogging) {
        SKSE::log::info("  UpdateCache: Scanning {} references in cell 0x{:08X}", cell->GetRuntimeData().references.size(), cell->GetFormID());
    }

    // Sadece uygun keyword/faction'a sahip npc'leri cache'e ekle
    bool added = false;
    for (auto& ref : cell->GetRuntimeData().references) {
        if (!ref) continue;
        auto actor = ref->As<RE::Actor>();
        if (!actor || actor == player) continue;
        
        auto baseObj = actor->GetActorBase();
        if (!baseObj) continue;

        if (Settings::EnableLogging) {
            SKSE::log::info("    - Inspecting Actor: refID=0x{:08X}, baseID=0x{:08X}, name='{}', isDead={}",
                            ref->GetFormID(), baseObj->GetFormID(), baseObj->GetName(), actor->IsDead());
        }
        
        // 0xFF ile baslayan formlar (ornegin 0xFF00113C) oyun icerisinde olusturulmus gecici kopyalardir (Runtime Temporary).
        // Bu formlardan NPC uretip (PlaceObjectAtMe) ardindan oyunu kaydedersek save dosyasi bozulur ve Save Load Crash yasanir!
        // Bu yuzden sadece eklentilerden gelen statik formlari (0x00 - 0xFE) kabul etmeliyiz.
        if (baseObj->GetFormID() >= 0xFF000000) {
            if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Runtime form (0xFF...)");
            continue;
        }
        
        // Dead actors: only useful as anchors, not as cache templates. Skip dead.
        if (actor->IsDead()) {
            if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Dead");
            continue;
        }
        
        // Faction kontrolü yapalım (Öncelik: vanilla faction ID'leri)
        bool isCorrectFaction = false;
        bool isKnownOtherFaction = false;

        actor->VisitFactions([&](RE::TESFaction* f, int8_t rank) {
            if (f) {
                RE::FormID fID = f->GetFormID();
                
                // Dost faction kontrolü
                if (fID == 0x0005C84E || fID == 0x00019809) { 
                    isCorrectFaction = false; 
                    isKnownOtherFaction = true; 
                    return true; 
                }

                // Bilinen factionları gruplayalım
                bool isBandit = (fID == 0x0001BCC0);
                bool isVampire = (fID == 0x00027242);
                bool isForsworn = (fID == 0x00043599);
                bool isWarlock = (fID == 0x00030C66);
                bool isDraugr = (fID == 0x0002430D);
                bool isAnimal = (
                    fID == 0x0001CB62 || fID == 0x00028FDF || fID == 0x0002C6C8 || 
                    fID == 0x00043596 || fID == 0x0004359A || fID == 0x00043598 || 
                    fID == 0x00043594 || fID == 0x00043595 || fID == 0x00043597
                );
                bool isFalmer = (fID == 0x0002446A);
                bool isDwemer = (fID == 0x0001BCC1);

                if (faction == FactionType::Bandit && isBandit) isCorrectFaction = true;
                else if (faction == FactionType::Vampire && isVampire) isCorrectFaction = true;
                else if (faction == FactionType::Forsworn && isForsworn) isCorrectFaction = true;
                else if (faction == FactionType::Warlock && isWarlock) isCorrectFaction = true;
                else if (faction == FactionType::Draugr && isDraugr) isCorrectFaction = true;
                else if (faction == FactionType::Animal && isAnimal) isCorrectFaction = true;
                else if (faction == FactionType::Falmer && isFalmer) isCorrectFaction = true;
                else if (faction == FactionType::Dwemer && isDwemer) isCorrectFaction = true;
                else if (isBandit || isVampire || isForsworn || isWarlock || isDraugr || isAnimal || isFalmer || isDwemer) {
                    // Kendi aradığımız faction değil, ama BAŞKA bir bilinen düşman faction'ına sahip.
                    isKnownOtherFaction = true;
                }
            }
            return false;
        });

        // ── KESİN DÜŞMANLIK KONTROLÜ (BUG FIX) ──
        // NPC'nin oyuncuya karşı gerçekten düşmanca bir tavır sergileyip sergilemediğini kontrol et.
        // SRC (Skyrim Realistic Conquering) gibi modlar zindanları "dost madencilere" (Embershard Miner)
        // çevirdiğinde, bu madencilerin combatStyle'ı olsa bile oyuncuya dostturlar.
        // IsHostileToActor() Faction ilişkilerine bakar, savaş başlamamış olsa bile doğru döner.
        bool isHostile = actor->IsHostileToActor(player);

        if (!isHostile) {
            if (Settings::EnableLogging) {
                SKSE::log::info("      -> SKIPPED: Not Hostile to Player");
            }
            continue;
        }

        // ── KESİN TÜR KONTROLÜ (IRK/KEYWORD) ──
        // Şablonlardan veya ırklardan gelen factionlar her zaman VisitFactions'ta çıkmayabilir.
        // Bu yüzden yaratık olup olmadığını en güvenilir yol olan Keyword'ler ile kontrol ediyoruz.
        bool isCreature = baseObj->HasKeywordString("ActorTypeAnimal") || 
                          baseObj->HasKeywordString("ActorTypeCreature") || 
                          baseObj->HasKeywordString("ActorTypeMonster") ||
                          baseObj->HasKeywordString("ActorTypeDragon");

        bool isUndead = baseObj->HasKeywordString("ActorTypeUndead");
        bool isDwarven = baseObj->HasKeywordString("ActorTypeDwarven");

        if (faction == FactionType::Bandit || faction == FactionType::Warlock || faction == FactionType::Forsworn) {
            if (isCreature || isDwarven || isUndead) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Humanoid, found Creature/Undead/Dwarven.");
                continue;
            }
        } else if (faction == FactionType::Vampire) {
            if (isCreature || isDwarven) continue;
        } else if (faction == FactionType::Animal) {
            if (!isCreature) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Animal, found non-Animal.");
                continue;
            }
        } else if (faction == FactionType::Draugr) {
            if (!isUndead) continue;
        } else if (faction == FactionType::Dwemer) {
            if (!isDwarven) continue;
        }

        // GENIŞLETILMIŞ FALLBACK: Vanilla faction bulunamazsa combatStyle kontrolü kullan.
        // Zaten isHostile == true olduğu için kesinlikle düşman olduklarını biliyoruz.
        if (!isCorrectFaction) {
            // EĞER bu NPC başka BİLİNEN bir düşman grubuna aitse (örneğin Bandit arıyoruz ama bu bir Kurt), 
            // fallback ile onu Bandit olarak kabul etmemeliyiz!
            if (isKnownOtherFaction) {
                if (Settings::EnableLogging) {
                    SKSE::log::info("      -> SKIPPED: Belongs to a different known faction.");
                }
                continue;
            }

            bool isEssential = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kEssential);
            bool isProtected = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kProtected);
            bool hasCombatStyle = (baseObj->combatStyle != nullptr);
            bool isGuard = actor->IsGuard();

            // Creature ise (Draugr/Animal/Falmer/Dwemer) faction ID kontrolü çok güvenilmez,
            // combatStyle yeterli.
            bool isCreatureFaction = (faction == FactionType::Draugr ||
                                      faction == FactionType::Animal ||
                                      faction == FactionType::Falmer ||
                                      faction == FactionType::Dwemer);

            if (hasCombatStyle && !isGuard && (!isEssential || isCreatureFaction) && !isProtected) {
                isCorrectFaction = true;
                if (Settings::EnableLogging) {
                    SKSE::log::info("    - CombatStyle fallback accepted: '{}' (BaseID: 0x{:08X})",
                        baseObj->GetName(), baseObj->GetFormID());
                }
            }
        }

        if (Settings::EnableLogging) {
            SKSE::log::info("    - Found Actor: '{}' (BaseID: 0x{:08X}), isCorrectFaction={}, Dead={}", 
                baseObj->GetName(), baseObj->GetFormID(), isCorrectFaction, actor->IsDead());
        }

        if (isCorrectFaction) {
            // Düşman factionına aitse cache'e ekle
            bool exists = false;
            for (auto* cachedBase : cache) {
                if (cachedBase == baseObj) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                cache.push_back(baseObj);
                added = true;
                if (Settings::EnableLogging) {
                    SKSE::log::info("      -> CACHED!");
                }
                if (cache.size() > 50) { // Keep bounded
                    cache.erase(cache.begin());
                }
            }
        }
    }
    
    if (added) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  UpdateCache: Faction {} cache size is now {}", static_cast<int>(faction), cache.size());
        }
    }
}

// ── Spawn count based on player level ────────────────────────────
int BanditSpawner::GetSpawnCount() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0;

    int pLevel = player->GetLevel();
    if (pLevel < 10) return 0; // Lvl 1-9 hic spawn yok

    int maxSpawns = Settings::MaxSpawnsLevel10_24;
    if (pLevel >= 50)      maxSpawns = Settings::MaxSpawnsLevel50Plus;
    else if (pLevel >= 25) maxSpawns = Settings::MaxSpawnsLevel25_49;

    if (maxSpawns < 1) maxSpawns = 1;
    std::uniform_int_distribution<> dist(1, maxSpawns);
    return dist(GetRNG());
}

struct BossSpawnInfo {
    int guaranteed; // Garanti boss sayisi
    int extraChance; // Ek boss icin yuzde sans (0-100)
};

BossSpawnInfo GetBossSpawnInfo(int pLevel) {
    if (pLevel < 10)   return {0, 0};
    if (pLevel >= 50)  return {2, Settings::BossChanceLevel50Plus};  // 2 garanti + %40 sans
    if (pLevel >= 25)  return {1, Settings::BossChanceLevel25_49};   // 1 garanti + %30 sans
    return                     {0, Settings::BossChanceLevel10_24};  // 0 garanti + %20 sans
}


// ── Fallback FormIDs for vanilla leveled lists ───────────────────
// DEĞİŞİKLİK (CRASH FIX): OBody, SimpleDualSheath gibi 3D Hook kullanan modların 
// "TESLevCharacter" spawn edildiğinde (henüz gerçek NPC'ye dönüşmeden) 
// 3D model okumaya çalışıp oyunu çökertmesini önlemek için LeveledList (LChar) kullanımını KALDIRDIK.
// Artık %100 her zaman "Dinamik Kopyalama (Cache)" sisteminden, halihazırda oyunda var olan 
// gerçek "TESNPC" objelerini kullanıyoruz.
static RE::TESBoundObject* GetFactionBaseObject(FactionType faction, bool isBoss) {
    auto& cache = BanditSpawner::GetCacheForFaction(faction);
    
    // Eğer cache boşsa, anlık olarak mevcut hücreyi tekrar tara
    if (cache.empty()) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player && player->GetParentCell()) {
            BanditSpawner::UpdateCache(player->GetParentCell(), faction);
        }
    }

    if (!cache.empty()) {
        std::uniform_int_distribution<> dist(0, (int)cache.size() - 1);
        auto* cachedBase = cache[dist(BanditSpawner::GetRNG())];
        if (cachedBase) {
            SKSE::log::info("  DYNAMIC COPY: Using a cached NPC base object for Faction {} (Total cached: {})", static_cast<int>(faction), cache.size());
            return cachedBase;
        }
    }

    SKSE::log::error("  DYNAMIC COPY FAILED: No cached NPCs available for Faction {}. Cannot spawn! (Aborting to prevent LChar crashes)", static_cast<int>(faction));
    return nullptr;
}

// ── Determine faction from location keywords ─────────────────────
FactionType BanditSpawner::GetFactionFromLocation(RE::BGSLocation* loc) {
    if (!loc) return FactionType::Unknown;

    auto kwdBandit   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeBanditCamp");
    auto kwdVampire  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeVampireLair");
    auto kwdWarlock  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeWarlockLair");
    auto kwdForsworn = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeForswornCamp");
    auto kwdDraugr   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDraugrCrypt");
    auto kwdDungeon  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");
    
    // YENI FRAKSIYONLAR
    auto kwdAnimal = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeAnimalDen");
    auto kwdFalmer = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeFalmerHive");
    auto kwdDwemer = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDwarvenRuin");

    // FormID fallbacks
    if (!kwdBandit)   kwdBandit   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000171DF);
    if (!kwdVampire)  kwdVampire  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DB);
    if (!kwdWarlock)  kwdWarlock  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DE);
    if (!kwdForsworn) kwdForsworn = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D5);
    if (!kwdDraugr)   kwdDraugr   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D2);
    if (!kwdDungeon)  kwdDungeon  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0E);
    
    if (!kwdAnimal) kwdAnimal = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0C);
    if (!kwdFalmer) kwdFalmer = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D4);
    if (!kwdDwemer) kwdDwemer = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D3);

    if (Settings::EnableLogging) {
        SKSE::log::info("  GetFactionFromLocation: loc=0x{:08X}", loc->GetFormID());
        // loglamayı kısaltalım ki kalabalık yapmasın
    }

    if (kwdBandit   && loc->HasKeyword(kwdBandit)   && Settings::EnableBandits)   return FactionType::Bandit;
    if (kwdForsworn && loc->HasKeyword(kwdForsworn) && Settings::EnableForsworn)  return FactionType::Forsworn;
    if (kwdVampire  && loc->HasKeyword(kwdVampire)  && Settings::EnableVampires)  return FactionType::Vampire;
    if (kwdWarlock  && loc->HasKeyword(kwdWarlock)  && Settings::EnableWarlocks)  return FactionType::Warlock;
    if (kwdDraugr   && loc->HasKeyword(kwdDraugr)   && Settings::EnableDraugr)    return FactionType::Draugr;
    if (kwdAnimal   && loc->HasKeyword(kwdAnimal)   && Settings::EnableAnimals)   return FactionType::Animal;
    if (kwdFalmer   && loc->HasKeyword(kwdFalmer)   && Settings::EnableFalmer)    return FactionType::Falmer;
    if (kwdDwemer   && loc->HasKeyword(kwdDwemer)   && Settings::EnableDwemer)    return FactionType::Dwemer;
    if (kwdDungeon  && loc->HasKeyword(kwdDungeon)  && Settings::EnableBandits)   return FactionType::Bandit;

    return FactionType::Unknown;
}

// ── INI-backed NPC FormID persistence ────────────────────────────
// Faction'a karşılık gelen INI anahtarını döndür
static const char* FactionToINIKey(FactionType f) {
    switch (f) {
        case FactionType::Bandit:   return "Bandit";
        case FactionType::Vampire:  return "Vampire";
        case FactionType::Warlock:  return "Warlock";
        case FactionType::Forsworn: return "Forsworn";
        case FactionType::Draugr:   return "Draugr";
        case FactionType::Animal:   return "Animal";
        case FactionType::Falmer:   return "Falmer";
        case FactionType::Dwemer:   return "Dwemer";
        default:                    return nullptr;
    }
}

// Noktalı virgül ile ayrılmış hex FormID string'ini parse eder, formlara bakıp cache'e ekler.
static void ParseAndAddToCache(const char* csvStr, FactionType faction) {
    if (!csvStr || csvStr[0] == '\0') return;
    auto& cache = BanditSpawner::GetCacheForFaction(faction);

    std::istringstream ss(csvStr);
    std::string token;
    int added = 0, skipped = 0;
    while (std::getline(ss, token, ';')) {
        // Trim whitespace
        auto trim = [](std::string& s) {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
            while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t')) s.pop_back();
        };
        trim(token);
        if (token.empty()) continue;

        // Parse hex FormID (supports 0x prefix or raw hex)
        RE::FormID fid = 0;
        try { fid = static_cast<RE::FormID>(std::stoul(token, nullptr, 16)); }
        catch (...) { ++skipped; continue; }

        // Runtime FormID'leri (0xFF...) yüklemeyi atla - bunlar önceki oturumda spawn edilenlerdi
        if (fid >= 0xFF000000) { ++skipped; continue; }

        auto* form = RE::TESForm::LookupByID<RE::TESBoundObject>(fid);
        if (!form) { ++skipped; continue; }

        // TESNPC olduğunu doğrula
        if (!form->As<RE::TESNPC>()) { ++skipped; continue; }

        // Zaten cache'de var mı?
        auto it = std::find(cache.begin(), cache.end(), form);
        if (it != cache.end()) { ++skipped; continue; }

        cache.push_back(form);
        ++added;
    }
    if (Settings::EnableLogging && (added > 0 || skipped > 0)) {
        SKSE::log::info("  LoadCachedFormIDs [{}]: +{} loaded, {} skipped",
                        FactionToINIKey(faction), added, skipped);
    }
}

void BanditSpawner::LoadCachedFormIDs() {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(Settings::INI_PATH) != SI_OK) {
        SKSE::log::info("LoadCachedFormIDs: INI not found or unreadable, skipping.");
        return;
    }

    constexpr FactionType factions[] = {
        FactionType::Bandit, FactionType::Vampire, FactionType::Warlock,
        FactionType::Forsworn, FactionType::Draugr, FactionType::Animal,
        FactionType::Falmer, FactionType::Dwemer
    };

    for (auto f : factions) {
        const char* key = FactionToINIKey(f);
        if (!key) continue;
        const char* val = ini.GetValue("CachedNPCs", key, nullptr);
        if (val) ParseAndAddToCache(val, f);
    }
    SKSE::log::info("LoadCachedFormIDs: complete.");
}

void BanditSpawner::SaveCachedFormIDs() {
    CSimpleIniA ini;
    ini.SetUnicode();
    // Mevcut INI'yi oku (diğer section'ları bozmamak için)
    ini.LoadFile(Settings::INI_PATH);

    constexpr FactionType factions[] = {
        FactionType::Bandit, FactionType::Vampire, FactionType::Warlock,
        FactionType::Forsworn, FactionType::Draugr, FactionType::Animal,
        FactionType::Falmer, FactionType::Dwemer
    };

    for (auto f : factions) {
        const char* key = FactionToINIKey(f);
        if (!key) continue;

        auto& cache = GetCacheForFaction(f);
        if (cache.empty()) continue;

        // Mevcut kayıtlı ID'leri oku
        std::string existing;
        const char* exVal = ini.GetValue("CachedNPCs", key, nullptr);
        if (exVal) existing = exVal;

        // Mevcut ID'leri set'e ekle
        std::set<RE::FormID> known;
        {
            std::istringstream ss(existing);
            std::string tok;
            while (std::getline(ss, tok, ';')) {
                if (tok.empty()) continue;
                try { known.insert(static_cast<RE::FormID>(std::stoul(tok, nullptr, 16))); }
                catch (...) {}
            }
        }

        // Cache'deki yeni ID'leri ekle
        bool dirty = false;
        for (auto* obj : cache) {
            if (!obj) continue;
            RE::FormID fid = obj->GetFormID();
            if (fid >= 0xFF000000) continue; // runtime referans, kaydetme
            if (known.count(fid)) continue;  // zaten kayıtlı
            known.insert(fid);
            dirty = true;
        }

        if (!dirty) continue;

        // Yeni string oluştur
        std::string result;
        result.reserve(known.size() * 10);
        for (RE::FormID fid : known) {
            if (!result.empty()) result += ';';
            char buf[12];
            snprintf(buf, sizeof(buf), "%08X", fid);
            result += buf;
        }

        ini.SetValue("CachedNPCs", key, result.c_str(),
            "; Otomatik keşfedilen NPC FormID'leri - elle düzenleme YAPMAYIN");
    }

    SI_Error rc = ini.SaveFile(Settings::INI_PATH);
    if (rc == SI_OK) {
        SKSE::log::info("SaveCachedFormIDs: INI updated.");
    } else {
        SKSE::log::error("SaveCachedFormIDs: INI save FAILED (error={}).", static_cast<int>(rc));
    }
}


// ── Helper: Spawn a single actor ─────────────────────────────────
// XMarker YAKLAŞIMI KALDIRILDI (CRASH FIX):
// tempMarker->DeleteThis() çağrısı marker'ı hücre referans listesinden hemen kaldırmıyordu;
// "hayalet" referans olarak kalıyordu. UnreadBooksGlow, DeathDropOverhaul gibi modlar
// ForEachReferenceInRange sırasında bu yarı silinmiş referansa erişince null pointer
// (rax=0x0) dereference ile çöküyordu.
// Şimdi: doğrudan anchor üzerinde spawn et, ardından SetPosition ile konumla.
static RE::ObjectRefHandle SpawnSingleActor(RE::TESObjectREFR* anchor, FactionType faction, bool isBoss,
                                              float offsetX, float offsetY) {
    auto baseObj = GetFactionBaseObject(faction, isBoss);

    if (!baseObj) {
        SKSE::log::error("  SPAWN FAILED: No cached NPC found for faction={} boss={}",
                        static_cast<int>(faction), isBoss);
        return RE::ObjectRefHandle();
    }

    // Hedef pozisyon hesapla: anchor + XY offset, anchor'in Z'si
    RE::NiPoint3 targetPos = anchor->GetPosition();
    targetPos.x += offsetX;
    targetPos.y += offsetY;

    // Doğrudan anchor üzerinde spawn et (XMarker yok = hayalet referans yok)
    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (!spawned) {
        SKSE::log::error("  SPAWN FAILED: PlaceObjectAtMe returned null for '{}'", baseObj->GetName());
        return RE::ObjectRefHandle();
    }

    // Hedef konuma taşı (3D yokken de çalışır, oyun 3D yüklenince uygular)
    spawned->SetPosition(targetPos);

    auto handle = spawned->GetHandle();

    if (Settings::EnableLogging) {
        SKSE::log::info("  SPAWNED: '{}' (boss={}) offset=({:.0f},{:.0f}) refID=0x{:08X}",
                        baseObj->GetName(), isBoss, offsetX, offsetY, spawned->GetFormID());
    }

    // FixActorAI KALDIRILDI (CRASH FIX):
    // EvaluatePackage → DeathDropOverhaul crash, EnableAI → UnreadBooksGlow crash tetikliyordu.
    // PlaceObjectAtMe ile spawn edilen aktörler AI'larını ve düşmanlıklarını
    // Skyrim'in kendi sistemi aracılığıyla doğal olarak başlatır.

    return handle;
}



// ── Main spawn: Reinforcements ───────────────────────────────────
SpawnResult BanditSpawner::SpawnReinforcements(RE::TESObjectCELL* cell, FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    int count = GetSpawnCount();
    bool isInterior = cell ? cell->IsInteriorCell() : false;

    SKSE::log::info("=== SpawnReinforcements ===");
    SKSE::log::info("  Count={}, Faction={}, Interior={}, Cell=0x{:08X}", 
                    count, static_cast<int>(faction), isInterior, cell ? cell->GetFormID() : 0);

    // Cache'i spawn döngüsünden ÖNCE bir kez doldur (her spawn için ayrı ayrı taramak yerine)
    // Bu hem performansı artırır hem de hata ayıklamayı kolaylaştırır.
    if (cell) {
        UpdateCache(cell, faction);
        auto& cacheCheck = GetCacheForFaction(faction);
        if (Settings::EnableLogging) {
            SKSE::log::info("  Pre-spawn cache: Faction {} has {} cached NPC templates",
                static_cast<int>(faction), cacheCheck.size());
        }
        if (cacheCheck.empty()) {
            SKSE::log::error("  ABORTING: No NPC templates found for Faction {} in cell 0x{:08X}. "
                             "Are there any living enemies here?",
                             static_cast<int>(faction), cell->GetFormID());
            return result;
        }
    }

    auto& rng = GetRNG();

    // İç mekan: 200-500 birim (mağara içinde kalır)
    // Dış mekan: PatrolSpawnDistance kadar uzak (devriye etkisi)
    float minDist, maxDist;
    if (isInterior) {
        minDist = 200.0f;
        maxDist = 500.0f;
    } else {
        float base = Settings::PatrolSpawnDistance;
        minDist = base * 0.7f;
        maxDist = base * 1.3f;
    }

    // Boss spawn kontrolü: garantili + ekstra şans
    int pLevel = player->GetLevel();
    auto bossInfo = GetBossSpawnInfo(pLevel);
    int totalBossCount = bossInfo.guaranteed;
    if (bossInfo.extraChance > 0) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= bossInfo.extraChance) {
            totalBossCount++;
        }
    }
    if (totalBossCount > 0) {
        SKSE::log::info("  >> BOSS SPAWN: {} boss (guaranteed={}, extra_chance={}%, level={})",
            totalBossCount, bossInfo.guaranteed, bossInfo.extraChance, pLevel);
    }

    // Anchor'lari topla: Hucrede halihazirda var olan dusmanlari konum referansi olarak kullan.
    // NOT: Anchor sadece KONUM icin kullanilir. Cache'deki NPC template'i spawn icin kullanilir.
    // Bu nedenle anchor arama kosullari UpdateCache ile ayni olmalı.
    std::vector<RE::TESObjectREFR*> anchors;
    if (cell) {
        for (auto& ref : cell->GetRuntimeData().references) {
            if (!ref) continue;
            auto actor = ref->As<RE::Actor>();
            if (!actor || actor == player) continue;
            if (actor->IsDead()) continue; // Olü aktörler konum referansı için işe yaramaz
            
            auto baseObj = actor->GetActorBase();
            if (!baseObj) continue;

            bool isCorrectFaction = false;
            actor->VisitFactions([&](RE::TESFaction* f, int8_t rank) {
                if (f) {
                    RE::FormID fID = f->GetFormID();
                    if (faction == FactionType::Bandit && fID == 0x0001BCC0) isCorrectFaction = true;
                    else if (faction == FactionType::Vampire && fID == 0x00027242) isCorrectFaction = true;
                    else if (faction == FactionType::Forsworn && fID == 0x00043599) isCorrectFaction = true;
                    else if (faction == FactionType::Warlock && fID == 0x00030C66) isCorrectFaction = true;
                    else if (faction == FactionType::Draugr && fID == 0x0002430D) isCorrectFaction = true;
                    else if (faction == FactionType::Animal && (fID == 0x0001CB62 || fID == 0x00028FDF)) isCorrectFaction = true;
                    else if (faction == FactionType::Falmer && fID == 0x0002446A) isCorrectFaction = true;
                    else if (faction == FactionType::Dwemer && fID == 0x0001BCC1) isCorrectFaction = true;
                    else if (fID == 0x0005C84E || fID == 0x00019809) { isCorrectFaction = false; return true; } // PlayerFaction -> reddet
                }
                return false;
            });
            
            // Geniş fallback: combatStyle kontrolü
            if (!isCorrectFaction) {
                bool isEssential = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kEssential);
                bool isProtected = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kProtected);
                bool hasCombatStyle = (baseObj->combatStyle != nullptr);
                bool isGuard = actor->IsGuard();
                if (hasCombatStyle && !isGuard && !isEssential && !isProtected) {
                    isCorrectFaction = true;
                }
            }

            if (isCorrectFaction) {
                anchors.push_back(actor);
            }
        }
    }

    if (Settings::EnableLogging) {
        SKSE::log::info("  Found {} existing valid anchors in the cell.", anchors.size());
    }


    for (int i = 0; i < count; ++i) {
        bool isBoss = (i < totalBossCount);

        RE::TESObjectREFR* spawnAnchor = player;
        float currentMinDist = minDist;
        float currentMaxDist = maxDist;

        // Eger hucrede halihazirda ayni tip dusman varsa, onlardan birinin yaninda dogur (cluster effect)
        if (!anchors.empty()) {
            std::uniform_int_distribution<> anchorDist(0, (int)anchors.size() - 1);
            spawnAnchor = anchors[anchorDist(rng)];
            // Düşmanın dibinde doğsun (50 ile 150 birim arası)
            currentMinDist = 50.0f;
            currentMaxDist = 150.0f;
        }

        std::uniform_real_distribution<float> angleDist(0.0f, static_cast<float>(2.0 * M_PI));
        std::uniform_real_distribution<float> radiusDist(currentMinDist, currentMaxDist);
        float angle = angleDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(spawnAnchor, faction, isBoss, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnReinforcements DONE: {}/{} spawned ===", result.count, count);

    return result;
}

// ── Ambush spawn ─────────────────────────────────────────────────
// isOutdoorCamp=false → zindan cikisi pususu (AmbushSpawnDistance ~500 birim, yakin)
// isOutdoorCamp=true  → acik dunya kamp terk pususu (AmbushCampSpawnDistance ~1500 birim, uzak)
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction, bool isOutdoorCamp) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    int pLevel = player->GetLevel();
    if (pLevel < 10) {
        SKSE::log::info("SpawnAmbush: SKIPPED (player level < 10)");
        return result;
    }

    auto& rng = GetRNG();

    std::uniform_int_distribution<> chanceDist(1, 100);
    int roll = chanceDist(rng);
    if (roll > Settings::AmbushChance) {
        SKSE::log::info("SpawnAmbush: SKIPPED (rolled {}%, threshold {}%)", roll, Settings::AmbushChance);
        return result;
    }

    int minC = (Settings::AmbushMinCount < 1) ? 1 : Settings::AmbushMinCount;
    int maxC = (Settings::AmbushMaxCount < minC) ? minC : Settings::AmbushMaxCount;
    std::uniform_int_distribution<> countDist(minC, maxC);
    int count = countDist(rng);

    // Boss kontrolü
    // Boss spawn kontrolü: garantili + ekstra şans
    auto bossInfo = GetBossSpawnInfo(pLevel);
    int totalBossCount = bossInfo.guaranteed;
    if (bossInfo.extraChance > 0) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= bossInfo.extraChance) {
            totalBossCount++;
        }
    }

    // Mesafeyi pusu tipine göre seç
    float ambushDist = isOutdoorCamp ? Settings::AmbushCampSpawnDistance : Settings::AmbushSpawnDistance;

    SKSE::log::info("=== SpawnAmbush ===");
    SKSE::log::info("  AMBUSH! count={}, faction={}, roll={}%, type={}, dist={:.0f}",
                   count, static_cast<int>(faction), roll, isOutdoorCamp ? "CAMP" : "DUNGEON", ambushDist);
    if (totalBossCount > 0) {
        SKSE::log::info("  >> BOSS: {} adet (guaranteed={}, extra_chance={}%, level={})",
            totalBossCount, bossInfo.guaranteed, bossInfo.extraChance, pLevel);
    }

    float playerAngle = player->GetAngleZ();

    for (int i = 0; i < count; ++i) {
        bool isBoss = (i < totalBossCount);

        // Oyuncunun önünde 120 derecelik bir yarım dairede yayıl
        std::uniform_real_distribution<float> arcDist(-1.05f, 1.05f);  // ±60 derece
        
        // Zindan çıkışları için çok daha yakın spawnla
        float minR = isOutdoorCamp ? ambushDist * 0.7f : 150.0f;
        float maxR = isOutdoorCamp ? ambushDist * 1.3f : 300.0f;
        
        std::uniform_real_distribution<float> radiusDist(minR, maxR);
        
        float angle = playerAngle + arcDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, faction, isBoss, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnAmbush DONE: {}/{} spawned ===", result.count, count);

    return result;
}
