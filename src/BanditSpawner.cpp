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
            editorID.find("Draugr") != std::string::npos ||
            editorID.find("Skeleton") != std::string::npos) 
        {
            SKSE::log::info("  Found LvlList: '{}' -> FormID: 0x{:08X}", editorID, form->GetFormID());
        }
    }
    SKSE::log::info("=== DUMP FINISHED ===");
}

// ── Standardized tracking ID helper ──────────────────────────────
RE::FormID BanditSpawner::GetTrackingID(RE::TESObjectCELL* cell, RE::BGSLocation* loc) {
    if (loc && loc->GetFormID() != 0) {
        return loc->GetFormID();
    }
    if (cell) {
        return cell->GetFormID();
    }
    return 0;
}

// ── Common Faction Classification Logic ──────────────────────────
FactionType BanditSpawner::DetectActorFaction(RE::Actor* actor) {
    if (!actor) return FactionType::Unknown;

    FactionType detected = FactionType::Unknown;

    actor->VisitFactions([&](RE::TESFaction* f, int8_t) {
        if (f) {
            RE::FormID fID = f->GetFormID();
            // Friendly faction check
            if (fID == 0x0005C84E || fID == 0x00019809) {
                detected = FactionType::Unknown;
                return true; // Stop visiting
            }

            if (fID == 0x0001BCC0) detected = FactionType::Bandit;
            else if (fID == 0x00027242) detected = FactionType::Vampire;
            else if (fID == 0x00043599) detected = FactionType::Forsworn;
            else if (fID == 0x00030C66 || fID == 0x0004359A) detected = FactionType::Warlock;
            else if (fID == 0x0002430D || fID == 0x000B974F || fID == 0x0001BCC1 || fID == 0x000E6E16 || fID == 0x00050C17) detected = FactionType::Draugr;
            else if (fID == 0x0002997E || fID == 0x0003AD57) detected = FactionType::Falmer;
            else if (fID == 0x00043598) detected = FactionType::Dwemer;
            else if (fID == 0x0001CB61 || fID == 0x0001CB62 || fID == 0x00023E73 || fID == 0x00028FDF || 
                     fID == 0x0002C6C8 || fID == 0x00043596 || fID == 0x00043597 || fID == 0x00043594 || fID == 0x00043595) {
                detected = FactionType::Animal;
            }

            if (detected != FactionType::Unknown) {
                return true; // Found matching faction
            }
        }
        return false;
    });

    if (detected == FactionType::Unknown) {
        auto baseObj = actor->GetActorBase();
        if (baseObj) {
            std::string actorName = baseObj->GetName();
            const char* eidStr = baseObj->GetFormEditorID();
            std::string actorEid = eidStr ? eidStr : "";
            std::string raceEid = (baseObj->race && baseObj->race->GetFormEditorID()) ? baseObj->race->GetFormEditorID() : "";

            auto toLower = [](std::string s) {
                for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return s;
            };

            std::string lowerName = toLower(actorName);
            std::string lowerEid = toLower(actorEid);
            std::string lowerRace = toLower(raceEid);

            if (lowerName.find("skeleton") != std::string::npos || lowerEid.find("skeleton") != std::string::npos || lowerRace.find("skeleton") != std::string::npos ||
                lowerName.find("draugr") != std::string::npos || lowerEid.find("draugr") != std::string::npos || lowerRace.find("draugr") != std::string::npos) {
                detected = FactionType::Draugr;
            }
        }
    }

    return detected;
}

bool BanditSpawner::ClassifyActorFaction(RE::Actor* actor, FactionType targetFaction, bool& outIsKnownOtherFaction) {
    outIsKnownOtherFaction = false;
    if (!actor || targetFaction == FactionType::Unknown) return false;

    FactionType detected = DetectActorFaction(actor);

    if (detected != FactionType::Unknown) {
        if (detected == targetFaction) {
            return true;
        } else {
            outIsKnownOtherFaction = true;
            return false;
        }
    }

    return false;
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

// ── Elite (Boss) Cache ────────────────────────────────────────────
std::vector<RE::TESBoundObject*>& BanditSpawner::GetBossCacheForFaction(FactionType faction) {
    static std::vector<RE::TESBoundObject*> cacheBossBandit;
    static std::vector<RE::TESBoundObject*> cacheBossVampire;
    static std::vector<RE::TESBoundObject*> cacheBossWarlock;
    static std::vector<RE::TESBoundObject*> cacheBossForsworn;
    static std::vector<RE::TESBoundObject*> cacheBossDraugr;
    static std::vector<RE::TESBoundObject*> cacheBossAnimal;
    static std::vector<RE::TESBoundObject*> cacheBossFalmer;
    static std::vector<RE::TESBoundObject*> cacheBossDwemer;
    static std::vector<RE::TESBoundObject*> cacheBossUnknown;

    switch (faction) {
        case FactionType::Bandit:   return cacheBossBandit;
        case FactionType::Vampire:  return cacheBossVampire;
        case FactionType::Warlock:  return cacheBossWarlock;
        case FactionType::Forsworn: return cacheBossForsworn;
        case FactionType::Draugr:   return cacheBossDraugr;
        case FactionType::Animal:   return cacheBossAnimal;
        case FactionType::Falmer:   return cacheBossFalmer;
        case FactionType::Dwemer:   return cacheBossDwemer;
        default:                    return cacheBossUnknown;
    }
}

// ── Elite NPC tespiti ─────────────────────────────────────────────
// Kural A: NPC adında elit anahtar kelime varsa → elit
// Kural B: NPC taban seviyesi oyuncu seviyesine göre eşik değerinin üzerindeyse → elit
bool BanditSpawner::IsEliteNPC(RE::Actor* /*actor*/, RE::TESNPC* npc) {
    if (!npc) return false;

    // ── Kural A: İsim tabanlı ──
    auto toLower = [](std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };

    std::string name = toLower(npc->GetName() ? npc->GetName() : "");
    const char* eidRaw = npc->GetFormEditorID();
    std::string eid = toLower(eidRaw ? eidRaw : "");

    // Madenci, işçi, sivil veya esirler ASLA Boss/Elit olamaz
    if (name.find("miner") != std::string::npos || eid.find("miner") != std::string::npos ||
        name.find("worker") != std::string::npos || eid.find("worker") != std::string::npos ||
        name.find("captive") != std::string::npos || eid.find("captive") != std::string::npos ||
        name.find("prisoner") != std::string::npos || eid.find("prisoner") != std::string::npos ||
        name.find("civilian") != std::string::npos || eid.find("civilian") != std::string::npos) {
        return false;
    }

    // Elite anahtar kelimeler (vanilla + OBIS/Lawless uyumlu)
    static const std::vector<std::string> eliteKeywords = {
        "chief", "overlord", "wight lord", "master",
        "hagraven", "vampire lord", "elder", "lich",
        "shaman", "champion", "commander", "boss",
        "warlock", "necromancer", "summoner", "witch"
    };

    for (const auto& kw : eliteKeywords) {
        if (name.find(kw) != std::string::npos || eid.find(kw) != std::string::npos) {
            return true;
        }
    }

    // ── Kural B: Seviye tabanlı (sabit taban seviye, ölçeklenmiş değil) ──
    // actorData.level: TESNPC'nin xEdit'teki sabit "Level" alanı (CalcMin gibi değil)
    int npcBaseLevel = static_cast<int>(npc->actorData.level);
    if (npcBaseLevel <= 0) return false; // Leveled (0) NPC'ler için atla

    auto player = RE::PlayerCharacter::GetSingleton();
    int pLevel = player ? player->GetLevel() : 1;

    int eliteThreshold = 15; // Lvl < 25 için
    if (pLevel >= 50)      eliteThreshold = 35;
    else if (pLevel >= 25) eliteThreshold = 25;

    return (npcBaseLevel >= eliteThreshold);
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
        
        if (baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kUnique)) {
            if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Unique actor");
            continue;
        }
        
        RE::TESBoundObject* effectiveBase = baseObj;
        while (effectiveBase && effectiveBase->GetFormID() >= 0xFF000000) {
            auto actorBaseData = effectiveBase->As<RE::TESActorBaseData>();
            if (actorBaseData && actorBaseData->baseTemplateForm) {
                effectiveBase = actorBaseData->baseTemplateForm->As<RE::TESBoundObject>();
            } else {
                break;
            }
        }

        if (!effectiveBase || effectiveBase->GetFormID() >= 0xFF000000) {
            if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Runtime form (0xFF...) with no valid static template");
            continue;
        }

        // Çözümlenmiş template'in de unique olmadığını kontrol et
        // (Örn: 0xFF Stalleo -> 0x00... Stalleo gibi özel NPC'lere çözümleniyor olabilir)
        {
            auto effectiveNPC = effectiveBase->As<RE::TESNPC>();
            if (effectiveNPC && effectiveNPC->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kUnique)) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Resolved template is a Unique actor");
                continue;
            }
        }
        
        // Faction kontrolü yapalım
        bool isKnownOtherFaction = false;
        bool isCorrectFaction = ClassifyActorFaction(actor, faction, isKnownOtherFaction);

        // NOT: IsHostileToActor kontrolü KALDIRILDI.
        // Draugr/iskelet gibi NPC'ler uyku animasyonunda ya da henüz detect edilmemiş olabilir.
        // Ölü NPC'ler de geçerli klon şablonudur (FormID sabittir, canlılık durumu önemsizdir).
        // Yanlış faction karışımını önlemek için isKnownOtherFaction + isGuard + isEssential yeterli.

        bool isAnimalKw = baseObj->HasKeywordString("ActorTypeAnimal");
        bool isCreatureKw = baseObj->HasKeywordString("ActorTypeCreature") || baseObj->HasKeywordString("ActorTypeMonster") || baseObj->HasKeywordString("ActorTypeDragon");
        bool isUndeadKw = baseObj->HasKeywordString("ActorTypeUndead");
        bool isDwarvenKw = baseObj->HasKeywordString("ActorTypeDwarven");

        if (baseObj->race) {
            if (!isAnimalKw) isAnimalKw = baseObj->race->HasKeywordString("ActorTypeAnimal");
            if (!isCreatureKw) isCreatureKw = baseObj->race->HasKeywordString("ActorTypeCreature") || baseObj->race->HasKeywordString("ActorTypeMonster") || baseObj->race->HasKeywordString("ActorTypeDragon");
            if (!isUndeadKw) isUndeadKw = baseObj->race->HasKeywordString("ActorTypeUndead");
            if (!isDwarvenKw) isDwarvenKw = baseObj->race->HasKeywordString("ActorTypeDwarven");
        }

        if (faction == FactionType::Bandit || faction == FactionType::Warlock || faction == FactionType::Forsworn) {
            if (isAnimalKw || isCreatureKw || isDwarvenKw || isUndeadKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Humanoid, found Creature/Undead/Dwarven.");
                continue;
            }
        } else if (faction == FactionType::Vampire) {
            if (isAnimalKw || isCreatureKw || isDwarvenKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Vampire (Humanoid/Undead), found Animal/Creature/Dwarven.");
                continue;
            }
        } else if (faction == FactionType::Animal) {
            if (!isAnimalKw && !isCreatureKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Animal/Creature, found non-Animal.");
                continue;
            }
            if (isUndeadKw || isDwarvenKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Animal/Creature, found Undead/Dwarven.");
                continue;
            }
        } else if (faction == FactionType::Draugr) {
            if (isAnimalKw || isDwarvenKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Draugr, found Animal/Dwarven.");
                continue;
            }
        } else if (faction == FactionType::Dwemer) {
            if (isAnimalKw || isUndeadKw) {
                if (Settings::EnableLogging) SKSE::log::info("      -> SKIPPED: Expected Dwemer, found Animal/Undead.");
                continue;
            }
        }

        // GENIŞLETILMIŞ FALLBACK: Faction ID bulunamazsa combatStyle kontrolü kullan.
        // isKnownOtherFaction=true ise farklı bir bilinen gruba ait → kesinlikle reddet.
        // DİKKAT: SRC (Skyrim Realistic Conquering) veya diğer modlar tarafından eklenen dost/tarafsız
        // madenciler ("Embershard Miner" vb.), işçiler veya esirler bir combatStyle'a sahip olabilirler
        // ama haydut/düşman DEĞİLLERDİR. Bunların bandit/düşman olarak klonlanmasını engelliyoruz!
        if (!isCorrectFaction) {
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

            // İsim / EditorID filtresi: Miner, worker, captive, civilian, citizen gibi sivil/dost NPC'leri engelle
            auto toLower = [](std::string s) {
                for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return s;
            };
            std::string aName = toLower(baseObj->GetName() ? baseObj->GetName() : "");
            const char* eidRaw = baseObj->GetFormEditorID();
            std::string aEid = toLower(eidRaw ? eidRaw : "");

            bool isCivilianOrMiner = (aName.find("miner") != std::string::npos || aEid.find("miner") != std::string::npos ||
                                      aName.find("worker") != std::string::npos || aEid.find("worker") != std::string::npos ||
                                      aName.find("captive") != std::string::npos || aEid.find("captive") != std::string::npos ||
                                      aName.find("prisoner") != std::string::npos || aEid.find("prisoner") != std::string::npos ||
                                      aName.find("citizen") != std::string::npos || aEid.find("citizen") != std::string::npos ||
                                      aName.find("civilian") != std::string::npos || aEid.find("civilian") != std::string::npos);

            // Oyuncuya düşman mı kontrolü: Eğer oyuncuya düşman değilse ve ismi sivil/miner ise ASLA fallback kabul etme
            bool isHostile = actor->IsHostileToActor(player);

            bool isHumanoidFaction = (faction == FactionType::Bandit || faction == FactionType::Warlock ||
                                      faction == FactionType::Forsworn || faction == FactionType::Vampire);

            bool allowFallback = hasCombatStyle && !isGuard && !isProtected && !isCivilianOrMiner;
            if (isHumanoidFaction) {
                allowFallback = allowFallback && !isEssential;
                // Humanoid factionlarda ya isHostile olmalı ya da ismi/eid'si belirgin şekilde haydut/düşman olmalı
                if (!isHostile) {
                    bool hasHostileKw = (aName.find("bandit") != std::string::npos || aEid.find("bandit") != std::string::npos ||
                                         aName.find("outlaw") != std::string::npos || aEid.find("outlaw") != std::string::npos ||
                                         aName.find("thug") != std::string::npos || aEid.find("thug") != std::string::npos ||
                                         aName.find("highwayman") != std::string::npos || aEid.find("highwayman") != std::string::npos ||
                                         aName.find("marauder") != std::string::npos || aEid.find("marauder") != std::string::npos ||
                                         aName.find("warlock") != std::string::npos || aEid.find("warlock") != std::string::npos ||
                                         aName.find("necromancer") != std::string::npos || aEid.find("necromancer") != std::string::npos ||
                                         aName.find("forsworn") != std::string::npos || aEid.find("forsworn") != std::string::npos ||
                                         aName.find("vampire") != std::string::npos || aEid.find("vampire") != std::string::npos);
                    if (!hasHostileKw) {
                        allowFallback = false;
                    }
                }
            }

            if (allowFallback) {
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
                if (cachedBase == effectiveBase) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                cache.push_back(effectiveBase);
                added = true;
                if (Settings::EnableLogging) {
                    SKSE::log::info("      -> CACHED TEMPLATE: '{}' (FormID: 0x{:08X})",
                        effectiveBase->GetName(), effectiveBase->GetFormID());
                }
                if (cache.size() > 50) { // Keep bounded
                    cache.erase(cache.begin());
                }
            }

            // ── Elite / Boss Cache kontrolü ──
            // Gerçek TESNPC template'ini al (effectiveBase zaten çözümlenmiş)
            auto* effectiveNPC = effectiveBase->As<RE::TESNPC>();
            if (effectiveNPC && IsEliteNPC(actor, effectiveNPC)) {
                auto& bossCache = GetBossCacheForFaction(faction);
                bool bossExists = false;
                for (auto* cached : bossCache) {
                    if (cached == effectiveBase) { bossExists = true; break; }
                }
                if (!bossExists) {
                    bossCache.push_back(effectiveBase);
                    if (Settings::EnableLogging) {
                        SKSE::log::info("      -> BOSS CACHED TEMPLATE: '{}' (FormID: 0x{:08X})",
                            effectiveBase->GetName(), effectiveBase->GetFormID());
                    }
                    if (bossCache.size() > 30) {
                        bossCache.erase(bossCache.begin());
                    }
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

// ── Spawn count based on player level + density ───────────────────
int BanditSpawner::GetSpawnCount() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 0;

    int pLevel = player->GetLevel();
    // Seviye 1'den spawn özelliği: kapalıysa Lvl 1-9 hiç spawn yapmaz.
    // Açıksa Lvl 1-9 arası Lvl 10-24 ile aynı maxSpawns kullanır.
    if (pLevel < 10 && !Settings::SpawnFromLevel1) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  GetSpawnCount: Player level {} < 10 and SpawnFromLevel1=false, no spawns.", pLevel);
        }
        return 0; // Lvl 1-9 hic spawn yok (varsayilan)
    }

    int maxSpawns = Settings::MaxSpawnsLevel10_24;
    if (pLevel >= 50)      maxSpawns = Settings::MaxSpawnsLevel50Plus;
    else if (pLevel >= 25) maxSpawns = Settings::MaxSpawnsLevel25_49;

    if (maxSpawns < 1) maxSpawns = 1;

    // ── SpawnDensity uygulaması ──
    // %100 → her zaman maksimum (sabit, rastgele yok)
    // %x   → random(1, max(1, round(maxSpawns * x / 100)))
    // %0   → 0 spawn
    int density = Settings::SpawnDensity;
    if (density <= 0) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  GetSpawnCount: SpawnDensity=0, no spawns.");
        }
        return 0;
    }

    if (density >= 100) {
        // Tam yoğunluk: her zaman maksimum
        if (Settings::EnableLogging) {
            SKSE::log::info("  GetSpawnCount: SpawnDensity=100%, spawning max={}", maxSpawns);
        }
        return maxSpawns;
    }

    // Kısmi yoğunluk: üst limiti density ile ölçekle, rastgeleliği koru
    int scaledMax = static_cast<int>(std::round(maxSpawns * density / 100.0f));
    if (scaledMax < 1) scaledMax = 1;

    std::uniform_int_distribution<> dist(1, scaledMax);
    int result = dist(GetRNG());

    if (Settings::EnableLogging) {
        SKSE::log::info("  GetSpawnCount: density={}%, maxSpawns={}, scaledMax={}, result={}",
            density, maxSpawns, scaledMax, result);
    }
    return result;
}

struct BossSpawnInfo {
    int guaranteed; // Garanti boss sayisi
    int extraChance; // Ek boss icin yuzde sans (0-100)
};

BossSpawnInfo GetBossSpawnInfo(int pLevel) {
    // SpawnFromLevel1 açıksa Lvl 1-9 arası Lvl 10-24 boss şansını kullanır
    if (pLevel < 10 && !Settings::SpawnFromLevel1) return {0, 0};
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
    // ── Boss spawn için önce elit cache'e bak ──
    if (isBoss) {
        auto& bossCache = BanditSpawner::GetBossCacheForFaction(faction);

        // Boss cache boşsa mevcut hücreyi tekrar tara (yeni elit NPC keşfedilmiş olabilir)
        if (bossCache.empty()) {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player && player->GetParentCell()) {
                BanditSpawner::UpdateCache(player->GetParentCell(), faction);
            }
        }

        if (!bossCache.empty()) {
            std::uniform_int_distribution<> dist(0, (int)bossCache.size() - 1);
            auto* cachedBoss = bossCache[dist(BanditSpawner::GetRNG())];
            if (cachedBoss) {
                SKSE::log::info("  BOSS DYNAMIC COPY: Using elite NPC '{}' (Faction {} | BossCache size: {})",
                    cachedBoss->GetName(), static_cast<int>(faction), bossCache.size());
                return cachedBoss;
            }
        }

        // Boss cache boşsa normal cache'e düş (fallback)
        SKSE::log::warn("  BOSS FALLBACK: No elite NPCs cached for Faction {}, using normal cache.", static_cast<int>(faction));
    }

    // Normal (non-boss) seçim veya boss fallback
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

FactionType BanditSpawner::ScanCellForDominantFaction(RE::TESObjectCELL* cell) {
    if (!cell) return FactionType::Unknown;
    
    std::unordered_map<FactionType, int> counts;
    auto player = RE::PlayerCharacter::GetSingleton();
    
    for (auto& ref : cell->GetRuntimeData().references) {
        if (!ref) continue;
        auto actor = ref->As<RE::Actor>();
        if (!actor || actor == player) continue;
        
        auto baseObj = actor->GetActorBase();
        if (!baseObj) continue;
        
        FactionType foundFaction = DetectActorFaction(actor);
        
        // Sadece Settings'te açık olan factionları say
        if (foundFaction == FactionType::Bandit && !Settings::EnableBandits) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Vampire && !Settings::EnableVampires) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Forsworn && !Settings::EnableForsworn) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Warlock && !Settings::EnableWarlocks) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Draugr && !Settings::EnableDraugr) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Falmer && !Settings::EnableFalmer) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Dwemer && !Settings::EnableDwemer) foundFaction = FactionType::Unknown;
        if (foundFaction == FactionType::Animal && !Settings::EnableAnimals) foundFaction = FactionType::Unknown;

        if (foundFaction != FactionType::Unknown) {
            counts[foundFaction]++;
        }
    }
    
    FactionType bestFaction = FactionType::Unknown;
    int maxCount = 0;
    for (auto& pair : counts) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            bestFaction = pair.first;
        }
    }
    
    return bestFaction;
}

FactionType BanditSpawner::DetermineFactionForCell(RE::TESObjectCELL* cell, RE::BGSLocation* loc) {
    if (loc) {
        // Şehir, kasaba veya yerleşim yeriyse KESİNLİKLE tarama yapma
        auto kwdCity = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeCity");
        auto kwdTown = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeTown");
        auto kwdSettlement = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeSettlement");
        auto kwdDwelling = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDwelling");
        auto kwdPlayerHouse = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypePlayerHouse");
        
        if (!kwdCity) kwdCity = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D7);
        if (!kwdTown) kwdTown = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D8);
        if (!kwdSettlement) kwdSettlement = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0F);
        if (!kwdDwelling) kwdDwelling = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CA7C);
        if (!kwdPlayerHouse) kwdPlayerHouse = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000A8DE4);

        if ((kwdCity && loc->HasKeyword(kwdCity)) ||
            (kwdTown && loc->HasKeyword(kwdTown)) ||
            (kwdSettlement && loc->HasKeyword(kwdSettlement)) ||
            (kwdDwelling && loc->HasKeyword(kwdDwelling)) ||
            (kwdPlayerHouse && loc->HasKeyword(kwdPlayerHouse))) {
            return FactionType::Unknown;
        }
    }

    FactionType dominant = ScanCellForDominantFaction(cell);
    if (dominant != FactionType::Unknown) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  DetermineFactionForCell: Found dominant faction {} in cell", static_cast<int>(dominant));
        }
        return dominant;
    }
    
    // Eğer hücrede düşman yoksa (giriş odası vb.), location'a geri dön
    return GetFactionFromLocation(loc);
}

FactionType BanditSpawner::GetFactionFromLocation(RE::BGSLocation* loc) {
    if (!loc) return FactionType::Unknown;

    // Şehir, kasaba veya yerleşim yeriyse KESİNLİKLE spawn atma
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
        auto npc = form->As<RE::TESNPC>();
        if (!npc) { ++skipped; continue; }
        if (npc->IsUnique()) { ++skipped; continue; }

        // Sivil / madenci / tutsak isim kontrolü (önceki oturumlarda yanlış kaydedilmiş olanları temizle)
        auto toLower = [](std::string s) {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        };
        std::string nName = toLower(npc->GetName() ? npc->GetName() : "");
        const char* nEidRaw = npc->GetFormEditorID();
        std::string nEid = toLower(nEidRaw ? nEidRaw : "");
        if (nName.find("miner") != std::string::npos || nEid.find("miner") != std::string::npos ||
            nName.find("worker") != std::string::npos || nEid.find("worker") != std::string::npos ||
            nName.find("captive") != std::string::npos || nEid.find("captive") != std::string::npos ||
            nName.find("prisoner") != std::string::npos || nEid.find("prisoner") != std::string::npos ||
            nName.find("civilian") != std::string::npos || nEid.find("civilian") != std::string::npos) {
            ++skipped;
            continue;
        }

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

// ── Boss NPC cache INI persistence ───────────────────────────────
void BanditSpawner::SaveCachedBossFormIDs() {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(Settings::INI_PATH);

    constexpr FactionType factions[] = {
        FactionType::Bandit, FactionType::Vampire, FactionType::Warlock,
        FactionType::Forsworn, FactionType::Draugr, FactionType::Animal,
        FactionType::Falmer, FactionType::Dwemer
    };

    for (auto f : factions) {
        const char* key = FactionToINIKey(f);
        if (!key) continue;

        auto& bossCache = GetBossCacheForFaction(f);
        if (bossCache.empty()) continue;

        // Mevcut kayıtlı ID'leri oku
        std::string existing;
        const char* exVal = ini.GetValue("CachedBossNPCs", key, nullptr);
        if (exVal) existing = exVal;

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

        bool dirty = false;
        for (auto* obj : bossCache) {
            if (!obj) continue;
            RE::FormID fid = obj->GetFormID();
            if (fid >= 0xFF000000) continue;
            if (known.count(fid)) continue;
            known.insert(fid);
            dirty = true;
        }

        if (!dirty) continue;

        std::string result;
        result.reserve(known.size() * 10);
        for (RE::FormID fid : known) {
            if (!result.empty()) result += ';';
            char buf[12];
            snprintf(buf, sizeof(buf), "%08X", fid);
            result += buf;
        }

        ini.SetValue("CachedBossNPCs", key, result.c_str(),
            "; Otomatik kesfedilen ELITE (boss) NPC FormID'leri - elle duzenleme YAPMAYIN");
    }

    SI_Error rc = ini.SaveFile(Settings::INI_PATH);
    if (rc == SI_OK) {
        SKSE::log::info("SaveCachedBossFormIDs: INI updated.");
    } else {
        SKSE::log::error("SaveCachedBossFormIDs: INI save FAILED (error={}).", static_cast<int>(rc));
    }
}

void BanditSpawner::LoadCachedBossFormIDs() {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(Settings::INI_PATH) != SI_OK) {
        SKSE::log::info("LoadCachedBossFormIDs: INI not found or unreadable, skipping.");
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
        const char* val = ini.GetValue("CachedBossNPCs", key, nullptr);
        if (!val || val[0] == '\0') continue;

        auto& bossCache = GetBossCacheForFaction(f);

        std::istringstream ss(val);
        std::string token;
        int added = 0, skipped = 0;
        while (std::getline(ss, token, ';')) {
            auto trim = [](std::string& s) {
                while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
                while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t')) s.pop_back();
            };
            trim(token);
            if (token.empty()) continue;

            RE::FormID fid = 0;
            try { fid = static_cast<RE::FormID>(std::stoul(token, nullptr, 16)); }
            catch (...) { ++skipped; continue; }

            if (fid >= 0xFF000000) { ++skipped; continue; }

            auto* form = RE::TESForm::LookupByID<RE::TESBoundObject>(fid);
            if (!form) { ++skipped; continue; }

            auto* npc = form->As<RE::TESNPC>();
            if (!npc || npc->IsUnique()) { ++skipped; continue; }

            // Sivil / madenci / tutsak isim kontrolü
            auto toLower = [](std::string s) {
                for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return s;
            };
            std::string nName = toLower(npc->GetName() ? npc->GetName() : "");
            const char* nEidRaw = npc->GetFormEditorID();
            std::string nEid = toLower(nEidRaw ? nEidRaw : "");
            if (nName.find("miner") != std::string::npos || nEid.find("miner") != std::string::npos ||
                nName.find("worker") != std::string::npos || nEid.find("worker") != std::string::npos ||
                nName.find("captive") != std::string::npos || nEid.find("captive") != std::string::npos ||
                nName.find("prisoner") != std::string::npos || nEid.find("prisoner") != std::string::npos ||
                nName.find("civilian") != std::string::npos || nEid.find("civilian") != std::string::npos) {
                ++skipped;
                continue;
            }

            auto it = std::find(bossCache.begin(), bossCache.end(), form);
            if (it != bossCache.end()) { ++skipped; continue; }

            bossCache.push_back(form);
            ++added;
        }

        if (Settings::EnableLogging && (added > 0 || skipped > 0)) {
            SKSE::log::info("  LoadCachedBossFormIDs [{}]: +{} loaded, {} skipped",
                key, added, skipped);
        }
    }
    SKSE::log::info("LoadCachedBossFormIDs: complete.");
}


// ── Helper: Spawn a single actor ─────────────────────────────────
// XMarker YAKLAŞIMI KALDIRILDI (CRASH FIX):
// tempMarker->DeleteThis() çağrısı marker'ı hücre referans listesinden hemen kaldırmıyordu;
// "hayalet" referans olarak kalıyordu. UnreadBooksGlow, DeathDropOverhaul gibi modlar
// ForEachReferenceInRange sırasında bu yarı silinmiş referansa erişince null pointer
// (rax=0x0) dereference ile çöküyordu.
// Şimdi: doğrudan anchor üzerinde spawn et, ardından SetPosition ile konumla.
// İÇ MEKANDA HAVOK RAYCAST YASAK — HDT-SMP ile race condition → SEH crash (catch ile yakalanamaz)
// Dış mekan için Havok zemin tespiti
static bool GetGroundHeight(RE::TESObjectCELL* cell, const RE::NiPoint3& pos, float& groundZ, bool& isWalkableSlope) {
    if (!cell) return false;
    // İç mekanda PickObject çağrısı HDT-SMP physics adımıyla çakışır → access violation → CRASH
    if (cell->IsInteriorCell()) return false;

    auto bhkWorld = cell->GetbhkWorld();
    if (!bhkWorld) return false;

    auto havokWorld = bhkWorld->GetWorld1();
    if (!havokWorld) return false;

    constexpr float scale = 0.0142875f;
    RE::bhkPickData pickData;
    RE::NiPoint3 start = pos;
    start.z += 500.0f;
    RE::NiPoint3 end = pos;
    end.z -= 1000.0f;

    pickData.rayInput.from = RE::hkVector4(start.x * scale, start.y * scale, start.z * scale, 0.0f);
    pickData.rayInput.to = RE::hkVector4(end.x * scale, end.y * scale, end.z * scale, 0.0f);

    bhkWorld->PickObject(pickData);
    if (pickData.rayOutput.HasHit()) {
        groundZ = start.z + (end.z - start.z) * pickData.rayOutput.hitFraction;
        isWalkableSlope = (pickData.rayOutput.normal.quad.m128_f32[2] >= 0.4f);
        return true;
    }
    return false;
}

// Dış mekan için tavan kontrolü (iç mekanda her zaman geçer)
static bool HasHeadroom(RE::TESObjectCELL* cell, const RE::NiPoint3& groundPos, float minHeadroom = 120.0f) {
    if (!cell) return true;
    // İç mekanda Havok raycast yasak — güvenli varsay
    if (cell->IsInteriorCell()) return true;

    auto bhkWorld = cell->GetbhkWorld();
    if (!bhkWorld || !bhkWorld->GetWorld1()) return true;

    constexpr float scale = 0.0142875f;
    RE::bhkPickData pickData;
    RE::NiPoint3 start = groundPos;
    start.z += 10.0f;
    RE::NiPoint3 end = groundPos;
    end.z += (minHeadroom + 10.0f);

    pickData.rayInput.from = RE::hkVector4(start.x * scale, start.y * scale, start.z * scale, 0.0f);
    pickData.rayInput.to = RE::hkVector4(end.x * scale, end.y * scale, end.z * scale, 0.0f);

    bhkWorld->PickObject(pickData);
    if (pickData.rayOutput.HasHit()) {
        if (pickData.rayOutput.hitFraction < 0.90f) {
            return false; // Düşük tavan veya kaya içi engeli
        }
    }
    return true;
}

// Dış mekan için yol hattı kontrolü (iç mekanda her zaman geçer)
static bool IsPathClear(RE::TESObjectCELL* cell, const RE::NiPoint3& fromPos, const RE::NiPoint3& toPos) {
    if (!cell) return true;
    // İç mekanda Havok raycast yasak — güvenli varsay
    if (cell->IsInteriorCell()) return true;

    auto bhkWorld = cell->GetbhkWorld();
    if (!bhkWorld || !bhkWorld->GetWorld1()) return true;

    constexpr float scale = 0.0142875f;
    RE::bhkPickData pickData;

    RE::NiPoint3 start = fromPos;
    start.z += 60.0f;
    RE::NiPoint3 end = toPos;
    end.z += 60.0f;

    pickData.rayInput.from = RE::hkVector4(start.x * scale, start.y * scale, start.z * scale, 0.0f);
    pickData.rayInput.to = RE::hkVector4(end.x * scale, end.y * scale, end.z * scale, 0.0f);

    bhkWorld->PickObject(pickData);
    if (pickData.rayOutput.HasHit()) {
        if (pickData.rayOutput.hitFraction < 0.90f) {
            return false; // Dağ kayası veya yapı duvarı engelliyor
        }
    }
    return true;
}

static RE::NiPoint3 FindValidSpawnPosition(RE::TESObjectREFR* anchor, float minDist, float maxDist,
                                          float baseAngle = -1.0f, float arcSpread = static_cast<float>(2.0 * M_PI)) {
    RE::NiPoint3 anchorPos = anchor->GetPosition();
    auto cell = anchor->GetParentCell();
    auto& rng = BanditSpawner::GetRNG();

    bool isInterior = cell ? cell->IsInteriorCell() : false;

    std::uniform_real_distribution<float> radiusDist(minDist, maxDist);
    std::uniform_real_distribution<float> angleDist(0.0f, static_cast<float>(2.0 * M_PI));

    // ── İÇ MEKAN: Havok raycast yok, anchor çevresine rastgele offset ──────
    if (isInterior) {
        // İç mekanlarda anchor NPC'nin tam üzerinde olduğu konumdur.
        // Hafif rastgele X/Y sapması; Z anchor ile aynı (NPC yerçekimiyle yere oturur).
        bool useArc = (baseAngle >= 0.0f);
        for (int attempt = 0; attempt < 8; ++attempt) {
            float angle;
            if (useArc) {
                std::uniform_real_distribution<float> arcDist(-arcSpread * 0.5f, arcSpread * 0.5f);
                angle = baseAngle + arcDist(rng);
            } else {
                angle = angleDist(rng);
            }
            float radius = radiusDist(rng);
            RE::NiPoint3 candidate = anchorPos;
            candidate.x += radius * std::cos(angle);
            candidate.y += radius * std::sin(angle);
            // Z olduğu gibi kalır; Havok simülasyonu NPC'yi zemine oturtacak
            return candidate;
        }
        // Yedek: anchor'ın tam üstü
        RE::NiPoint3 fallback = anchorPos;
        std::uniform_real_distribution<float> tiny(-20.0f, 20.0f);
        fallback.x += tiny(rng);
        fallback.y += tiny(rng);
        return fallback;
    }

    // ── DIŞ MEKAN: Havok raycast ile zemin + tavan + yol hattı kontrolü ────
    float maxZDiff = 250.0f;
    float requiredHeadroom = 130.0f;
    bool useArc = (baseAngle >= 0.0f);

    for (int attempt = 0; attempt < 15; ++attempt) {
        float angle;
        if (useArc) {
            std::uniform_real_distribution<float> arcDist(-arcSpread * 0.5f, arcSpread * 0.5f);
            angle = baseAngle + arcDist(rng);
        } else {
            angle = angleDist(rng);
        }
        float radius = radiusDist(rng);

        RE::NiPoint3 candidate = anchorPos;
        candidate.x += radius * std::cos(angle);
        candidate.y += radius * std::sin(angle);

        float groundZ = 0.0f;
        bool isWalkable = false;

        if (GetGroundHeight(cell, candidate, groundZ, isWalkable)) {
            float zDiff = std::abs(groundZ - anchorPos.z);
            if (isWalkable && zDiff <= maxZDiff) {
                candidate.z = groundZ + 10.0f;

                if (HasHeadroom(cell, candidate, requiredHeadroom)) {
                    if (IsPathClear(cell, anchorPos, candidate)) {
                        return candidate;
                    }
                }
            }
        }
    }

    // Dış mekan yedek: daha kısa yarıçapla tekrar dene
    for (int attempt = 0; attempt < 8; ++attempt) {
        float angle = angleDist(rng);
        float radius = 75.0f;
        RE::NiPoint3 candidate = anchorPos;
        candidate.x += radius * std::cos(angle);
        candidate.y += radius * std::sin(angle);

        float groundZ = 0.0f;
        bool isWalkable = false;
        if (GetGroundHeight(cell, candidate, groundZ, isWalkable)) {
            if (std::abs(groundZ - anchorPos.z) <= 150.0f) {
                candidate.z = groundZ + 10.0f;
                if (HasHeadroom(cell, candidate, requiredHeadroom) && IsPathClear(cell, anchorPos, candidate)) {
                    return candidate;
                }
            }
        }
    }

    // Son çare: anchor pozisyonunun hemen yanına koy
    RE::NiPoint3 fallback = anchorPos;
    std::uniform_real_distribution<float> offsetDist(-30.0f, 30.0f);
    fallback.x += offsetDist(rng);
    fallback.y += offsetDist(rng);
    return fallback;
}

// ── Helper: Spawn a single actor ─────────────────────────────────
static RE::ObjectRefHandle SpawnSingleActor(RE::TESObjectREFR* anchor, FactionType faction, bool isBoss,
                                              const RE::NiPoint3& targetPos) {
    auto baseObj = GetFactionBaseObject(faction, isBoss);

    if (!baseObj) {
        SKSE::log::error("  SPAWN FAILED: No cached NPC found for faction={} boss={}",
                        static_cast<int>(faction), isBoss);
        return RE::ObjectRefHandle();
    }

    // Doğrudan anchor üzerinde spawn et
    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (!spawned) {
        SKSE::log::error("  SPAWN FAILED: PlaceObjectAtMe returned null for '{}'", baseObj->GetName());
        return RE::ObjectRefHandle();
    }

    // Doğrulanmış zemin konumuna taşı
    spawned->SetPosition(targetPos);

    auto handle = spawned->GetHandle();

    if (Settings::EnableLogging) {
        SKSE::log::info("  SPAWNED: '{}' (boss={}) pos=({:.0f},{:.0f},{:.0f}) refID=0x{:08X}",
                        baseObj->GetName(), isBoss, targetPos.x, targetPos.y, targetPos.z, spawned->GetFormID());
    }

    // Bir sonraki karede (main thread) Draugr / Ambush / Sleep animasyon durumunu sıfırla ve savaşa sok
    auto task = SKSE::GetTaskInterface();
    if (task) {
        task->AddTask([handle]() {
            auto ref = handle.get();
            if (!ref) return;
            auto actor = ref->As<RE::Actor>();
            if (!actor || actor->IsDead()) return;

            // Draugr ve diğer sleeping/ambush durumundaki NPC'lerin animasyon kilitlerini kaldır (uyandır)
            actor->SetGraphVariableBool("bIsAmbushing", false);
            actor->SetGraphVariableBool("bInAmbushState", false);
            actor->SetGraphVariableBool("IsSleeping", false);
            actor->SetGraphVariableBool("bAmbush", false);
            actor->SetGraphVariableBool("bAmbushState", false);

            actor->EvaluatePackage(true, false);

            // ── SALDIRGANLIK DÜZELTMESİ ──
            // Draugr gibi orijinalinde "uykuda / tuzakla uyandırılan" NPC şablonlarının
            // Aggression değeri genelde Unaggressive/Aggressive Only If Provoked'tır.
            // Doğal algılama sistemini DEĞİŞTİRMEZ - sadece NPC bir kez düşman algıladığında
            // pasif beklemek yerine inisiyatif alıp saldırmasını sağlar.
            auto avOwner = actor->AsActorValueOwner();
            if (avOwner) {
                avOwner->SetActorValue(RE::ActorValue::kAggression, 2.0f); // 2 = Very Aggressive
            }
        });
    }

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

        // Performans limiti: Sadece BİZİM spawn ettiğimiz runtime aktörleri say
        // (refID >= 0xFF000000 → PlaceObjectAtMe ile oluşturulan runtime referansları)
        // Vanilla / mod NPC'lerini saymıyoruz; onlar bu limitten etkilenmemeli.
        int ourSpawnedInCell = 0;
        for (auto& ref : cell->GetRuntimeData().references) {
            if (!ref) continue;
            auto a = ref->As<RE::Actor>();
            if (a && !a->IsDead() && (a->GetFormID() & 0xFF000000) == 0xFF000000) {
                ourSpawnedInCell++;
            }
        }
        if (ourSpawnedInCell + count > Settings::MaxRuntimeActorsPerCell) {
            int adjusted = Settings::MaxRuntimeActorsPerCell - ourSpawnedInCell;
            if (adjusted < 0) adjusted = 0;
            SKSE::log::warn("  MaxRuntimeActorsPerCell limit reached (ourSpawned={}, max={}, desired={}). Adjusted count to {}",
                            ourSpawnedInCell, Settings::MaxRuntimeActorsPerCell, count, adjusted);
            count = adjusted;
            if (count <= 0) return result;
        }
    }

    auto& rng = GetRNG();

    float minDist, maxDist;
    if (isInterior) {
        minDist = 200.0f;
        maxDist = 500.0f;
    } else {
        float base = Settings::PatrolSpawnDistance;
        minDist = base * 0.7f;
        maxDist = base * 1.3f;
    }

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

    std::vector<RE::TESObjectREFR*> anchors;
    if (cell) {
        for (auto& ref : cell->GetRuntimeData().references) {
            if (!ref) continue;
            auto actor = ref->As<RE::Actor>();
            if (!actor || actor == player) continue;
            if (actor->IsDead()) continue;
            
            auto baseObj = actor->GetActorBase();
            if (!baseObj) continue;

            bool isKnownOther = false;
            bool isCorrectFaction = ClassifyActorFaction(actor, faction, isKnownOther);
            
            if (!isCorrectFaction && !isKnownOther) {
                bool isEssential = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kEssential);
                bool isProtected = baseObj->actorData.actorBaseFlags.any(RE::ACTOR_BASE_DATA::Flag::kProtected);
                bool hasCombatStyle = (baseObj->combatStyle != nullptr);
                bool isGuard = actor->IsGuard();
                bool isCreatureFaction = (faction == FactionType::Animal);

                auto toLower = [](std::string s) {
                    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return s;
                };
                std::string aName = toLower(baseObj->GetName() ? baseObj->GetName() : "");
                const char* eidRaw = baseObj->GetFormEditorID();
                std::string aEid = toLower(eidRaw ? eidRaw : "");
                bool isCivilianOrMiner = (aName.find("miner") != std::string::npos || aEid.find("miner") != std::string::npos ||
                                          aName.find("worker") != std::string::npos || aEid.find("worker") != std::string::npos ||
                                          aName.find("captive") != std::string::npos || aEid.find("captive") != std::string::npos ||
                                          aName.find("prisoner") != std::string::npos || aEid.find("prisoner") != std::string::npos);

                if (hasCombatStyle && !isGuard && (!isEssential || isCreatureFaction) && !isProtected && !isCivilianOrMiner) {
                    if (actor->IsHostileToActor(player)) {
                        isCorrectFaction = true;
                    }
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

        if (!anchors.empty()) {
            std::uniform_int_distribution<> anchorDist(0, (int)anchors.size() - 1);
            spawnAnchor = anchors[anchorDist(rng)];
            currentMinDist = 50.0f;
            currentMaxDist = 150.0f;
        }

        RE::NiPoint3 spawnPos = FindValidSpawnPosition(spawnAnchor, currentMinDist, currentMaxDist);

        auto handle = SpawnSingleActor(spawnAnchor, faction, isBoss, spawnPos);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnReinforcements DONE: {}/{} spawned ===", result.count, count);

    return result;
}

// ── Ambush spawn ─────────────────────────────────────────────────
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction, bool isOutdoorCamp) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    int pLevel = player->GetLevel();
    // SpawnFromLevel1 kapalıysa seviye 10 altında ambush yok
    if (pLevel < 10 && !Settings::SpawnFromLevel1) {
        SKSE::log::info("SpawnAmbush: SKIPPED (player level < 10, SpawnFromLevel1=false)");
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

    auto bossInfo = GetBossSpawnInfo(pLevel);
    int totalBossCount = bossInfo.guaranteed;
    if (bossInfo.extraChance > 0) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= bossInfo.extraChance) {
            totalBossCount++;
        }
    }

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

        float minR = isOutdoorCamp ? ambushDist * 0.7f : 150.0f;
        float maxR = isOutdoorCamp ? ambushDist * 1.3f : 300.0f;

        RE::NiPoint3 spawnPos = FindValidSpawnPosition(player, minR, maxR, playerAngle, 2.1f);

        auto handle = SpawnSingleActor(player, faction, isBoss, spawnPos);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnAmbush DONE: {}/{} spawned ===", result.count, count);

    return result;
}





