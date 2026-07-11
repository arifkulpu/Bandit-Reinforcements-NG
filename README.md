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
- **Örümcekler ve Yırtıcı Hayvanlar** (Hayvan İnleri)
- **Falmerlar** (Falmer Kovanları)
- **Dwemer Makinaları** (Cüce Harabeleri)

Her fraksiyon INI dosyasından ayrı ayrı aktif/pasif yapılabilir. Kapatılan yaratık türleri için zindanlarda fazladan asker doğmaz.

#### ⚔️ Takviye Liderleri (Boss Spawns)
Gelen grupların arasında bir Lider (Boss) bulunması oyuncu seviyesine göre değişir:
- **Seviye 10–24:** %20 ihtimalle 1 Boss çıkar (garanti yok).
- **Seviye 25–49:** 1 Boss **garanti** doğar + %30 ihtimalle 2. Boss da çıkar.
- **Seviye 50+:** 2 Boss **garanti** doğar + %40 ihtimalle 3. Boss da çıkar.

#### 🏹 Pusu Mekaniği (Dungeon Ambushes)
Bir zindanı temizleyip dışarı çıktığınızda, kapıda sizi bekleyen bir **pusu/intikam grubu** doğabilir. Bu pusular zindandan dışarı adım attığınız anda doğrudan **çok yakınınızda** (150-300 birim) belirerek sürpriz yaparlar! Açık dünya kamplarından ayrılırken ise uzaktan koşarak gelirler. Pusu şansı INI dosyasından ayarlanabilir (varsayılan: %30).

#### 📊 Seviye Ölçeklendirmesi
Oyuncu seviyesine göre çıkacak maksimum düşman sayısı değişir (Seviye 10 altındayken fazladan düşman/pusu asla çıkmaz):
- Seviye 1–9: Ekstra düşman çıkmaz (Sıfır takviye).
- Seviye 10–24: 1 ile 5 arası takviye.
- Seviye 25–49: 1 ile 10 arası takviye.
- Seviye 50+: 1 ile 15 arası takviye.

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

#### 🤝 Mod Uyumluluğu ve "Dinamik Kopyalama"
Mod artık **Dinamik Kopyalama Sistemi (Dynamic Cache)** ile çalışıyor. Bir mekana girdiğinizde içerideki düşmanları sessizce tarar ve onların sınıf özelliklerini (okçu, büyücü vb.) hafızasına alır. Doğurduğu takviyeler bizzat zindandaki mevcut düşmanların birebir kopyaları olur!
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- Bu modlarla **otomatik olarak %100 uyumludur**. Takviyeler doğrudan bu modların kendine has kıyafetlerine/isimlerine sahip düşmanlar olarak doğar!
- Zindan içerisinde doğan takviyeler doğrudan diğer düşmanların hemen **dibinde/yanında** belirir (rastgele saçılmak yerine onlarla grup olurlar).

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
- **Spiders & Animals** (Animal Dens)
- **Falmer** (Falmer Hives)
- **Dwemer Automatons** (Dwarven Ruins)

Each faction can be individually enabled/disabled via the INI file. Disabled factions will not trigger extra spawns.

#### ⚔️ Boss Spawns
The chance and number of **Bosses** (Bandit Chief, Vampire Lord, etc.) in a group scales with player level:
- **Levels 10–24:** 20% chance for 1 Boss (no guarantee).
- **Levels 25–49:** 1 Boss **guaranteed** + 30% chance for a 2nd Boss.
- **Levels 50+:** 2 Bosses **guaranteed** + 40% chance for a 3rd Boss.

#### 🏹 Dungeon Ambushes
After clearing a dungeon and exiting, a **revenge/ambush group** may spawn at the entrance. Dungeon exit ambushes spawn **extremely close** to the player (150-300 units) for a surprise jump! Outdoor camp ambushes will still run towards you from a distance. Ambush chance is configurable (default: 30%).

#### 📊 Level Scaling
Maximum extra enemies strictly scale with player level (no spawns under level 10):
- Levels 1–9: No extra spawns (Zero reinforcements).
- Levels 10–24: 1 to 5 extra enemies.
- Levels 25–49: 1 to 10 extra enemies.
- Levels 50+: 1 to 15 extra enemies.

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

#### 🤝 Mod Compatibility & "Dynamic Cache"
The mod now uses a **Dynamic Caching System**. When you enter a dungeon, it silently scans the existing enemies (archers, mages, bruisers) and memorizes them. The reinforcements it spawns are **perfect copies** of the enemies already present in the dungeon!
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- It is **100% automatically compatible** with these mods out of the box! Reinforcements will spawn wearing the exact custom armors and names from your overhauls.
- Reinforcements spawned inside dungeons will cluster **directly next to existing enemies** rather than spawning randomly at the entrance.

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
