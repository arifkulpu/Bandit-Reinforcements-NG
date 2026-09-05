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

### V1.1 Güncelleme Notları

#### 🧠 Akıllı Çoğunluk Taraması (Dominant Faction Scanner)
Artık bir zindana (veya kampa) girdiğinizde, mod o zindanın etiketine körü körüne güvenmek yerine **içerideki düşmanları tek tek sayar**. Eğer içeride 5 Falmer, 2 Chaurus ve 1 Kurt bulursa, en yüksek sayıya sahip olan Falmer grubunu seçer. Üstelik oluşturulan yeni takviyeler, bu orijinal sayılara (3 Falmer, 2 Chaurus gibi) **orantılı olarak** dağıtılır!

#### 🛡️ Pusu Bekleme Süresi (Ambush Cooldown)
Bir zindandan veya kamptan çıkarken üst üste pusu yeme (gir-çık yaparak sürekli klonların belirmesi) sorunu çözüldü. Artık aynı mekandan her çıkışınızda pusu tetiklenmemesi için **gerçek zamanlı 30 saniyelik bir bekleme süresi (cooldown)** eklendi.

#### 💀 Ölü Bedenlerden Şablon Çıkarma
Eğer daha önceden temizlediğiniz bir mağaraya geri dönerseniz ve içerideki herkes ölmüşse, sistem artık hata vermez. Ölü düşmanların bedenlerini ve ırklarını analiz ederek onları şablon olarak kullanır ve zindana doğru yeni takviyeler çağırır!

#### ⚙️ Eksik Vanilla Şablonları Düzeltildi
Oyunda Falmer, Dwemer, Warlock gibi özel ırkların bazen şablonsuz kalarak doğamaması sorunu çözüldü. Mod artık tüm ana ırkların Vanilla (Orijinal Oyun) kodlarını otomatik olarak INI dosyasına kaydeder. Yabani bir kurdun yanlışlıkla Falmer olarak doğması gibi tuhaf hatalar tamamen giderildi.

#### ⚔️ Destek Kuvvetler (Backup Forces)
Zindan temizleme sürelerini bir hayatta kalma (survival) mekaniğine dönüştüren yeni bir sistem eklendi! Eğer bir zindanda fazla vakit geçirirseniz, sistem bunu "oyuncu zorlanıyor veya oyalanıyor" olarak algılar ve arkanızdan bir destek birliği gönderir. Düşmanlar geldiğinde ekranın sol üst köşesinde "Backup forces have arrived!" bildirimi belirir. Bu işlem INI dosyasından ayarlanan **Minimum ve Maksimum süreler arasında tamamen rastgele** bir sürede (Örn: 1 dk ile 10 dk arası) gerçekleşir. Ayrıca bir zindanda arka arkaya kaç dalga (wave) destek geleceğini de yine INI üzerinden ayarlayabilirsiniz.

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

### V2.3 Güncelleme Notları

#### 🎛️ SKSE Menu Framework v3 (Oyun İçi Ayar Menüsü) Desteği
Modun tüm ayarlarını artık oyun esnasında Duraklama (Pause) Menüsü üzerinden **SKSE Menu Framework** ("Bandit Reinforcements NG" bölümü) panelinden canlı olarak değiştirebilirsiniz! Yapılan tüm değişiklikler anında `BanditReinforcementsNG.ini` dosyasına otomatik kaydedilir. (Framework opsiyoneldir; kurulu değilse mod INI dosyasıyla sorunsuz çalışmaya devam eder).

#### 🏔️ Dağ İçi, Kaya İçi ve Dik Uçurum Sıkışma Engeli (Gelişmiş Fizik Taraması)
Açık dünyada veya dağlık alanlarda doğan düşmanların aşırı dik kayalıklara, dağ tepelerine veya içi boş dağ kütlelerinin/kayaların içine doğup sıkışması engellendi:
- Düşmanların ayakları tam olarak yürüdüğünüz zemine oturtulur.
- Dik kayalıklar ve tırmanılamaz dağ yamaçları (eğim > 50°) otomatik elenir.
- Düşmanın doğacağı yer ile sizin aranızda dağ kütlesi, dikey kayalık veya duvar varsa o nokta iptal edilir.
- İçi boş dağ maketlerinin içi veya kayaların arası otomatik tespit edilerek düşmanların harita dışına sıkışması tamamen önlenir.
- **İç Mekan (Zindan/Mağara) Uyumu:** Zindanlardaki alçak tünellerde ve basık maden galerilerinde tavan engeline takılmadan esnek ve sorunsuz çalışacak şekilde optimize edilmiştir.

#### 🎯 Özel (Unique) Karakterlerin Kopyalanması Tamamen Engellendi
Stalleo, Faendal, Farkas gibi oyundaki özel (şahsa münhasır) NPC'lerin görev esnasında yanlışlıkla haydut veya düşman kopyası olarak doğması engellendi.

#### 🏘️ Kasaba ve Köy Güvenli Bölge Koruması
Başka modlarla eklenenler de dahil olmak üzere şehirler, kasabalar, köyler ve oyuncu evleri güvenli bölge ilan edildi. Mod bu bölgelerde tamamen pasif kalır ve düşman doğurmaz.

#### 🏰 Hücre Bazlı Spawn Sadece İç Mekanlara Kısıtlandı
Periyodik hücre spawn sistemi yalnızca iç mekanlarda (zindanlar, mağaralar) çalışır. Dış mekanlarda zaten ayrı bir pusu (Ambush) sistemi bulunduğu için gereksiz düşman kalabalığı önlenmiştir.

### V2.4 Güncelleme Notları

#### 🎲 Destek Kuvvet Yüzde Şansı (Backup Forces Chance)
Destek kuvvet sisteminin her mağarada mutlaka çıkması yerine **% şans ile ayarlanabilmesi** sağlandı (Varsayılan %30). Hem `BanditReinforcementsNG.ini` üzerinden hem de oyun içi SKSE Menu Framework menüsünden (%0 - %100) canlı olarak ayarlanabilir.

#### 🧟 Draugr Hareketsiz / Donuk Bekleme Sorununa Çözüm
Doğan Draugr'ların veya mezar/ambush uyku pozisyonundaki yaratıkların siz saldırmadıkça hareketsiz durması problemi tamamen çözüldü. Spawn anında animasyon kilitleri (`bIsAmbushing`, `IsSleeping`) otomatik kaldırılır ve yapay zeka hemen uyandırılarak oyuncuya karşı aktif savaşa sokulur.

#### 🏛️ Mağara İçi Bölge Değişimlerinde Anında Tarama
Bir mağara veya zindan içinde kapılardan geçerek yeni alt alanlara (hücrelere) girdiğiniz an sistem o bölgedeki düşman tiplerini anında tarar ve klon havuzunu günceller.

#### 💀 İskelet (Skeleton) Desteği
Zindanlarda, mezarlıklarda ve büyücü/necromancer mağaralarında bulunan tüm İskelet türleri (`Skeleton`, `Skeleton Archer`, `Skeleton Mage` vb.) fraksiyon ve isim tanıma sistemine dahil edildi. Artık Draugr zindanlarında ve mezarlarda Draugr'ların yanı sıra İskeletler de takviye ve pusu olarak kopyalanıp doğacaktır.

### V2.5 Güncelleme Notları

#### 💥 Kritik Çökme Düzeltmesi — İç Mekanlarda Havok Raycast (Embershard Mine / HDT-SMP Crash)
İç mekanlarda (zindanlar, mağaralar, maden tünelleri) spawn pozisyonu hesaplanırken kullanılan Havok fizik raycast'inin (`bhkWorld::PickObject`), HDT-SMP fiziği ile aynı anda çalıştığında oyunu çöktürdüğü kritik hata tamamen çözüldü.
- **Sorun:** `bhkWorldM` üzerinde `PickObject` çağrısı, HDT-SMP fizik adımıyla aynı frame'de yürütüldüğünde Windows Structured Exception (erişim ihlali) üretiyordu. Bu tür çökmeler C++ `try-catch` ile yakalanamaz.
- **Çözüm:** İç mekanlarda Havok raycast **tamamen devre dışı** bırakıldı. Spawn konumları artık Havok'a dokunmadan, anchor pozisyonundan rastgele X/Y sapmasıyla belirleniyor. Oyun fiziği NPC'yi yerçekimiyle zemine oturtmaya devam ediyor.
- Dış mekanlar ise etkilenmedi; dağ/kaya sıkışma önleme sistemi dışarıda eskisi gibi çalışmaya devam eder.

#### 🔢 Aktör Sayısı Limiti Düzeltmesi — Vanilla NPC'ler Artık Sayılmıyor
`MaxRuntimeActorsPerCell` limiti artık hücredeki **TÜM** aktörleri değil, yalnızca **modumuzun spawn ettiği** runtime aktörlerini (FormID `0xFF000000+`) sayıyor.
- **Eski davranış:** Embershard Mine gibi kalabalık dungeon'larda 29 vanilla NPC varken limit 30'du → `%100` ayarında bile sadece 1 klon doğuyordu.
- **Yeni davranış:** Vanilla/diğer mod NPC'leri limite dahil değil. İstenilen sayıda klon sorunsuz doğar.

#### 🚫 Sivil ve Madenci NPC'lerin Klonlanması Engellendi
SRC (Skyrim Realistic Conquering) gibi mods tarafından dungeonlara eklenen dost/tarafsız `Embershard Miner`, `Embershard Mine Bailiff` gibi NPC'lerin yanlışlıkla haydut/boss olarak klonlanması tamamen önlendi.
- **Sorun:** CombatStyle fallback sistemi, faction ID'si olmayan ama savaş stili olan her NPC'yi düşman sayıyordu. Madenciler de bu kategoriye giriyordu.
- **Düzeltme:** İsim ve EditorID'sinde `miner`, `worker`, `captive`, `prisoner`, `civilian`, `citizen` geçen NPC'ler artık cache'e, boss cache'e ve anchor listesine **hiçbir zaman** alınmıyor. Ayrıca humanoid fraksiyon NPC'lerinin fallback ile kabul edilebilmesi için oyuncuya düşman olmaları ya da isimlerinde/EID'lerinde `bandit`, `outlaw`, `thug`, `highwayman` vb. anahtar kelimeler bulunması şartı getirildi.
- INI'ye önceki oturumlarda kaydedilmiş madenci/sivil FormID'leri de oyun başlangıcında filtrelenerek atılıyor.

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

### V1.1 Update Notes

#### 🧠 Dominant Faction Scanner
When you enter a dungeon (or camp), the mod no longer blindly relies on the location's tag. Instead, it **scans and counts every enemy inside**. If it finds 5 Falmer, 2 Chaurus, and 1 Wolf, it selects the Falmer group as the dominant faction. Furthermore, the newly spawned reinforcements are distributed **proportionally** (e.g., 3 Falmer, 2 Chaurus) to match the original ecosystem!

#### 🛡️ Ambush Cooldown System
Resolved the issue where quickly entering and exiting a dungeon would trigger endless, back-to-back ambushes. A **real-time 30-second cooldown** has been added to ambush events for the same location to prevent spawn spamming.

#### 💀 Dead NPCs as Templates
If you return to a previously cleared cave and everyone is dead, the mod no longer fails to spawn reinforcements. It now analyzes the corpses and races of the dead enemies, using them as valid templates to spawn new, identical reinforcements!

#### ⚙️ Missing Vanilla Templates Fixed
Fixed an issue where special races like Falmer, Dwemer, and Warlocks would sometimes fail to spawn due to missing templates. The mod now automatically injects the exact Vanilla (Base Game) FormIDs of all major factions into the INI file. Strange bugs, such as a wild wolf mistakenly spawning as a Falmer, have been completely eliminated.

#### ⚔️ Backup Forces System
A new survival mechanic has been added based on your dungeon clear times! If you spend too much time inside a dungeon, the system assumes you are struggling or taking too long and sends a backup force after you from behind. When they arrive, a "Backup forces have arrived!" notification will appear in the top-left corner. This system triggers at a **completely random time between a Minimum and Maximum duration** defined in your INI file (e.g., between 1 and 10 minutes). You can also configure exactly how many backup waves can hit you in a single dungeon run.

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

### V2.3 Update Notes

#### 🎛️ SKSE Menu Framework v3 (In-Game Menu) Support
All mod settings can now be configured live in-game through the Pause Menu via **SKSE Menu Framework** under the "Bandit Reinforcements NG" panel! Changes are automatically saved to `BanditReinforcementsNG.ini`. (Framework is optional; if not installed, the mod continues to run using INI settings).

#### 🏔️ Mountain Cavity, Rock Collision & Cliff Stuck Prevention (Advanced Physics Validation)
Fixed enemies spawning inside hollow mountain meshes, rock interiors, vertical cliff faces, or unreachable mountain peaks:
- Snaps enemy feet precisely to the walkable ground surface.
- Automatically rejects steep cliffs and non-walkable slopes (slope angle > 50°).
- Cancels spawn locations separated from you by mountain rock walls, cliffs, or solid structures.
- Prevents spawning inside hollow 3D mountain models or rock enclosures.
- **Dungeon & Cave Optimized:** Interior dungeons and low-ceiling mines are specially handled so enemies spawn smoothly without triggering artificial ceiling blocks.

#### 🎯 Unique NPC Cloning Fully Fixed
Named unique NPCs (such as Stalleo, Faendal, Farkas, etc.) will no longer be mistakenly cloned as enemy templates under any circumstances.

#### 🏘️ Town & Settlement Safe Zones
Cities, towns, villages, and player houses—including those added by other mods—are strictly designated as safe zones. The mod remains completely passive in these areas.

#### 🏰 Cell-Based Spawning Restricted to Interiors
The periodic cell spawn system now exclusively operates inside interior locations (dungeons, caves), allowing the separate Ambush system to handle outdoor encounters without clutter.

### V2.4 Update Notes

#### 🎲 Configurable Backup Forces Chance
Interior backup forces now rely on a customizable percentage chance (default 30%) per dungeon run instead of triggering unconditionally. Adjustable live via INI (`iBackupChance`) or SKSE Menu Framework (0-100%).

#### 🧟 Draugr Frozen/Idle Ambush State Fix
Fixed an issue where spawned Draugr remained frozen or unresponsive until hit. Ambush/sleeping animation graph locks (`bIsAmbushing`, `IsSleeping`) are automatically cleared upon spawn, waking up the AI to actively engage the player.

#### 🏛️ Interior Sub-Cell Instant Scanning
When moving through doors into new sub-cells/sections inside a dungeon, enemy types in that specific section are scanned and cached immediately.

#### 💀 Skeleton Support
All Skeleton varieties (`Skeleton`, `Skeleton Archer`, `Skeleton Mage`, etc.) across dungeons, crypts, and necromancer lairs have been fully integrated into the recognition system. Skeletons now spawn alongside or in place of Draugr during dungeon reinforcements and ambushes.

### V2.5 Update Notes

#### 💥 Critical Crash Fix — Havok Raycast in Interior Cells (Embershard Mine / HDT-SMP Crash)
A critical engine crash caused by calling Havok physics raycast (`bhkWorld::PickObject`) inside interior cells while HDT-SMP was simultaneously updating physics has been completely resolved.
- **Root Cause:** Calling `PickObject` on `bhkWorldM` during the same frame as HDT-SMP's physics step produced a Windows Structured Exception (access violation). These SEH crashes cannot be caught by C++ `try-catch`.
- **Fix:** Havok raycasting is now **completely disabled inside interior cells**. Spawn positions are calculated using a simple random X/Y offset from the anchor position, with no Havok calls whatsoever. The game engine's built-in gravity then settles the NPC onto the floor naturally.
- Exterior spawning is unaffected — the mountain/cliff collision prevention system continues to work normally outdoors.

#### 🔢 Actor Count Limit Fix — Vanilla NPCs No Longer Counted
The `MaxRuntimeActorsPerCell` limit now counts only **our mod's spawned** runtime actors (FormID `0xFF000000+`) instead of **all** living actors in the cell.
- **Old behavior:** In populous dungeons like Embershard Mine with 29 vanilla NPCs, a limit of 30 meant only 1 clone could spawn even with `SpawnDensity=100%`.
- **New behavior:** Vanilla and other mod NPCs are excluded from the count. The desired number of clones now spawns correctly.

#### 🚫 Civilian and Miner NPC Cloning Prevented
Friendly/neutral NPCs added by mods like SRC (Skyrim Realistic Conquering) — such as `Embershard Miner` or `Embershard Mine Bailiff` — are no longer mistakenly cloned as bandits or boss enemies.
- **Root Cause:** The CombatStyle fallback system treated any NPC with a combat style but no recognized faction ID as a valid enemy template. Miners fit this profile.
- **Fix:** NPCs whose name or EditorID contains `miner`, `worker`, `captive`, `prisoner`, `civilian`, or `citizen` are now permanently excluded from the regular cache, boss cache, anchor list, and INI loading. Additionally, humanoid-faction NPCs can only pass the CombatStyle fallback if they are actively hostile to the player **or** their name/EID contains clear enemy keywords (`bandit`, `outlaw`, `thug`, `highwayman`, etc.).
- Miner/civilian FormIDs previously saved to the INI from earlier sessions are also filtered out on game load.

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
