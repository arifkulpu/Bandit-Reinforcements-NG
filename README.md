# Bandit Reinforcements NG

[Türkçe](#türkçe) | [English](#english)

---

## Türkçe

### Modun Amacı
Bandit Reinforcements NG, Skyrim'deki haydut kamplarına ve zindanlarına yaklaştığınızda, rastgele sayıda ekstra haydut ekleyen dinamik bir SKSE eklentisidir. `CommonLibSSE-NG` kullanılarak geliştirildiği için **1.5.97 (SE), 1.6.1170 (AE) ve VR** sürümlerinin tümüyle tam uyumludur.

### Özellikler
- **Dinamik Çoğalma (Spawning):** Haydut kamplarına (Bandit Camp) veya zindanlara (Dungeon) giriş yaparken sistem tetiklenir ve ortama yeni haydutlar eklenir.
- **Seviye Ölçeklendirmesi:** Oyuncu seviyesine göre çıkacak maksimum haydut sayısı değişir.
  - Seviye 1–10 arası: Maksimum 3
  - Seviye 10–25 arası: Maksimum 6
  - Seviye 25+: Maksimum 10
- **Çeşitlilik:** Eklenen haydutlar, bölgedeki diğer haydut sınıflarına (okçu, savaşçı vb.) uyum sağlar.
- **Performans Dostu (Zero-Overhead):** Sistem arkada sürekli bir "Update" döngüsü çalıştırmaz. Sadece oyuncu yeni bir hücreye (Cell) girdiğinde durumu kontrol eder (Lazy Evaluation). Bu sayede FPS düşüşü yaşanmaz.
- **Bekleme Süresi (Cooldown):** 
  - Bir kamp tamamen temizlenirse, 10 oyun içi gün boyunca o kampa yeni haydut eklenmez.
  - Eğer kampa girip savaşmadan kaçarsanız, yaratılan grup 3 gün sonra otomatik olarak temizlenir.

### Kurulum
1. [SKSE](http://skse.silverlock.org/)'nin sürümünüze uygun versiyonunun kurulu olduğundan emin olun.
2. Oluşturulan `BanditReinforcementsNG.dll` ve (varsa) `BanditReinforcementsNG.ini` dosyalarını `Data/SKSE/Plugins/` dizininin içine atın.
3. Oyunu SKSE üzerinden başlatın.

---

## English

### Purpose of the Mod
Bandit Reinforcements NG is a dynamic SKSE plugin that adds a random number of extra bandits as you approach bandit camps and dungeons in Skyrim. Built using `CommonLibSSE-NG`, it is fully compatible with **1.5.97 (SE), 1.6.1170 (AE), and VR** versions out of the box.

### Features
- **Dynamic Spawning:** The system triggers when entering Bandit Camps or Dungeons, adding new bandits seamlessly to the environment.
- **Level Scaling:** The maximum number of extra bandits scales with the player's level.
  - Levels 1–10: Max 3
  - Levels 10–25: Max 6
  - Levels 25+: Max 10
- **Variety:** Added bandits match the regional leveled lists (archers, melee, etc.) ensuring they fit naturally into the camp.
- **Performance Friendly (Zero-Overhead):** The mod uses a Lazy Evaluation technique. Instead of running a constant background update loop, it only processes logic when the player attaches to a new cell. This ensures zero FPS drop.
- **Cooldown System:**
  - Cleared Camps: If you kill all spawned bandits, the camp goes on a 10 in-game day cooldown.
  - Escaped Camps: If you enter a camp but flee without fighting, the spawned group is automatically cleaned up after 3 in-game days.

### Installation
1. Ensure you have the appropriate version of [SKSE](http://skse.silverlock.org/) installed for your game.
2. Drop the `BanditReinforcementsNG.dll` and `BanditReinforcementsNG.ini` (if available) into your `Data/SKSE/Plugins/` folder.
3. Launch the game via SKSE.
