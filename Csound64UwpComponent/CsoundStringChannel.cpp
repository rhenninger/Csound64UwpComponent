#include "pch.h"
#include "CsoundStringChannel.h"
#include "CsoundStringChannel.g.cpp"

namespace winrt::Csound64UwpComponent::implementation
{
    CsoundStringChannel::CsoundStringChannel()
    {
        m_channelInfo.type = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL;
    }

    hstring CsoundStringChannel::ActualValue()
    {
        size_t size = csoundGetChannelDatasize(mp_csound, m_channelInfo.name);
        char* c = new char[size+1];
        csoundGetStringChannel(mp_csound, m_channelInfo.name, c);
        std::string val{ c }; //to strip excessive trailing \0
        delete[] c;
        return winrt::to_hstring(val);
    }

    void CsoundStringChannel::ActualValue(hstring const& value)
    {
        if (mp_csound && m_channelInfo.name) csoundSetStringChannel(mp_csound, m_channelInfo.name, winrt::to_string(value).data());

    }
    hstring CsoundStringChannel::Name()
    {
        return winrt::to_hstring(m_name);
    }

    void CsoundStringChannel::Name(hstring const& value)
    {
        m_name = winrt::to_string(value);
        m_channelInfo.name = m_name.data();
    }

    Csound64UwpComponent::ChannelType CsoundStringChannel::Type()
    {
        return Csound64UwpComponent::ChannelType::StringChannel;
    }

    bool CsoundStringChannel::IsInput()
    {
        return (m_channelInfo.type & CSOUND_INPUT_CHANNEL);
    }

    bool CsoundStringChannel::IsOutput()
    {
        return (m_channelInfo.type & CSOUND_OUTPUT_CHANNEL);
    }

    Windows::Foundation::IInspectable CsoundStringChannel::Value()
    {
        return winrt::box_value(winrt::to_hstring(ActualValue()));
    }

    void CsoundStringChannel::Value(Windows::Foundation::IInspectable const& value)
    {
        ActualValue(unbox_value_or<hstring>(value, L""));
    }

    void CsoundStringChannel::DeclareChannelParameters(CSOUND* pCsound, hstring const& name, bool isInput, bool isOutput)
    {
        mp_csound = pCsound;
        double* value{};
        m_channelInfo.type = CSOUND_STRING_CHANNEL;
        if (isInput) m_channelInfo.type |= CSOUND_INPUT_CHANNEL;
        if (isOutput) m_channelInfo.type |= CSOUND_OUTPUT_CHANNEL;
        Name(name);
        csoundGetChannelPtr(pCsound, &value, m_name.c_str(), m_channelInfo.type);
    }

}
