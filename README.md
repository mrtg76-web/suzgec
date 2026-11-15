# Süzgeç — 48 bantlık spektral filtre

Süzgeç, 48 bant geçiren filtreden oluşan bir ses efekti (VST3/AU). Bant merkezleri 55 Hz'den başlıyor ve 9:8 oranıyla (tam ses, 203.9 sent) ~13.9 kHz'e kadar diziliyor. Ekranda bir spektral şekil çizersin, gelen ses o şekle göre süzülür. Yüksek rezonansta camsı tınlamalar, derin vuruşlar ve metalik dokular çıkar.

> **Faz 1.** Bu sürümde filtre bankası, çizim ekranı, Ana/Filtre/Pan bölümleri ve Ters Çevir var. Trip modülasyonu, Ring Mod ve Downsample 2. fazda; Spektral Yakalama (Analiz/Uygula), MIDI kontrolü, presetler ve oversampling 3. fazda gelecek.

## Kurulum (Mac)

1. GitHub'da **Actions** sekmesine gir. En son yeşil (✓) çalışmayı aç, en alttaki `Suzgec-mac-…` paketini indir ve aç.
2. Dosyaları kopyala:
   - `Suzgec.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `Suzgec.component` → `~/Library/Audio/Plug-Ins/Components/`
3. Plugin imzasız olduğu için macOS ilk açılışta onu engelleyebilir. Terminal'de şu komutu bir kez çalıştır:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Suzgec.vst3 ~/Library/Audio/Plug-Ins/Components/Suzgec.component
   ```
4. Ableton > Preferences > Plug-Ins bölümünde **Use VST3 Plug-in System Folders** ve **Use Audio Units** açık olsun, sonra **Rescan**.
5. Bir audio track'e **Suzgec**'i efekt olarak yükle.

Paket hem Apple Silicon hem Intel/Rosetta için derlenir. Live 10 M1'de Rosetta ile çalıştığı için bu önemli.

**Windows:** `Suzgec-windows` paketindeki `Suzgec.vst3` klasörünü `C:\Program Files\Common Files\VST3\` içine kopyala.

## Kullanım

### Ekran
- **Sürükle:** bant seviyelerini çiz. Tek harekette birden fazla bandın üzerinden geçebilirsin. Dikey konum seviyeyi (%0–100), yatay konum bandı belirler.
- **Alt + tık** ya da **çift tık:** bandı varsayılan seviyeye (%75) döndür.
- Durum satırı, fare altındaki bandın numarasını, frekansını, nota adını ve seviyesini gösterir.
- Bantların içindeki LED blokları o bandın canlı çıkış seviyesini gösterir (−60…0 dB).
- Her 6 bant yaklaşık bir oktav eder; alttaki frekans etiketleri buna göre yerleştirildi.
- Her bant seviyesi aynı zamanda host otomasyon parametresi (`Band 1` … `Band 48`).

### Ters Çevir
Bütün bant seviyelerini tersine çevirir: %90 → %10, %20 → %80, %50 aynı kalır. Tepeler ve boşluklar yer değiştirir; frekans sırası değişmez.

### Ana
| Ayar | Ne yapar |
|---|---|
| Giriş | Filtre bankasından önceki kazanç (−24…+24 dB). Yükseldikçe her bant kendi içinde satürasyona girer. |
| Çıkış | Kuru/ıslak karışımından sonraki kazanç (−24…+24 dB). |
| Karışım | %0 = orijinal ses, %100 = işlenmiş ses. |
| Yumuşak Kırp | En sonda, Çıkış'tan sonra tepe noktalarını yumuşakça yuvarlar. Bant içindeki satürasyondan ayrıdır. |

### Filtre
| Ayar | Ne yapar |
|---|---|
| Kaydır | Bütün bankayı bir yarım ton aşağı (−1 ST), yerinde (KAPALI) ya da yukarı (+1 ST) kaydırır. |
| Eğim | 12 dB/oct ya da 24 dB/oct. 24 daha ayrık bantlar ve daha güçlü spektral ayrım verir. |
| Rezonans | Filtrelerin Q değeri (2…100). Yükseldikçe tınlama ve perdeli yanıt artar. |

Rezonans ve eğim değişince genel seviye otomatik telafi edilir: bütün bantlar %100 iken çıkış yaklaşık girişle aynı seviyede kalır.

### Pan
Tek ve çift numaralı bantların stereo konumunu ayrı ayrı ayarlar. İkisini ortada bırakırsan görüntü ortada kalır. Zıt taraflara koyarsan genişlik elde edersin.

### Görünüm
- **Renk düğmesi** (YEŞİL, TURUNCU, MAVİ, KIRMIZI, SARI, MOR, S/B): tıklayınca sonraki renk, sağ tıklayınca önceki renk.
- **CRT:** tarama çizgileri ve ekran parıltısı.
- **MENÜ:** pencere boyutu (Küçük/Standart/Büyük), preseti panoya kopyala / panodan yapıştır, bütün bantları %75'e sıfırla. Pencereyi sağ alt köşeden sürükleyerek de boyutlandırabilirsin.
- Düğmelerde çift tık varsayılan değere döndürür. Değer yazısına çift tıklayıp elle değer girebilirsin.

Bu görünüm ayarları sesi etkilemez; projeyle birlikte kaydedilir.

## İpuçları
- Rezonansı %70'in üstüne çıkar, birkaç bandı açık bırak ve davul ya da gürültü geçir: perdeli, çan gibi vuruşlar çıkar.
- 24 dB eğim + Ters Çevir, bir sesin baskın frekanslarını oyup geri kalanını bırakmak için iyi bir yöntem.
- Tek bantları sola, çift bantları sağa al: her tam ses adımı farklı tarafa düşer, hareketli bir stereo alan oluşur.

## Derleme

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build        # çekirdek testleri
```

JUCE 8 derleme sırasında otomatik indirilir. DSP çekirdeği (`core/`) JUCE'den bağımsızdır ve testleri JUCE olmadan çalışır.

## Lisans

GPLv3. Bu proje bağımsız bir çalışmadır; herhangi bir ticari ürünle bağlantısı yoktur.
