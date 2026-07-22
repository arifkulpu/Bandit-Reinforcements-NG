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

### Özellikler (v2.0.0 Yeni Güncellemeleri ile)

#### 🐺 Hatalı "Kurt Spawn" (Yanlış Hayvan) Düzeltmesi
Eskiden, mod bir kampa girdiğinizde düşmanları tararken Kurtları yanlışlıkla "Haydut" olarak sınıflandırabiliyordu. Artık geliştirilmiş "Irk ve Keyword Analizi" sayesinde sistem, canlıların tam olarak ne olduğunu çok daha zekice tespit ediyor. Kurtlar, Örümcekler ve Hayvanlar, asla insan düşmanların (Haydutlar vb.) yerine geçemez.

#### 🎯 Unique (Özel İsimli) Karakterlerin Kopyalanması Engellendi
Oyun içindeki görevlerle bağlantılı veya sadece 1 tane bulunması gereken "Özel İsimli (Unique)" karakterlerin klonlanması sorunu çözüldü. Sistem, artık bu karakterleri otomatik olarak atlayarak sadece jenerik düşmanları klonlar.

#### ⚔️ Çoklu Fraksiyon Çatışması (Kendi Arasında Savaşma) Düzeltildi
Bir kampta hem Haydutlar hem de Kurtlar bulunuyorsa, oyun motorunun kafa karışıklığı yaşayıp grupları birbirine saldırtması problemi çözüldü. Artık her fraksiyon kendi kimliğiyle, kendi dostlarına sadık olarak kusursuzca doğar.

#### 🏃 Dış Mekan / Kamp Kaçış Sistemi Geliştirildi
Bir bölgeden yeterince uzaklaştığınızda (Combat Stop Distance), yaratılan ekstra askerler savaşmayı bırakır ve kampı devriye gezmek üzere geri dönerler. Artık tüm harita boyunca sizi kovalayıp performans sorunu yaratmazlar.

#### 🏕️ İsimsiz Vahşi Doğa Kampları
Haritada resmi bir adı veya etiketi olmayan, ancak etrafta **en az 1 adet** düşmanın bulunduğu yıkık dökük alanlar da mod tarafından otomatik tespit edilir. Buralara girdiğinizde ekstra asker çağırılır, buralardan çıkarken de pusu yeme ihtimaliniz vardır!

#### 🔄 Dinamik Takviyeler Ölçeklendirmesi
Oyuncu seviyesine göre çıkacak maksimum düşman sayısı değişir (Seviye 10 altındayken fazladan düşman/pusu asla çıkmaz):
- Seviye 1–9: Ekstra düşman çıkmaz (Sıfır takviye).
- Seviye 10–24: 1 ile 5 arası takviye.
- Seviye 25–49: 1 ile 10 arası takviye.
- Seviye 50+: 1 ile 15 arası takviye.

#### ⚡ Performans & Kararlılık (Zero-Overhead & XMarker Spawning)
- Sadece oyuncu yeni bir hücreye girdiğinde kontrol yapan **Lazy Evaluation** sistemi ile sıfır FPS kaybı yaşatır.
- NPC doğma (spawning) işlemi görünmez "XMarker" referansları kullanılarak **Frame-Perfect** bir şekilde yapılır. Bu sayede oyunda havada asılı kalan (T-pose) veya görünmez NPC sorunları tamamen çözülmüştür.
- Dinamik Kopyalama belleği (RAM) üzerinde çalışır, diske hiçbir kayıt (`.txt`) atmaz. Böylece oyununuzun Save dosyası asla bozulmaz (0xFF referans çökmesi yaşanmaz) ve tamamen güvenlidir.

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
Mod, diske yazmayan güvenli bir **Dinamik Kopyalama Sistemi (Dynamic Cache)** ile çalışır. Bir mekana girdiğinizde içerideki düşmanları sessizce tarar ve onların sınıf özelliklerini hafızasına alır. Doğurduğu takviyeler bizzat zindandaki mevcut düşmanların birebir kopyaları olur!
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- Bu modlarla **otomatik olarak %100 uyumludur**. Takviyeler doğrudan bu modların kendine has kıyafetlerine/isimlerine sahip düşmanlar olarak doğar!
- Zindan içerisinde doğan takviyeler rastgele saçılmak yerine var olan diğer düşmanların hemen **dibinde/yanında** belirir ve doğdukları ortamdaki eşyalarla/sandalyelerle anında etkileşime girerler (Sandbox AI).

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

### Features (with v2.0.0 New Updates)

#### 🐺 "Wolves Spawning as Bandits" Fix
Previously, the mod could incorrectly identify Wolves as "Bandits" when scanning a camp. With the newly improved "Race and Keyword Analysis", the system now intelligently determines the exact type of creature. Wolves, Spiders, and other Animals will never be mistaken for humanoid enemies (Bandits, etc.) again.

#### 🎯 Unique Character Cloning Prevented
The issue of cloning "Unique" named characters (which are tied to quests or meant to exist only once) has been fixed. The system now automatically skips these characters and exclusively clones generic enemies.

#### ⚔️ Multi-Faction Conflict (Infighting) Resolved
Fixed the game engine confusion that occurred when both Bandits and Wolves existed in the same camp, causing them to fight each other. Now, each faction spawns with its proper identity, remaining loyal to its allies.

#### 🏃 Outdoor / Camp Escape System Improved
When you move far enough away from an area (Combat Stop Distance), the spawned extra soldiers will stop fighting and return to patrol the camp. They will no longer chase you across the entire map, saving performance.

#### 🏕️ Unnamed Wilderness Camps
Even if an area has no official location tag, if the mod detects **at least 1 hostile enemy** organically waiting in the wild, it dynamically treats it as an "Unnamed Camp", triggering both reinforcements and ambushes upon leaving!

#### 🔄 Dynamic Reinforcements
Maximum extra enemies strictly scale with player level (no spawns under level 10):
- Levels 1–9: No extra spawns (Zero reinforcements).
- Levels 10–24: 1 to 5 extra enemies.
- Levels 25–49: 1 to 10 extra enemies.
- Levels 50+: 1 to 15 extra enemies.

#### ⚡ Performance & Stability (Zero-Overhead & XMarker Spawning)
- The mod uses **Lazy Evaluation** logic only when the player enters a new cell. Zero FPS impact.
- Spawning relies on hidden "XMarkers" for **Frame-Perfect placement**, completely eradicating vanilla engine bugs like T-posing or invisible NPCs!
- Dynamic caching is done 100% in-memory without persistent disk I/O, guaranteeing that your save file remains corruption-free (eliminating 0xFF invalid form crashes).

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
The mod uses a completely safe in-memory **Dynamic Caching System**. When you enter a dungeon, it silently scans the existing enemies and memorizes them. The reinforcements it spawns are **perfect copies** of the enemies already present!
- **OBIS** (Organized Bandits In Skyrim)
- **Lawless** (A Bandit Overhaul)
- **SPID** (Spell Perk Item Distributor)
- It is **100% automatically compatible** with these mods out of the box! Reinforcements will spawn wearing the exact custom armors and names from your overhauls.
- Reinforcements spawned inside dungeons will cluster **directly next to existing enemies** and immediately engage with the environment (Sandbox AI) rather than spawning randomly at the entrance.

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
