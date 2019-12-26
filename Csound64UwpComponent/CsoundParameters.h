#pragma once
#include "CsoundParameters.g.h"
#include "csound.h"


namespace winrt::Csound64UwpComponent::implementation
{
	struct CsoundParameters : CsoundParametersT<CsoundParameters>
	{
		CsoundParameters() = default;

		bool DebugMode();
		void DebugMode(bool value);

		Csound64UwpComponent::MessageLevels MessageLevel();
		void MessageLevel(Csound64UwpComponent::MessageLevels const& value);

		int32_t BufferFrames();
		void BufferFrames(int32_t value);

		int32_t HardwareBufferFrames();
		void HardwareBufferFrames(int32_t value);

		int32_t Tempo();
		void Tempo(int32_t value);

		bool IsSampleAccurate();
		void IsSampleAccurate(bool value);

		double SampleRateOverride();
		void SampleRateOverride(double value);

		double e0dbfsOverride();
		void e0dbfsOverride(double value);

		int32_t KsmpsOverride();
		void KsmpsOverride(int32_t value);

		int32_t NchnlsOverride();
		void NchnlsOverride(int32_t value);

		int32_t InputNchnlsOverride();
		void InputNchnlsOverride(int32_t value);


		/***********  Internal (Public) Code that is used within Csound64 Module but not part of public metadata in winmd file.   ************/
		void RefreshLocalCsoundParams(CSOUND* pCsound);
		void UpdateCsoundParams(CSOUND* pCsound);

	private:
		CSOUND_PARAMS m_params{};
    };
}
namespace winrt::Csound64UwpComponent::factory_implementation
{
    struct CsoundParameters : CsoundParametersT<CsoundParameters, implementation::CsoundParameters>
    {
    };
}
