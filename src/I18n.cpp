#include "I18n.h"

// Sprachabhängige Textauswahl für die Radioansicht. Herkunft und Lizenz: docs/LIZENZEN.md.
extern const __FlashStringHelper *i18n_getRetroRadio(RetroRadioKey key, LangIndex lang);

const __FlashStringHelper *I18n::str(RetroRadioKey key)
{
    return i18n_getRetroRadio(key, LangIndex::DE);
}
