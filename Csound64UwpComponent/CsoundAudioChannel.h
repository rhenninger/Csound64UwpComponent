#pragma once
#include "CsoundAudioChannel.g.h"
#include "csound.h"

namespace winrt::Csound64UwpComponent::implementation
{
    struct CsoundAudioChannel : CsoundAudioChannelT<CsoundAudioChannel>
    {
        CsoundAudioChannel();

        com_array<double> ActualValue();
        void ActualValue(array_view<double const> value);
        hstring Name();
        void Name(hstring const& value);
        Csound64UwpComponent::ChannelType Type();
        bool IsInput();
        bool IsOutput();
        Windows::Foundation::IInspectable Value();
        void Value(Windows::Foundation::IInspectable const& value);

        /***********  Internal (Public) Code that is used within Csound64 Module but not part of public metadata in winmd file.   ************/
        void DeclareChannelParameters(CSOUND* pCsound, hstring const& name, bool isInput, bool isOutput);


    private:
        CSOUND* mp_csound{ nullptr }; //we will keep a local copy needed for internal calls without reference to composition
        controlChannelInfo_t m_channelInfo{};
        std::string m_name{}; //local strings for auto-memory management upon destruction.

    };
}
namespace winrt::Csound64UwpComponent::factory_implementation
{
    struct CsoundAudioChannel : CsoundAudioChannelT<CsoundAudioChannel, implementation::CsoundAudioChannel>
    {
    };
}
