#pragma once
#include "MidiDataAvailableEventArgs.g.h"

namespace winrt::Csound64UwpComponent::implementation
{
    struct MidiDataAvailableEventArgs : MidiDataAvailableEventArgsT<MidiDataAvailableEventArgs>
    {
        MidiDataAvailableEventArgs() = default;
        

        hstring DeviceName();
        com_array<uint8_t> MidiData();

        void LoadMidiData(hstring deviceName, winrt::array_view<uint8_t const> data);
    private:
        winrt::hstring m_deviceName{};
        winrt::com_array<uint8_t> m_mididata{};
    };
}
