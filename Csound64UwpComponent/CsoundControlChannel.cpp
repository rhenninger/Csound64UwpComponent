#include "pch.h"
#include "CsoundControlChannel.h"
#include "CsoundControlChannel.g.cpp"
#include "csound.h"

namespace winrt::Csound64UwpComponent::implementation
{
    CsoundControlChannel::CsoundControlChannel()
    {
        m_channelInfo.type = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL;
    }

    Windows::Foundation::IInspectable CsoundControlChannel::Value()
    {
        return winrt::box_value(ActualValue());
    }

    void CsoundControlChannel::Value(Windows::Foundation::IInspectable const& value)
    {
        auto prop = value.as<winrt::Windows::Foundation::IPropertyValue>();
        switch (prop.Type())
        {
        case winrt::Windows::Foundation::PropertyType::Double:
            ActualValue(unbox_value_or<double>(value, 0));
            break;
        case winrt::Windows::Foundation::PropertyType::Int32:
            ActualValue((double)(unbox_value_or<int>(value, 0)));
            break;
        case winrt::Windows::Foundation::PropertyType::Single:
            ActualValue((double)(unbox_value_or<float>(value, 0)));
            break;
        default:
            break;
        }
    }

    double CsoundControlChannel::ActualValue()
    {
        int err = (mp_csound && m_channelInfo.name) ? 0 : 1 ;
        auto val = csoundGetControlChannel(mp_csound, m_channelInfo.name,  &err);
        return (err == NULL) ? val : std::nan("");
    }

    void CsoundControlChannel::ActualValue(double value)
    {
        if (mp_csound && m_channelInfo.name) csoundSetControlChannel(mp_csound, m_channelInfo.name, value);
    }

    Csound64UwpComponent::ControlChannelBehavior CsoundControlChannel::Behavior()
    {
        return static_cast<Csound64UwpComponent::ControlChannelBehavior>(m_channelInfo.hints.behav);
    }
    double CsoundControlChannel::SuggestedDefaultValue()
    {
        return m_channelInfo.hints.dflt;
    }
    double CsoundControlChannel::SuggestedMinimumValue()
    {
        return m_channelInfo.hints.min;
    }
    double CsoundControlChannel::SuggestedMaximumValue()
    {
        return m_channelInfo.hints.max;
    }
    void CsoundControlChannel::UpdateSuggestedValues(Csound64UwpComponent::ControlChannelBehavior const& behavior, double minimum, double maximum, double defaultValue)
    {
        m_channelInfo.hints.behav = static_cast<controlChannelBehavior>(behavior);
        m_channelInfo.hints.min = minimum;
        m_channelInfo.hints.max = maximum;
        m_channelInfo.hints.dflt = defaultValue;
        auto result = csoundSetControlChannelHints(mp_csound, m_channelInfo.name, m_channelInfo.hints);
    }

    Windows::Foundation::Point CsoundControlChannel::SuggestedControllerPosition()
    {
        return Windows::Foundation::Point(static_cast<float>(m_channelInfo.hints.x), static_cast<float>(m_channelInfo.hints.y));
    }

    void CsoundControlChannel::SuggestedControllerPosition(Windows::Foundation::Point const& value)
    {
        m_channelInfo.hints.x = static_cast<int>(value.X);
        m_channelInfo.hints.y = static_cast<int>(value.Y);
        auto result = csoundSetControlChannelHints(mp_csound, m_channelInfo.name, m_channelInfo.hints);
    }

    Windows::Foundation::Size CsoundControlChannel::SuggestedControllerSize()
    {
        return Windows::Foundation::Size(static_cast<float>(m_channelInfo.hints.width), static_cast<float>(m_channelInfo.hints.height));
    }

    void CsoundControlChannel::SuggestedControllerSize(Windows::Foundation::Size const& value)
    {
        m_channelInfo.hints.height = static_cast<int>(value.Height);
        m_channelInfo.hints.width = static_cast<int>(value.Width);
        auto result = csoundSetControlChannelHints(mp_csound, m_channelInfo.name, m_channelInfo.hints);
    }

    hstring CsoundControlChannel::ControllerAttributes()
    {
        return winrt::to_hstring(m_attributes);
    }

    void CsoundControlChannel::ControllerAttributes(hstring const& value)
    {
        m_attributes = winrt::to_string(value);
        m_channelInfo.hints.attributes = m_attributes.data();
    }

    hstring CsoundControlChannel::Name()
    {
        return winrt::to_hstring(m_name);
    }
    void CsoundControlChannel::Name(hstring const& value)
    {
        m_name = winrt::to_string(value);
        m_channelInfo.name = m_name.data();
    }
    Csound64UwpComponent::ChannelType CsoundControlChannel::Type()
    {
        return Csound64UwpComponent::ChannelType::ControlChannel;
    }
    bool CsoundControlChannel::IsInput()
    {
        return (m_channelInfo.type & CSOUND_INPUT_CHANNEL);
    }
    bool CsoundControlChannel::IsOutput()
    {
        return (m_channelInfo.type & CSOUND_OUTPUT_CHANNEL);
    }

    void CsoundControlChannel::DeclareChannelParameters(CSOUND* pCsound, hstring const& name, bool isInput, bool isOutput)
    {
        mp_csound = pCsound;
        double* value{};
        m_channelInfo.type = CSOUND_CONTROL_CHANNEL;
        if (isInput) m_channelInfo.type |= CSOUND_INPUT_CHANNEL;
        if (isOutput) m_channelInfo.type |= CSOUND_OUTPUT_CHANNEL;
        Name(name);
        auto ok = csoundGetChannelPtr(pCsound, &value, m_name.c_str(), m_channelInfo.type);
        auto hok = csoundGetControlChannelHints(pCsound, m_name.c_str(), &m_channelInfo.hints);
        if (m_channelInfo.hints.attributes)
        {
            m_attributes = m_channelInfo.hints.attributes; //clean up csound allocated data.
            free(m_channelInfo.hints.attributes);
            m_channelInfo.hints.attributes = m_attributes.data();
        }
    }


}
