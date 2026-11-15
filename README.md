# Süzgeç — 48 bantlık spektral filtre

Süzgeç, 48 bant geçiren filtreden oluşan bir ses efekti (VST3/AU). Bant merkezleri 55 Hz'den başlıyor ve 9:8 oranıyla (tam ses, 203.9 sent) ~13.9 kHz'e kadar diziliyor. Ekranda bir spektral şekil çizersin, gelen ses o şekle göre süzülür. Yüksek rezonansta camsı tınlamalar, derin vuruşlar ve metalik dokular çıkar.

Kılavuzdaki bütün ses özellikleri var: filtre bankası, çizim, Ters Çevir, Trip, Ring Mod, Downsample, Spektral Yakalama, MIDI kontrolü (klavye modu ve sabit banka), presetler, parametre kilidi, rastgeleleştirici ve oversampling.

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

### Trip
Bant seviyelerini ve stereo alanı hareketlendirir. Çizdiğin şekil kayıtlı kalır; Trip onu canlandırır. Ekranda dolgu canlı seviyeyi, ince çizgi kayıtlı seviyeyi gösterir.

| Ayar | Ne yapar |
|---|---|
| Açık | Trip'i açar/kapatır. |
| Hız | 0.01 Hz – 2 kHz. Yüksek hızlarda (ses frekansında) genlik modülasyonu gibi davranır. |
| Senk | Hızı host temposuna kilitler: 64 ölçü … 1/64, noktalı (D) ve üçleme (T) dahil. Ölçü = 4 vuruş. Çalarken faz şarkı konumuna kilitlenir, yani her çalışta aynı yerden başlar. |
| Yoğunluk | Bant seviyesi modülasyonunun derinliği. %0'da seviyeler değişmez. |
| Şekil | Desen: sıralı dalga (bantlar sırayla) → tek/çift dönüşümlü → karışık. Pan hareketini de düzgün salınımdan düzensize doğru değiştirir. |
| Ters Yön | Trip döngüsünün yönünü çevirir. |
| Pan'a | Trip'in stereo konumu ne kadar hareket ettireceği. Yoğunluk %0 olsa da çalışır (Trip açık olmalı). |

Sağdaki **Desen** ekranında noktalar her bandın faz kaymasını, çizgi o anki fazı, alttaki çubuklar bantların o anki seviye çarpanını gösterir.

### Ring Mod
1. **RING MOD**'u aç.
2. Ekranda bantlara **tıklayarak** seç ya da bırak (seçili bantların üstünde ◆ belirir). Sürüklemek yine seviye çizer.
3. Seçili bantların filtrelenmiş sesi taşıyıcı olur ve işlenmiş sesi çarpar: metalik, inharmonik dokular.

**RASTGELE**, seçim yokken 3–5 rastgele bant seçer; seçim varken düğme **TEMİZLE** olur. Seçili bantların seviyeleri ve Trip, taşıyıcıyı da etkiler. Taşıyıcının seviyesi otomatik dengelenir, böylece ses kaybolmaz. Seçim yoksa ses modüle edilmez. Seçim projeyle ve presetle birlikte kaydedilir.

### Downsample 1/4
İşlenmiş sesi host örnekleme hızının dörtte birine düşürür (48 kHz projede 12 kHz). Filtre olmadan örnek tuttuğu için kasıtlı olarak kaba ve aliasing'li bir ses verir. Ring Mod ve Downsample karışımdan önce çalışır; dokuyu orijinalle harmanlamak için Karışım'ı düşür.

### Spektral Yakalama (Analiz / Uygula)
Bir sesin enerji dağılımını okuyup 48 bandın seviyelerine yazar. Ses kaydı değil, spektral bir şekil saklanır.

1. Kaynak sesi Süzgeç'ten geçir. Başlarken Trip ve Klavye modu kapalı olsun.
2. **ANALİZ**'e bas. Ekran gelen spektrumu gösterir. Kullanılabilir bir sinyal gelince düğme **UYGULA** olur.
3. İstediğin spektrum ekrandayken **UYGULA**'ya bas. Analiz durur, şekil bant seviyelerine yazılır.
4. Aynı Süzgeç'ten başka bir ses geçir ya da şekli preset olarak kaydedip başka track'te aç. Yeni ses yakalanan şekille süzülür.

- Analiz sırasında **TERS ÇEVİR**: gösterilen şeklin tersini uygular ve analizi bitirir.
- Ekranda çizmeye başlamak analizi uygulamadan durdurur. Analiz sırasında **ANALİZ…** düğmesine basmak da iptal eder.
- Düğme ANALİZ… durumunda kalıyorsa Süzgeç'e ses gelmiyordur. Pencere açıkken, kaynak durmadan UYGULA'ya bas.
- Ekranda en güçlü bant tepeye oturur; ondan 48 dB aşağısı sıfırdır.

### MIDI kontrolü
Süzgeç ses üretmez: MIDI bantları açıp kapatır, ses yine Süzgeç'ten geçmelidir. Yüksek rezonansla sürekli gürültü üzerine MIDI çalmayı dene (fabrika preseti **Gürültü Klavyesi**).

**Ableton Live 10'da MIDI göndermek:** Yeni bir MIDI track aç. Track'in **MIDI To** menüsünden Süzgeç'in olduğu audio track'i, alttaki menüden **Suzgec**'i seç. MIDI track'te Monitor'u **In** yap ya da track'i kayda hazırla. Süzgeç listede çıkmazsa AU sürümünü kullan (Audio Units açık olmalı). Ekrandaki klavye MIDI track olmadan da çalışır.

| | |
|---|---|
| **Klavye modu** (KLAVYE) | Ekranın altında A0–A8 klavyesi açılır. Bir nota en yakın bandı tam nota frekansına akort eder, velocity seviyesini belirler. Notası basılı olmayan bantlar kapanır. Çıkınca çizdiğin şekil geri gelir ve Kaydır yeniden açılır. |
| **Nota bırakma** | Bırakılan notanın bandı, sönüm süresine göre kapanır. Aynı banda düşen notalarda en son basılan öncelikli; onu bırakınca basılı kalan önceki notaya döner. |
| **Sabit banka** (KLAVYE kapalı) | MIDI en yakın sabit bandı akort etmeden açar. Diğer çizili bantlar olduğu gibi kalır. Notayı bırakınca o bant kapanır; o bandı ekranda yeniden çizince tekrar senin kontrolüne geçer. |
| **Pitch wheel** | Perde bükmez; bant **atak** süresini ayarlar: en altta 20 ms, en üstte 5 sn. |
| **Mod wheel (CC1)** | Bant **sönüm** süresini ayarlar: 20 ms – 5 sn. |

Atak ve sönüm süreleri MIDI kullanılırken ekranın durum satırında görünür.

### Presetler
Üstteki çubuk: **<** / **>** önceki/sonraki preset, ortadaki isim tıklanınca liste açılır, **KAYDET** kullanıcı preseti kaydeder, **ZAR** rastgele bir ses üretir.

- 12 fabrika preseti var (Cam Çanlar, Derin Gong, Metal Tel, Nefes Alan Spektrum, Lo-Fi Radyo, Gürültü Klavyesi…). Ableton'un program listesinde de görünürler.
- Kullanıcı presetleri `~/Music/Suzgec/Presets/` klasörüne `.szpreset` olarak kaydedilir. Listeden **Preset klasörünü aç** ile Finder'da açabilirsin.
- **ZAR**, bant şeklini, filtreyi, pan'ı, Trip'i, Ring Mod'u ve Downsample'ı rastgele ayarlar. Seviyeleri bozmamak için Giriş, Çıkış, Karışım ve Yumuşak Kırp'a dokunmaz.

### Parametre kilidi
Bir düğmeye ya da anahtara **sağ tıkla** → **Kilitle**. Kilitli parametreler preset değiştirirken, ZAR'da ve panodan yapıştırırken değişmez. Kilitli kontrollerin köşesinde kilit simgesi görünür.

- **Hepsini kilitle / Bütün kilitleri aç** aynı menüde.
- Ekrana sağ tıklamak 48 bant seviyesini birlikte kilitler. Böylece çizdiğin şekli koruyup presetlerin diğer ayarlarını deneyebilirsin.
- Ring Mod bant seçimi, **RING MOD** anahtarının kilidine bağlıdır.

### Değer girme
Bir düğmeye **Shift+tık** (ya da sağ tık → **Değer gir…**) ya da değer yazısına çift tık: değeri elle yaz.

### Oversampling
**MENÜ → Oversampling**: Kapalı, 2x, 4x, 8x, 16x. Satürasyon ve ring mod gibi doğrusal olmayan işlemlerden doğan istenmeyen aliasing'i azaltır. Karşılığında işlemci yükü ve gecikme artar; gecikme Ableton'a bildirilir. 16x çok ağırdır. Downsample her zaman host örnekleme hızına bağlı kalır, oversampling onun kasıtlı kabalığını gidermez.

### Görünüm
- **Renk düğmesi** (YEŞİL, TURUNCU, MAVİ, KIRMIZI, SARI, MOR, S/B): tıklayınca sonraki renk, sağ tıklayınca önceki renk.
- **CRT:** tarama çizgileri ve ekran parıltısı.
- **MENÜ:** oversampling, pencere boyutu (Küçük/Standart/Büyük), parlaklık, kontrast, preseti panoya kopyala / panodan yapıştır, bütün bantları %75'e sıfırla, bütün kilitleri aç. Pencereyi sağ alt köşeden sürükleyerek de boyutlandırabilirsin.
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
