#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <map>

class SuzgecProcessor;

// Fabrika ve kullanıcı presetleri, parametre kilidine uyarak yükleme, rastgeleleştirici.
// Bütün çağrılar mesaj iş parçacığından yapılmalı.
class PresetManager {
public:
    explicit PresetManager(SuzgecProcessor&);

    using Values = std::map<juce::String, float>; // parametre kimliği -> gerçek değer

    struct Entry {
        juce::String name;
        bool factory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    int numFactory() const { return (int) factory.size(); }
    juce::String factoryName(int i) const;
    std::vector<Entry> allEntries() const; // önce fabrika, sonra kullanıcı (alfabetik)

    void loadFactory(int index);
    bool loadFile(const juce::File&);
    bool save(const juce::String& name); // kullanıcı klasörüne
    void step(int delta);                 // listede önceki/sonraki
    void randomize();

    static juce::File userFolder(); // ~/Music/Suzgec/Presets
    juce::String currentName() const;
    int currentFactoryIndex() const { return lastFactory; }

    // Kilitli olmayan parametrelere değerleri uygula (ring seçimi: ringOn kilitli değilse)
    void apply(const Values& values, const std::uint64_t* ringMask, const juce::String& name);
    // ValueTree (proje/pano/dosya biçimi) -> değerler
    static bool treeToValues(const juce::ValueTree& tree, Values& out, std::uint64_t& ringMask);

private:
    struct Factory {
        const char* name;
        std::function<void(Values&, std::uint64_t&)> build;
    };
    Values defaults() const;
    void setCurrentName(const juce::String&);

    SuzgecProcessor& proc;
    std::vector<Factory> factory;
    int lastFactory = 0;
};
