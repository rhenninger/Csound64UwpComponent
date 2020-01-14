#include "pch.h"
#include "CsoundAudioChannel.h"
#include "CsoundAudioChannel.g.cpp"
#include <vector>

namespace winrt::Csound64UwpComponent::implementation
{

    CsoundAudioChannel::CsoundAudioChannel()
    {
        m_channelInfo.type = CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL;
    }

    com_array<double> CsoundAudioChannel::ActualValue()
    {
        auto samples = winrt::com_array<double>(csoundGetKsmps(mp_csound) * csoundGetNchnls(mp_csound));
        csoundGetAudioChannel(mp_csound, m_channelInfo.name, samples.data());
        return samples;
    }

    void CsoundAudioChannel::ActualValue(array_view<double const> value)
    {
        std::vector<double> v(value.size()); //to de-const incomming array to match csound argument
        std::copy(value.cbegin(), value.cend(), v.begin());
        if (mp_csound && m_channelInfo.name) csoundSetAudioChannel(mp_csound, m_channelInfo.name, v.data());
    }

    hstring CsoundAudioChannel::Name()
    {
        return winrt::to_hstring(m_name);
    }

    void CsoundAudioChannel::Name(hstring const& value)
    {
        m_name = winrt::to_string(value);
        m_channelInfo.name = m_name.data();
    }

    Csound64UwpComponent::ChannelType CsoundAudioChannel::Type()
    {
        return Csound64UwpComponent::ChannelType::AudioChannel;
    }

    bool CsoundAudioChannel::IsInput()
    {
        return (m_channelInfo.type & CSOUND_INPUT_CHANNEL);
    }

    bool CsoundAudioChannel::IsOutput()
    {
        return (m_channelInfo.type & CSOUND_OUTPUT_CHANNEL);
    }

    Windows::Foundation::IInspectable CsoundAudioChannel::Value()
    {
        auto v = ActualValue();
        auto vv = winrt::array_view<const double>{ v.begin(), v.end() };
        return winrt::Windows::Foundation::PropertyValue::CreateDoubleArray(vv);
    }

    void CsoundAudioChannel::Value(Windows::Foundation::IInspectable const& value)
    {
        winrt::com_array<double> v{}; //suitable for receiving GetDoubleArray() ; not const double here
        auto prop = value.as<winrt::Windows::Foundation::IPropertyValue>();
        prop.GetDoubleArray(v); 
        csoundSetAudioChannel(mp_csound, m_channelInfo.name, v.data());//bypass ActualValue which would force still more copying
    }

    void CsoundAudioChannel::DeclareChannelParameters(CSOUND* pCsound, hstring const& name, bool isInput, bool isOutput)
    {
        mp_csound = pCsound;
        double* value{};
        m_channelInfo.type = CSOUND_AUDIO_CHANNEL;
        if (isInput) m_channelInfo.type |= CSOUND_INPUT_CHANNEL;
        if (isOutput) m_channelInfo.type |= CSOUND_OUTPUT_CHANNEL;
        Name(name);
        csoundGetChannelPtr(pCsound, &value, m_name.c_str(), m_channelInfo.type);
    }


}
