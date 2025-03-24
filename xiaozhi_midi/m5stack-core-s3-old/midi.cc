#include "M5UnitSynth.h"
#include "audio_codec.h"
#include "board.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "iot/thing.h"
#include "mqtt_client.h"
#include <esp_log.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <map>

#define TAG "Midi"

constexpr int MIDI_CHANNEL = 0;
constexpr int MAX_MIDI_CHANNELS = 16;

std::string trim(const std::string& str) {
    const auto first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

int noteNameToMidiNumber(const std::string& noteName) {
    static const std::map<std::string, int> noteMap = {
        {"NOTE_B0", 23}, {"NOTE_C1", 24}, {"NOTE_CS1", 25}, {"NOTE_D1", 26}, {"NOTE_DS1", 27},
        {"NOTE_E1", 28}, {"NOTE_F1", 29}, {"NOTE_FS1", 30}, {"NOTE_G1", 31}, {"NOTE_GS1", 32},
        {"NOTE_A1", 33}, {"NOTE_AS1", 34}, {"NOTE_B1", 35}, {"NOTE_C2", 36}, {"NOTE_CS2", 37},
        {"NOTE_D2", 38}, {"NOTE_DS2", 39}, {"NOTE_E2", 40}, {"NOTE_F2", 41}, {"NOTE_FS2", 42},
        {"NOTE_G2", 43}, {"NOTE_GS2", 44}, {"NOTE_A2", 45}, {"NOTE_AS2", 46}, {"NOTE_B2", 47},
        {"NOTE_C3", 48}, {"NOTE_CS3", 49}, {"NOTE_D3", 50}, {"NOTE_DS3", 51}, {"NOTE_E3", 52},
        {"NOTE_F3", 53}, {"NOTE_FS3", 54}, {"NOTE_G3", 55}, {"NOTE_GS3", 56}, {"NOTE_A3", 57},
        {"NOTE_AS3", 58}, {"NOTE_B3", 59}, {"NOTE_C4", 60}, {"NOTE_CS4", 61}, {"NOTE_D4", 62},
        {"NOTE_DS4", 63}, {"NOTE_E4", 64}, {"NOTE_F4", 65}, {"NOTE_FS4", 66}, {"NOTE_G4", 67},
        {"NOTE_GS4", 68}, {"NOTE_A4", 69}, {"NOTE_AS4", 70}, {"NOTE_B4", 71}, {"NOTE_C5", 72},
        {"NOTE_CS5", 73}, {"NOTE_D5", 74}, {"NOTE_DS5", 75}, {"NOTE_E5", 76}, {"NOTE_F5", 77},
        {"NOTE_FS5", 78}, {"NOTE_G5", 79}, {"NOTE_GS5", 80}, {"NOTE_A5", 81}, {"NOTE_AS5", 82},
        {"NOTE_B5", 83}, {"NOTE_C6", 84}, {"NOTE_CS6", 85}, {"NOTE_D6", 86}, {"NOTE_DS6", 87},
        {"NOTE_E6", 88}, {"NOTE_F6", 89}, {"NOTE_FS6", 90}, {"NOTE_G6", 91}, {"NOTE_GS6", 92},
        {"NOTE_A6", 93}, {"NOTE_AS6", 94}, {"NOTE_B6", 95}, {"NOTE_C7", 96}, {"NOTE_CS7", 97},
        {"NOTE_D7", 98}, {"NOTE_DS7", 99}, {"NOTE_E7", 100}, {"NOTE_F7", 101}, {"NOTE_FS7", 102},
        {"NOTE_G7", 103}, {"NOTE_GS7", 104}, {"NOTE_A7", 105}, {"NOTE_AS7", 106}, {"NOTE_B7", 107},
        {"NOTE_C8", 108}, {"NOTE_CS8", 109}, {"NOTE_D8", 110}, {"NOTE_DS8", 111}
    };

    const auto it = noteMap.find(noteName);
    return (it != noteMap.end()) ? it->second : -1;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

using InstrumentNotesMap = std::map<std::string, std::vector<std::string>>;

InstrumentNotesMap parseInstruments(const std::string& input) {
    InstrumentNotesMap instruments;
    const auto lines = split(input, '\n');
    for (const auto& line : lines) {
        const auto colonPos = line.find(": ");
        if (colonPos == std::string::npos) continue;

        const auto instrument = trim(line.substr(0, colonPos));
        const auto notesStr = trim(line.substr(colonPos + 2));
        const auto notes = split(notesStr, ',');

        instruments[instrument] = notes;
    }
    return instruments;
}

const std::unordered_map<std::string, unit_synth_instrument_t> instrumentMap = {
    {"GrandPiano_1", GrandPiano_1}, {"BrightPiano_2", BrightPiano_2}, {"ElGrdPiano_3", ElGrdPiano_3},
    {"HonkyTonkPiano", HonkyTonkPiano}, {"ElPiano1", ElPiano1}, {"Violin", Violin}, {"Flute", Flute},
    {"GuitarHarmonics", GuitarHarmonics}, {"SynthBass1", SynthBass1}, {"SynthBass2", SynthBass2},
    {"SynthStrings1", SynthStrings1}, {"SynthStrings2", SynthStrings2}, {"SynthBrass1", SynthBrass1},
    {"SynthBrass2", SynthBrass2}, {"SynthVoice", SynthVoice}, {"FX1Rain", FX1Rain},
    {"FX2Soundtrack", FX2Soundtrack}, {"FX3Crystal", FX3Crystal}, {"FX4Atmosphere", FX4Atmosphere},
    {"FX5Brightness", FX5Brightness}, {"FX6Goblins", FX6Goblins}, {"FX7Echoes", FX7Echoes},
    {"FX8SciFi", FX8SciFi}, {"Sitar", Sitar}, {"Banjo", Banjo}, {"Shamisen", Shamisen},
    {"Koto", Koto}, {"Kalimba", Kalimba}, {"BagPipe", BagPipe}, {"Fiddle", Fiddle},
    {"Shanai", Shanai}, {"TinkleBell", TinkleBell}, {"Agogo", Agogo}, {"SteelDrums", SteelDrums},
    {"Woodblock", Woodblock}, {"TaikoDrum", TaikoDrum}, {"MelodicTom", MelodicTom},
    {"SynthDrum", SynthDrum}, {"ReverseCymbal", ReverseCymbal}, {"GtFretNoise", GtFretNoise},
    {"BreathNoise", BreathNoise}, {"Seashore", Seashore}, {"BirdTweet", BirdTweet},
    {"TelephRing", TelephRing}, {"Helicopter", Helicopter}, {"Applause", Applause},
    {"Gunshot", Gunshot}
};

unit_synth_instrument_t getInstrumentFromString(const std::string& instrumentName) {
    const auto it = instrumentMap.find(instrumentName);
    if (it != instrumentMap.end()) {
        return it->second;
    }
    printf("Warning: Unknown instrument %s, using default GrandPiano_1\n", instrumentName.c_str());
    return GrandPiano_1;
}

std::unordered_map<std::string, int> instrumentChannelMap;
int nextChannel = 0;

int getMidiChannel(const std::string& instrument) {
    const auto it = instrumentChannelMap.find(instrument);
    if (it != instrumentChannelMap.end()) {
        return it->second;
    }

    const int assignedChannel = nextChannel % MAX_MIDI_CHANNELS;
    instrumentChannelMap[instrument] = assignedChannel;
    nextChannel++;
    return assignedChannel;
}

void playInstrumentTask(void* param) {
    auto* data = static_cast<std::tuple<M5UnitSynth*, std::vector<std::string>, int>*>(param);
    M5UnitSynth* synth = std::get<0>(*data);
    auto& notes = std::get<1>(*data);
    const int midiChannel = std::get<2>(*data);

    for (const auto& note : notes) {
        const auto note_info = split(note, '-');
        if (note_info.size() != 3) continue;

        const auto note_name = trim(note_info[0]);
        const int velocity = std::stoi(trim(note_info[1]));
        const int duration = std::stoi(trim(note_info[2]));

        if (note_name == "REST") {
            vTaskDelay(pdMS_TO_TICKS(duration));
            continue;
        }

        const int note_num = noteNameToMidiNumber(note_name);
        if (note_num == -1) continue;

        synth->setNoteOn(midiChannel, note_num, velocity);
        vTaskDelay(pdMS_TO_TICKS(duration));
        synth->setNoteOff(midiChannel, note_num, velocity);
    }

    delete data;
    vTaskDelete(nullptr);
}

void playNotesMulti(M5UnitSynth& synth, const std::string& input) {
    const auto instruments = parseInstruments(input);

    for (const auto& [instrument, notes] : instruments) {
        const int channel = getMidiChannel(instrument);
        printf("Playing %s on MIDI channel %d...\n", instrument.c_str(), channel);

        auto* data = new std::tuple<M5UnitSynth*, std::vector<std::string>, int>(&synth, notes, channel);
        xTaskCreate(playInstrumentTask, instrument.c_str(), 4096, data, 1, nullptr);
    }
}

namespace iot {
    class Midi : public Thing {
    private:
        M5UnitSynth synth;
    public:
        Midi() : Thing("Midi", "midi播放器") {
            synth.begin(UART_NUM_2, UNIT_SYNTH_BAUD, 18, 17);
            methods_.AddMethod("Play", "乐队演奏", ParameterList({
                Parameter("Notes", R"(使用以下音符和乐器信息，创作一段音乐。

音符范围：
NOTE_B0 (23) 到 NOTE_DS8 (111)，以及 REST (0) 表示休止符。

乐器列表：
GrandPiano_1, BrightPiano_2, ElGrdPiano_3, HonkyTonkPiano, ElPiano1, ElPiano2, Harpsichord, Clavi, Celesta, Glockenspiel, MusicBox, Vibraphone, Marimba, Xylophone, TubularBells, Santur, DrawbarOrgan, PercussiveOrgan, RockOrgan, ChurchOrgan, ReedOrgan, AccordionFrench, Harmonica, TangoAccordion, AcGuitarNylon, AcGuitarSteel, AcGuitarJazz, AcGuitarClean, AcGuitarMuted, OverdrivenGuitar, DistortionGuitar, GuitarHarmonics, AcousticBass, FingerBass, PickedBass, FretlessBass, SlapBass1, SlapBass2, SynthBass1, SynthBass2, Violin, Viola, Cello, Contrabass, TremoloStrings, PizzicatoStrings, OrchestralHarp, Timpani, StringEnsemble1, StringEnsemble2, SynthStrings1, SynthStrings2, ChoirAahs, VoiceOohs, SynthVoice, OrchestraHit, Trumpet, Trombone, Tuba, MutedTrumpet, FrenchHorn, BrassSection, SynthBrass1, SynthBrass2, SopranoSax, AltoSax, TenorSax, BaritoneSax, Oboe, EnglishHorn, Bassoon, Clarinet, Piccolo, Flute, Recorder, PanFlute, BlownBottle, Shakuhachi, Whistle, Ocarina, Lead1Square, Lead2Sawtooth, Lead3Calliope, Lead4Chiff, Lead5Charang, Lead6Voice, Lead7Fifths, Lead8BassLead, Pad1Fantasia, Pad2Warm, Pad3PolySynth, Pad4Choir, Pad5Bowed, Pad6Metallic, Pad7Halo, Pad8Sweep, FX1Rain, FX2Soundtrack, FX3Crystal, FX4Atmosphere, FX5Brightness, FX6Goblins, FX7Echoes, FX8SciFi, Sitar, Banjo, Shamisen, Koto, Kalimba, BagPipe, Fiddle, Shanai, TinkleBell, Agogo, SteelDrums, Woodblock, TaikoDrum, MelodicTom, SynthDrum, ReverseCymbal, GtFretNoise, BreathNoise, Seashore, BirdTweet, TelephRing, Helicopter, Applause, Gunshot.

请按照以下格式输出你的创作：

[乐器1]: [音符1]-[力度1]-[时长1], [音符2]-[力度2]-[时长2], ...\r\n
[乐器2]: [音符3]-[力度3]-[时长3], [音符4]-[力度4]-[时长4], ...\r\n
...

其中：
- 音符使用提供的 NOTE_XXX 格式或 REST (0)。
- 力度范围为 0 到 127。
- 时长单位毫秒。)", kValueTypeString, true)
            }), [this](const ParameterList& parameters) {
                const std::string Notes = parameters["Notes"].string();
                printf("Notes: %s\n", Notes.c_str());
                playNotesMulti(synth, Notes);
            });
        }
    };
} // namespace iot

DECLARE_THING(Midi);
