#pragma once
#include <juce_core/juce_core.h>

// Kaynak dosyalar UTF-8; Türkçe karakter içeren her metin bu makrodan geçmeli.
#define TR(s) juce::String(juce::CharPointer_UTF8(s))
