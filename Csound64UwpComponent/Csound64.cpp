#include "pch.h"
#include "Csound64.h"
#include "Csound64.g.cpp"
#include <cstdio>
#include <functional>
#include <algorithm>


using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media;
using namespace std::placeholders;

//Encapsulated interface to get a pointer to the raw memory of an AudioFrame's AudioBuffer
//Used by PerformAudioFrame to fill frame's buffer direcly from csound
struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable) IMemoryBufferByteAccess : ::IUnknown
{
	virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

namespace winrt::Csound64UwpComponent::implementation
{
	Csound64::Csound64()
	{
		m_csound = csoundCreate(this);//provide a way back to here from csound for callbacks to work
		if (!m_csound) throw hresult_class_not_available(L"csound64.dll not found");
		csoundCreateMessageBuffer(m_csound, 0);
	}

	Csound64::~Csound64()
	{
		Close();
	}

	void Csound64::Close() noexcept
	{
		if (m_csound)
		{
			csoundSetHostData(m_csound, nullptr); //get rid of reference to this so refcount lowers
			csoundDestroyMessageBuffer(m_csound); //get rid of message buffer structure
			csoundDestroy(m_csound); //let csound release resources
			m_csound = nullptr; //zap our reference so Close() is idempotent
		}
	}

    int32_t Csound64::Version() const noexcept
    {
		return csoundGetVersion();
    }
    int32_t Csound64::ApiVersion() const noexcept
    {
		return csoundGetAPIVersion();
    }

	Csound64UwpComponent::InternalSamplePrecision Csound64::SamplePrecision() const noexcept
	{
		return (csoundGetSizeOfMYFLT() >= 4) ? InternalSamplePrecision::DoublePrecision : InternalSamplePrecision::SinglePrecision;
	}

	uint32_t Csound64::Ksmps() const noexcept
    {
		return (m_csound) ? csoundGetKsmps(m_csound) : 0;
    }
    uint32_t Csound64::Nchnls() const noexcept
    {
		return (m_csound) ? csoundGetNchnls(m_csound) : 0;
	}
    uint32_t Csound64::NchnlsInput() const noexcept
    {
		return (m_csound) ? csoundGetNchnlsInput(m_csound) : 0;
	}
    double Csound64::Sr() const noexcept
    {
		return (m_csound) ? csoundGetSr(m_csound) : 0;
	}
    double Csound64::Kr() const noexcept
    {
		return (m_csound) ? csoundGetKr(m_csound) : 0;
	}
    double Csound64::e0dBFS() const noexcept
    {
		return (m_csound) ? csoundGet0dBFS(m_csound) : 0;
	}
    double Csound64::A4() const noexcept
    {
		return (m_csound) ? csoundGetA4(m_csound) : 0;
	}

    bool Csound64::Debug() const noexcept
    {
		return (m_csound) && (csoundGetDebug(m_csound) != 0);
    }
    void Csound64::Debug(bool value) noexcept
    {
		if (m_csound) csoundSetDebug(m_csound, value ? 1 : 0);
    }

	Csound64UwpComponent::MessageLevels Csound64::MessageLevel() const noexcept
	{
		return (m_csound) ? static_cast<Csound64UwpComponent::MessageLevels>(csoundGetMessageLevel(m_csound)) : MessageLevels::None;
	}

	void Csound64::MessageLevel(Csound64UwpComponent::MessageLevels const& value) noexcept
	{
		if (m_csound) csoundSetMessageLevel(m_csound, static_cast<int>(value));
	}

	bool Csound64::HasMessages() const noexcept
	{
		return (m_csound) ? (csoundGetMessageCnt(m_csound) > 0) : false;
	}

	hstring Csound64::NextMessage(Csound64UwpComponent::MessageAttrs& attr)
	{
		hstring msg = L"";
		if (csoundGetMessageCnt(m_csound) > 0)
		{
			msg = winrt::to_hstring(csoundGetFirstMessage(m_csound));
			auto attributes = csoundGetFirstMessageAttr(m_csound);
			attr = static_cast<Csound64UwpComponent::MessageAttrs>(attributes & 0x0000ffff);
			csoundPopFirstMessage(m_csound);
		}

		return msg;
	}

	Csound64UwpComponent::CsoundParameters Csound64::GetCsoundParameters()
	{
		Csound64UwpComponent::CsoundParameters params{ nullptr };
		params = winrt::make<Csound64UwpComponent::implementation::CsoundParameters>();
		auto pParams = winrt::get_self<CsoundParameters>(params);//get actual implementation to find non-metadata public methods
		pParams->RefreshLocalCsoundParams(m_csound);
		return params;
	}


	bool Csound64::SetSingleParameter(hstring const& argStr)
	{
		auto o = winrt::to_string(argStr);
		return (m_csound) ? (csoundSetOption(m_csound, o.c_str()) == 0) : false;
	}

	void Csound64::SetCsoundParameters(Csound64UwpComponent::CsoundParameters const& params)
	{
		auto p = winrt::get_self< CsoundParameters>(params);
		p->UpdateCsoundParams(m_csound);
	}

	    void Csound64::Reset()
    {
		if (m_csound)
		{
			csoundReset(m_csound);
			csoundSetHostData(m_csound, this); //HostData can be zapped by csoundDestroyMessageBuffer (holdover from old implementation of message buffers)
			csoundCreateMessageBuffer(m_csound, 0);
			csoundSetHostImplementedMIDIIO(m_csound, 1); //zap midi
		}
    }

	Csound64UwpComponent::CsoundStatus Csound64::CompileCsd(hstring const& csd)
	{
		auto c = winrt::to_string(csd);
		auto cc = c.c_str();
		auto r = csoundCompileCsdText(m_csound, cc);
		return static_cast<Csound64UwpComponent::CsoundStatus>(r);

	}

	Csound64UwpComponent::CsoundStatus Csound64::CompileOrc(hstring const& orc)
	{
		auto o = winrt::to_string(orc);
		auto oo = o.c_str();
		auto r = csoundCompileOrc(m_csound, oo);
		return static_cast<Csound64UwpComponent::CsoundStatus>(r);
    }

	double Csound64::EvalCode(hstring const& code)
	{
		auto c = winrt::to_string(code);
		return csoundEvalCode(m_csound, c.c_str());
	}

	Csound64UwpComponent::CsoundStatus Csound64::ReadScore(hstring const& sco)
	{
		auto s = winrt::to_string(sco);
		auto sc = s.c_str();
		auto r = csoundReadScore(m_csound, sc);
		return static_cast<Csound64UwpComponent::CsoundStatus>(r);
	}

	Csound64UwpComponent::CsoundStatus Csound64::Start(int32_t frameCount)
	{
		CsoundStatus ok = CsoundStatus::UnspecifiedError;
		if (m_csound)
		{
			//Call with number for frames, or 0 for default in -b
			csoundSetHostImplementedAudioIO(m_csound, 1, (frameCount > 0) ? frameCount : static_cast<int>(Sr() / 100)); 
			csoundSetHostImplementedMIDIIO(m_csound, 1); //zap midi
			ok = static_cast<CsoundStatus>(csoundStart(m_csound));
		}
		return ok;
	}
	bool Csound64::IsScorePending() const noexcept
	{
		return (m_csound) ? csoundIsScorePending(m_csound) != 0 : false;
	}
	void Csound64::IsScorePending(bool value) noexcept
	{
		if (m_csound) csoundSetScorePending(m_csound, (value) ? 1 : 0);
	}

	double Csound64::ScoreTimeInSeconds() const noexcept
	{
		return (m_csound) ? csoundGetScoreTime(m_csound) : 0.0;
	}

	int64_t Csound64::ScoreTimeInSamples() const noexcept
	{
		return (m_csound) ? csoundGetCurrentTimeSamples(m_csound) : 0;
	}

	double Csound64::ScoreOffsetSeconds() const noexcept
	{
		return (m_csound) ? csoundGetScoreOffsetSeconds(m_csound) : 0.0;
	}
	void Csound64::ScoreOffsetSeconds(double value) noexcept
	{
		if (m_csound) csoundSetScoreOffsetSeconds(m_csound, value);
	}

	void Csound64::InputMessage(hstring const& msg)
	{
		auto m = winrt::to_string(msg);
		auto ms = m.c_str();
		csoundInputMessage(m_csound, ms);
	}

	bool Csound64::PerformKsmps(array_view<double> hostbuf)
	{
		//insure hostbuf has right size for nchnls (nchnls*ksmps), input_nchnls
		//scan for spin also if input_nchnls
		bool done = csoundPerformKsmps(m_csound);
		auto const pSpout = csoundGetSpout(m_csound);
		array_view<double const> spout{pSpout, pSpout+((size_t)Ksmps()*Nchnls())};
		std::copy(spout.cbegin(), spout.cend(), hostbuf.begin());
		//or fill with zero's
		return done;
	}

	int64_t  Csound64::OutputBufferSize()
	{
		return (m_csound) ? csoundGetOutputBufferSize(m_csound) : 0;
	}

	bool Csound64::PerformBuffer(array_view<double> buffer)
	{
		//insure hostbuf has right size for nchnls, input_nchnls; test m_csound
		auto const pBuffer = csoundGetOutputBuffer(m_csound);
		auto done = (pBuffer == 0);
		//scan for input buffer too
		if (!done)
		{
			done = csoundPerformBuffer(m_csound) != 0;
			auto bufsiz = csoundGetOutputBufferSize(m_csound);
			array_view<double const> obuf{ pBuffer, pBuffer + bufsiz };
			std::copy(obuf.cbegin(), obuf.cend(), buffer.begin());
		} //else if no pBuffer?: fill with zeros
		return done;
	}

	bool Csound64::PerformAudioFrame(Windows::Media::AudioFrame const& frame)
	{
		if (!m_csound) throw new hresult_class_not_available(L"Csound instance not found while generating samples");
		//test capacity and csbufsiz are equal, m_csound exists, 
		//Turn csounds output buffer into an array_view<double>
		auto const pCsBuffer = csoundGetOutputBuffer(m_csound); 
		auto done = (pCsBuffer == 0);

		//only proceed if buffer created
		done = csoundPerformBuffer(m_csound) != 0;
		auto csbufsiz = csoundGetOutputBufferSize(m_csound); //find csound buffer size and make a double array from it
		array_view<double const> obuf{ pCsBuffer, pCsBuffer + csbufsiz }; //build an array_view around it for easy access

		//Turn AudioFrame's buffer into an array_view<float> 
		auto audioBuf = frame.LockBuffer(AudioBufferAccessMode::ReadWrite);
		auto pRaw = audioBuf.CreateReference();
		auto pBufferByteAccess = pRaw.as<IMemoryBufferByteAccess>();//be sure we got a valid address
		uint8_t* pBytes{};	//will receive a pointer to first byte in frame's buffer
		uint32_t capacity{};//will receive byte count of frame's buffer
		HRESULT ok = pBufferByteAccess->GetBuffer(&pBytes, &capacity);

		//should we proceed if ok not OK?
		float* pSamples = (float*)pBytes;					//be sure we got a valid address
		auto size = capacity / sizeof(float);				//translate byte count to float count and make a float array from it

		if (static_cast<long>(size) != csbufsiz) throw new hresult_invalid_argument(L"output buffer size of csound and size expected do not match");

		//only proceed if done stays false, else fill buffer with 0's
		array_view<float> samples{ pSamples, pSamples+size };

		//copy csound's doubles into audio frame's floats
		std::transform(obuf.begin(), obuf.end(), samples.begin(), [](double sample) ->float {return static_cast<float>(sample); });

		if (e0dBFS() > 1.0) // Enforce AudioFrame's limit of max float values of 1.0
		{
			float f0dBfs = static_cast<float>(e0dBFS());
			std::transform(samples.cbegin(), samples.cend(), samples.begin(), [f0dBfs](float sample) {return sample / f0dBfs; });
		}
		return done; //frame now has csound's samples as floats in an AudioFrame
	}

	Csound64UwpComponent::CsoundTable Csound64::GetTable(int32_t tableNumber)
	{
		Csound64UwpComponent::CsoundTable table{ nullptr };
		table = winrt::make<Csound64UwpComponent::implementation::CsoundTable>();
		auto pTable = winrt::get_self<CsoundTable>(table);
		pTable->GetTableContents(m_csound, tableNumber);
		return table;
	}

	bool Csound64::UpdateTable(Csound64UwpComponent::CsoundTable const& table)
	{
		bool ok = false;
		auto pTable = winrt::get_self<CsoundTable>(table);
		if (pTable)
		{
			pTable->UpdateTableContents(m_csound);
			ok = true;
		}
		return ok;
	}


	Csound64UwpComponent::CsoundStatus Csound64::ScoreEvent(Csound64UwpComponent::ScoreEventTypes const& type, array_view<double const> p)
	{
		return (m_csound) ? static_cast<CsoundStatus>(csoundScoreEvent(m_csound, static_cast<char>(type), p.data(), p.size())) : CsoundStatus::UnspecifiedError;
	}

	Csound64UwpComponent::CsoundStatus Csound64::ScoreEventAbsolute(Csound64UwpComponent::ScoreEventTypes const& type, array_view<double const> p, double timeOffset)
	{
		return (m_csound) ? static_cast<CsoundStatus>(csoundScoreEventAbsolute(m_csound, static_cast<char>(type), p.data(), p.size(), timeOffset)) : CsoundStatus::UnspecifiedError;
	}

	void Csound64::RewindScore() noexcept
	{
		if (m_csound) csoundRewindScore(m_csound);
	}

	Csound64UwpComponent::CsoundStatus Csound64::KillInstance(double insno, hstring const& insName, Csound64UwpComponent::InstanceKillModes const& mode, bool allowRelease)
	{
		char* name = nullptr;
		if (insName.size() > 0) {
		}
		return static_cast<CsoundStatus>(csoundKillInstance(m_csound, insno, name, static_cast<int>(mode), allowRelease ? 1 : 0));
	}

	void Csound64::Stop() noexcept
    {
		if (m_csound) csoundStop(m_csound);
	}

	Csound64UwpComponent::CsoundStatus Csound64::Cleanup() noexcept
	{
		return (m_csound) ? static_cast<CsoundStatus>(csoundCleanup(m_csound)) : CsoundStatus::UnspecifiedError;
	}
}
// How to recover Csound64 object from csound* via csound callbacks:
//		auto ptr = csoundGetHostData(csound);
//		auto pCs = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
