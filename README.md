# Bandit Reinforcements NG

[Türkçe](#türkçe) | [English](#english)

---

## Türkçe

### Modun Amacı
Bandit Reinforcements NG, Skyrim'deki haydut kamplarına, zindanlara ve düşman inlerine yaklaştığınızda rastgele sayıda ekstra düşman ekleyen dinamik bir SKSE eklentisidir. `CommonLibSSE-NG` kullanılarak geliştirildiği için **1.5.97 (SE), 1.6.1170 (AE) ve VR** sürümlerinin tümüyle tam uyumludur.

### Özellikler

#### 🗡️ Dinamik Takviye (Reinforcements)
Düşman kamplarına veya zindanlara giriş yaparken sistem tetiklenir ve ortama yeni düşmanlar eklenir. Düşmanlar doğrudan yanınızda belirmez; oyuncudan **~2000 birim** uzaklıkta (yaklaşık bir ok menzili) doğarak kampa doğru **devriye gezer gibi** yaklaşırlar.

#### 🎯 Çoklu Fraksiyon Desteği
Sadece haydutlarla sınırlı değil! Girdiğiniz mekanın türüne göre uygun düşman otomatik olarak seçilir:
- **Haydutlar** (Bandit Kampları)
- **Vampirler** (Vampir İnleri)
- **Büyücüler/Necromancer'lar** (Warlock Sığınakları)
- **Forsworn / Yeminliler** (Reach Bölgesi Kampları)
- **Draugr / İskeletler** (Draugr Mezar Odaları)

Her fraksiyon INI dosyasından ayrı ayrı aktif/pasif yapılabilir.

#### ⚔️ Takviye Liderleri (Boss Spawns)
Oyuncu belli bir seviyeyi geçtiğinde (varsayılan: Seviye 10), doğan gruptaki düşmanlardan birinin **Boss** (Haydut Şefi, Vampir Lordu vb.) olma şansı vardır. Boss doğma olasılığı INI dosyasından ayarlanabilir (varsayılan: %15).

#### 🏹 Pusu Mekaniği (Dungeon Ambushes)
Bir zindanı temizleyip dışarı çıktığınızda, kapıda sizi bekleyen bir **pusu/intikam grubu** doğabilir. Bu grup, zindandaki fraksiyon türüne uygun düşmanlardan oluşur ve oyuncunun bakış yönünde bir yarım daire içinde belirir. Pusu şansı INI dosyasından ayarlanabilir (varsayılan: %30).

#### 📊 Seviye Ölçeklendirmesi
Oyuncu seviyesine göre çıkacak maksimum düşman sayısı değişir:
- Seviye 1–10: Maksimum 3
- Seviye 10–25: Maksimum 6
- Seviye 25+: Maksimum 10

#### ⚡ Performans Dostu (Zero-Overhead)
Sistem arkada sürekli bir "Update" döngüsü çalıştırmaz. Sadece oyuncu yeni bir hücreye (Cell) girdiğinde durumu kontrol eder (**Lazy Evaluation**). Bu sayede FPS düşüşü yaşanmaz.

#### ⏱️ Bekleme Süresi (Cooldown)
- **Temizlenen kamplar:** 10 oyun içi gün boyunca yeni düşman eklenmez.
- **Kaçılan kamplar:** Yaratılan grup 3 gün sonra otomatik olarak temizlenir.

#### 🔧 INI Ayar Dosyası
Tüm ayarlar `BanditReinforcementsNG.ini` dosyasından özelleştirilebilir:
- Spawn sayıları ve mesafesi
- Boss doğma şansı ve minimum seviye
- Pusu şansı ve grup büyüklüğü
- Cooldown süreleri
- Fraksiyon açma/kapama

#### 🤝 Mod Uyumluluğu
Mod, oyunun orijinal (Vanilla) **Leveled List** sistemini kullandığı için aşağıdaki modlarla **otomatik olarak %100 uyumludur** — ekstra yama gerekmez:
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- Bu modlar Skyrim'in Leveled List'lerini değiştirdiğinde, Bandit Reinforcements NG otomatik olarak bu yeni düşman tiplerini doğurur.

### Kurulum
1. [SKSE](http://skse.silverlock.org/)'nin sürümünüze uygun versiyonunun kurulu olduğundan emin olun.
2. `BanditReinforcementsNG.dll` ve `BanditReinforcementsNG.ini` dosyalarını `Data/SKSE/Plugins/` dizinine kopyalayın.
3. Oyunu SKSE üzerinden başlatın.

### Gereksinimler
- Skyrim SE (1.5.97), AE (1.6.1170) veya VR
- [SKSE](http://skse.silverlock.org/) (sürümünüze uygun)

---

## English

### Purpose of the Mod
Bandit Reinforcements NG is a dynamic SKSE plugin that adds random extra enemies when you approach enemy camps, dungeons, and lairs in Skyrim. Built with `CommonLibSSE-NG`, it is fully compatible with **1.5.97 (SE), 1.6.1170 (AE), and VR** out of the box.

### Features

#### 🗡️ Dynamic Reinforcements
The system triggers when entering hostile locations, spawning new enemies. Instead of appearing right next to you, enemies spawn **~2000 units away** (roughly one bow range) and **patrol towards the camp** like real reinforcements.

#### 🎯 Multi-Faction Support
Not limited to bandits! The mod automatically selects the appropriate enemy type based on the location:
- **Bandits** (Bandit Camps)
- **Vampires** (Vampire Lairs)
- **Warlocks/Necromancers** (Warlock Dens)
- **Forsworn** (Reach Region Camps)
- **Draugr/Skeletons** (Draugr Crypts)

Each faction can be individually enabled/disabled via the INI file.

#### ⚔️ Boss Spawns
When the player reaches a certain level (default: Level 10), there is a chance one of the spawned enemies will be a **Boss** (Bandit Chief, Vampire Lord, etc.). Boss spawn probability is configurable via INI (default: 15%).

#### 🏹 Dungeon Ambushes
After clearing a dungeon and exiting, a **revenge/ambush group** may spawn at the entrance. This group matches the dungeon's faction type and appears in a semicircle facing the player. Ambush chance is configurable (default: 30%).

#### 📊 Level Scaling
Maximum extra enemies scale with player level:
- Levels 1–10: Max 3
- Levels 10–25: Max 6
- Levels 25+: Max 10

#### ⚡ Performance Friendly (Zero-Overhead)
The mod uses **Lazy Evaluation** — no constant background update loop. Logic only runs when the player enters a new cell. Zero FPS impact.

#### ⏱️ Cooldown System
- **Cleared Camps:** 10 in-game day cooldown before new enemies spawn.
- **Escaped Camps:** Spawned groups are automatically cleaned up after 3 in-game days.

#### 🔧 INI Configuration
All settings are customizable via `BanditReinforcementsNG.ini`:
- Spawn counts and patrol distance
- Boss spawn chance and minimum level
- Ambush chance and group size
- Cooldown durations
- Per-faction enable/disable toggles

#### 🤝 Mod Compatibility
The mod uses Skyrim's native (Vanilla) **Leveled List** system, making it **automatically 100% compatible** with the following mods — no patches needed:
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- When these mods modify Skyrim's Leveled Lists, Bandit Reinforcements NG will automatically spawn their custom enemy types.

### Installation
1. Ensure you have the appropriate version of [SKSE](http://skse.silverlock.org/) installed.
2. Copy `BanditReinforcementsNG.dll` and `BanditReinforcementsNG.ini` into your `Data/SKSE/Plugins/` folder.
3. Launch the game via SKSE.

### Requirements
- Skyrim SE (1.5.97), AE (1.6.1170), or VR
- [SKSE](http://skse.silverlock.org/) (matching your game version)

---

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır. See [LICENSE.md](LICENSE.md) for details.
