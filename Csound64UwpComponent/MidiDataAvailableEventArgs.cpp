#include "pch.h"
#include "MidiDataAvailableEventArgs.h"
#include "MidiDataAvailableEventArgs.g.cpp"
#include <algorithm>

using namespace winrt::Windows::Devices::Midi;

namespace winrt::Csound64UwpComponent::implementation
{
    
    void MidiDataAvailableEventArgs::LoadMidiData(hstring deviceName, winrt::array_view<uint8_t const> data) {
        m_deviceName = deviceName;
        m_mididata = winrt::com_array<uint8_t>(data.cbegin(), data.cend());
    }


    hstring MidiDataAvailableEventArgs::DeviceName()
    {
        return winrt::to_hstring(m_deviceName);
    }

    com_array<uint8_t> MidiDataAvailableEventArgs::MidiData()
    {
        return winrt::com_array<uint8_t>(m_mididata.begin(), m_mididata.end());
    }
}
