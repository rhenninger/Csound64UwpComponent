#pragma once
#include "CsoundControlChannel.g.h"
#include "csound.h"

namespace winrt::Csound64UwpComponent::implementation
{
    struct CsoundControlChannel : CsoundControlChannelT<CsoundControlChannel>
    {
        CsoundControlChannel();

        hstring Name();
        void Name(hstring const& value);
        Csound64UwpComponent::ChannelType Type();
        bool IsInput();
        bool IsOutput();

        Windows::Foundation::IInspectable Value();
        void Value(Windows::Foundation::IInspectable const& value);
        double ActualValue();
        void ActualValue(double value);

        Csound64UwpComponent::ControlChannelBehavior Behavior();
        double SuggestedDefaultValue();
        double SuggestedMinimumValue();
        double SuggestedMaximumValue();
        void UpdateSuggestedValues(Csound64UwpComponent::ControlChannelBehavior const& behavior, double minimum, double maximum, double defaultValue);

        Windows::Foundation::Point SuggestedControllerPosition();
        void SuggestedControllerPosition(Windows::Foundation::Point const& value);
        Windows::Foundation::Size SuggestedControllerSize();
        void SuggestedControllerSize(Windows::Foundation::Size const& value);
        hstring ControllerAttributes();
        void ControllerAttributes(hstring const& value);

       /***********  Internal (Public) Code that is used within Csound64 Module but not part of public metadata in winmd file.   ************/
        void DeclareChannelParameters(CSOUND* pCsound, hstring const& name, bool isInput, bool isOutput);

    private:
        CSOUND* mp_csound{ nullptr }; //we will keep a local copy needed for internal calls without reference to composition
        controlChannelInfo_t m_channelInfo{};
        std::string m_name{}; //local strings for auto-memory management upon destruction.
        std::string m_attributes{};
    };
}
namespace winrt::Csound64UwpComponent::factory_implementation
{
    struct CsoundControlChannel : CsoundControlChannelT<CsoundControlChannel, implementation::CsoundControlChannel>
    {
    };
}
