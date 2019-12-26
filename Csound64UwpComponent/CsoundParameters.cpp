#include "pch.h"
#include "CsoundParameters.h"
#include "CsoundParameters.g.cpp"


namespace winrt::Csound64UwpComponent::implementation
{
    bool CsoundParameters::DebugMode()
    {
		return m_params.debug_mode != 0;
    }
    void CsoundParameters::DebugMode(bool value)
    {
		m_params.debug_mode = value ? 1 : 0;
    }

	Csound64UwpComponent::MessageLevels CsoundParameters::MessageLevel()
	{
		return static_cast<Csound64UwpComponent::MessageLevels>(m_params.message_level);
	}
	void CsoundParameters::MessageLevel(Csound64UwpComponent::MessageLevels const& value)
	{
		m_params.message_level = static_cast<int>(value);
	}

    int32_t CsoundParameters::BufferFrames()
    {
		return m_params.buffer_frames;
    }
    void CsoundParameters::BufferFrames(int32_t value)
    {
		m_params.buffer_frames = value;
    }

    int32_t CsoundParameters::HardwareBufferFrames()
    {
		return m_params.hardware_buffer_frames;
    }
    void CsoundParameters::HardwareBufferFrames(int32_t value)
    {
		m_params.hardware_buffer_frames = value;
    }

    int32_t CsoundParameters::Tempo()
    {
		return m_params.tempo;
    }
    void CsoundParameters::Tempo(int32_t value)
    {
		m_params.tempo = value;
    }

	bool CsoundParameters::IsSampleAccurate()
	{
		return m_params.sample_accurate != 0;
	}
	void CsoundParameters::IsSampleAccurate(bool value)
	{
		m_params.sample_accurate = value ? 1 : 0;
	}

    double CsoundParameters::SampleRateOverride()
    {
		return m_params.sample_rate_override;
    }
    void CsoundParameters::SampleRateOverride(double value)
    {
		m_params.sample_rate_override = value;
    }

    double CsoundParameters::e0dbfsOverride()
    {
		return m_params.e0dbfs_override;
    }
    void CsoundParameters::e0dbfsOverride(double value)
    {
		m_params.e0dbfs_override = value;
    }

    int32_t CsoundParameters::KsmpsOverride()
    {
		return m_params.ksmps_override;
    }

    void CsoundParameters::KsmpsOverride(int32_t value)
    {
		m_params.ksmps_override = value;
    }

	int32_t CsoundParameters::NchnlsOverride()
	{
		return m_params.nchnls_override;
	}
	void CsoundParameters::NchnlsOverride(int32_t value)
	{
		m_params.nchnls_override = value;
	}

	int32_t CsoundParameters::InputNchnlsOverride()
	{
		return m_params.nchnls_i_override;
	}
	void CsoundParameters::InputNchnlsOverride(int32_t value)
	{
		m_params.nchnls_i_override = value;
	}


/***********  Internal (Public) Code that is used within Csound64 Module but not part of public metadata in winmd file.   ************/
	void CsoundParameters::RefreshLocalCsoundParams(CSOUND* pCsound)
	{
		if (pCsound) csoundGetParams(pCsound, &m_params);
	}
	void CsoundParameters::UpdateCsoundParams(CSOUND* pCsound)
	{
		if (pCsound) csoundSetParams(pCsound, &m_params);
	}
}