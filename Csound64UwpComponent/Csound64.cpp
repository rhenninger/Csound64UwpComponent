#include "pch.h"
#include "csound.h"
#include "Csound64.h"
#include "Csound64.g.cpp"
#include <cstdio>
#include <cstring>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include "CsoundControlChannel.h"
#include "CsoundStringChannel.h"
#include "CsoundAudioChannel.h"


using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Devices::Midi;
using namespace winrt::Windows::Storage::Streams;
using namespace std::placeholders;

//Encapsulated interface to get a pointer to the raw memory of an AudioFrame's AudioBuffer
//Used by PerformAudioFrame to fill frame's buffer direcly from csound
struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable) IMemoryBufferByteAccess : ::IUnknown
{
	virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

//Csound midi callbacks for processing midi data in and out
extern "C" int MidiInOpen(CSOUND * csound, void** userdata, const char* devName)
{
	auto ptr = csoundGetHostData(csound);
	auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
	if (!pCsound64) return -1;
	if (!(userdata && devName)) return -2;
	auto name = new std::string(devName);
	if (userdata) *userdata = name;
	pCsound64->OpenMidiDataStream(winrt::to_hstring(devName), 1024);
	return 0;
}

extern "C" int MidiRead(CSOUND* csound, void* userdata, unsigned char* buf, int nBytes)
{
	auto ptr = csoundGetHostData(csound);
	auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
	if (!(pCsound64 && userdata)) return -1;
	auto b = winrt::array_view<uint8_t>(buf, buf+nBytes);
	auto devName = reinterpret_cast<std::string*>(userdata);
	int cnt = pCsound64->ReceiveMidiData(winrt::to_hstring(devName->data()), b);
	return cnt;
}

extern "C" int MidiInClose(CSOUND* csound, void* userdata)
{
	if (userdata)
	{
		auto ptr = csoundGetHostData(csound);
		auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
		if (!(pCsound64 && userdata)) return -1;
		auto devName = static_cast<std::string*>(userdata);
		pCsound64->CloseMidiDataStream(winrt::to_hstring(devName->data()));
		delete devName;
	}
	return 0;
}

extern "C" int MidiOutOpen(CSOUND * csound, void** userdata, const char* devName)
{
	auto ptr = csoundGetHostData(csound);
	auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
	if (!pCsound64) return -1;
	if (!(userdata && devName)) return -2;
	auto name = new std::string(devName);
	if (userdata) *userdata = name;
	pCsound64->OpenMidiDataStream(winrt::to_hstring(devName), 0); //maybe not needed for writing
	return 0;
}

extern "C" int MidiWrite(CSOUND * csound, void* userdata, const unsigned char* buf, int nBytes)
{
	auto ptr = csoundGetHostData(csound);
	auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
	if (!(pCsound64 && userdata)) return -1;
	auto b = winrt::array_view<uint8_t const>(buf, buf + nBytes);
	auto devName = reinterpret_cast<std::string*>(userdata);
	pCsound64->RaiseMidiDataAvailable(winrt::to_hstring(devName->data()), b);
	return nBytes;
}

extern "C" int MidiOutClose(CSOUND * csound, void* userdata)
{
	if (userdata)
	{
		auto ptr = csoundGetHostData(csound);
		auto pCsound64 = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
		if (!(pCsound64 && userdata)) return -1;
		auto devName = static_cast<std::string*>(userdata);
		pCsound64->CloseMidiDataStream(winrt::to_hstring(devName->data())); //maybe not needed for writing
		delete devName;
	}
	return 0;
}

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
			DetachFromCsound();
			csoundDestroy(m_csound); //let csound release resources
			m_csound = nullptr; //zap our reference so Close() is idempotent
		}
	}

	void Csound64::DetachFromCsound() //closes items we use attached to csound
	{
		for (auto stmt : m_midiStreams) CloseMidiDataStream(winrt::to_hstring(stmt.first));
		csoundSetHostData(m_csound, nullptr); //get rid of reference to this so refcount lowers
		csoundDestroyMessageBuffer(m_csound); //get rid of message buffer structure
	}



	winrt::event_token Csound64::MidiDataAvailable(Windows::Foundation::TypedEventHandler<Csound64UwpComponent::Csound64, Csound64UwpComponent::MidiDataAvailableEventArgs> const& handler)
	{
		return m_midiDataAvailable.add(handler);
	}
	void Csound64::MidiDataAvailable(winrt::event_token const& token) noexcept
	{
		m_midiDataAvailable.remove(token);
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
    uint32_t Csound64::Nchnls_i() const noexcept
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

	int64_t  Csound64::OutputBufferSize() const noexcept
	{
		return (m_csound) ? csoundGetOutputBufferSize(m_csound) : 0;
	}

	int64_t Csound64::InputBufferSize() const noexcept
	{
		return (m_csound) ? csoundGetInputBufferSize(m_csound) : 0;
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
			DetachFromCsound(); //call common routines for releasing memory we shared with csound
			csoundReset(m_csound);
			csoundSetHostData(m_csound, this); //HostData can be zapped by csoundDestroyMessageBuffer (holdover from old implementation of message buffers)
			csoundCreateMessageBuffer(m_csound, 0); //rebuild message buffer
			AttachMidiCallbacks(); //re-attach midi callbacks in case they got zapped in csoundReset()
			//save Audio declaration to Start() when outputbuffer size is known
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
			AttachMidiCallbacks(); //make sure midi is attached
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

	bool Csound64::PerformKsmps(array_view<double> hostbuf, array_view<double const> spin)
	{
		if (!m_csound) throw new hresult_class_not_available(L"Csound instance not found while generating samples");

		//if there are samples to transfer into csound's engine for processing, move them into csounds spin buffer prior to processing
		if ((Nchnls_i() > 0) && spin.data() && (spin.size() == (Ksmps() * Nchnls_i())))
		{
			auto pSpin = csoundGetSpin(m_csound);
			if (pSpin)
			{
				array_view<double> spinbuf{ pSpin, pSpin + ((size_t)Ksmps() * Nchnls_i()) };
				std::copy(spin.cbegin(), spin.cend(), spinbuf.begin());
			}
		}

		//Calculate the new samples and transfer them into the host's provided array
		bool done = csoundPerformKsmps(m_csound);
		if (!done)
		{
			auto const pSpout = csoundGetSpout(m_csound);
			array_view<double const> spout{ pSpout, pSpout + ((size_t)Ksmps() * Nchnls()) };
			if (spout.size() == hostbuf.size())
				std::copy(spout.cbegin(), spout.cend(), hostbuf.begin());
			else
				std::copy_n(spout.begin(), std::min(spout.size(), hostbuf.size()), hostbuf.begin());
		}
		else //or fill with zero's if done already...
		{
			std::fill(hostbuf.begin(), hostbuf.end(), 0.0);
		}
		return done;
	}


	bool Csound64::PerformBuffer(array_view<double> buffer, array_view<double const> spin)
	{
		if (!m_csound) throw new hresult_class_not_available(L"Csound instance not found while generating samples");

		//Fill csound's input buffer if there are incoming samples to forward to it prior to processing
		if ((Nchnls_i() > 0) && spin.data() && (spin.size()))
		{
			auto insize = InputBufferSize();
			if (insize && (insize == spin.size()))
			{
				auto pInbuf = csoundGetInputBuffer(m_csound);
				array_view<double> spinbuf{ pInbuf, pInbuf+insize};
				std::copy(spin.cbegin(), spin.cend(), spinbuf.begin());
			}
		}
		
		//If we are still active, set up transfer of samples from output buffer to host's array
		auto const pBuffer = csoundGetOutputBuffer(m_csound);
		auto done = (pBuffer == 0);
		if (!done)
		{
			done = csoundPerformBuffer(m_csound) != 0;
			auto bufsiz = csoundGetOutputBufferSize(m_csound);
			array_view<double const> obuf{ pBuffer, pBuffer + bufsiz };
			if (buffer.size() == obuf.size())
				std::copy(obuf.cbegin(), obuf.cend(), buffer.begin());
			else
				std::copy_n(obuf.cbegin(), std::min(buffer.size(), obuf.size()), buffer.begin());
		} 
		else //if no pBuffer?: fill with zeros
		{
			std::fill(buffer.begin(), buffer.end(), 0.0);
		}
		return done;
	}

	bool Csound64::PerformAudioFrame(Windows::Media::AudioFrame const& frame, Windows::Media::AudioFrame const& inFrame)
	{
		if (!m_csound) throw new hresult_class_not_available(L"Csound instance not found while generating samples");
		uint8_t* pBytes{};	//will receive a pointer to first byte in an AudioFrame's buffer
		uint32_t capacity{};//will receive byte count of an AudioFrame's buffer
		HRESULT ok = S_OK; //will receive error code from buffer access attempts
		float* pSamples{};//will hold pBytes recast as a pointer to floats: the size of samples in an AudioFrame
		uint64_t size{}; //will hold size of a buffer in floats

		//If there are samples in the input frame, transfer these into csounds input buffer for processing
		auto csInbufSize = InputBufferSize();
		auto pCsInBuffer = csoundGetInputBuffer(m_csound);
		if ((Nchnls_i() > 0) && (inFrame) && (pCsInBuffer) && (csInbufSize > 0))
		{
			//unpack inFrameand copy into csound's InputBuffer
			auto inbuf = inFrame.LockBuffer(AudioBufferAccessMode::Read);
			auto pInRaw = inbuf.CreateReference();
			auto pInBufByteAccess = pInRaw.as<IMemoryBufferByteAccess>();//be sure we got a valid address
			ok = pInBufByteAccess->GetBuffer(&pBytes, &capacity);
			if (ok == S_OK)
			{
				pSamples = (float*)pBytes;					//be sure we got a valid address in float boundaries
				size = capacity / sizeof(float);			//translate byte count to float count and make a float array from it
				array_view<const float> sampsin{ pSamples, pSamples + size };//frame AudioFrame's buffer in an array_view

				array_view<double> ibuf{ pCsInBuffer, pCsInBuffer + csInbufSize }; //build an array_view over csound's inputbuffer

				std::transform(sampsin.cbegin(), sampsin.cend(), ibuf.begin(), [](float sampin) ->double {return sampin; });
			}
		}

		//Make output frame's buffer look like a memory block so csound will move its output samples into it.
		//test capacity and csbufsiz are equal, turn csounds output buffer into an array_view<double>
		auto const pCsBuffer = csoundGetOutputBuffer(m_csound); 
		auto done = (pCsBuffer == 0);

		//only proceed if buffer created
		done = csoundPerformBuffer(m_csound) != 0;
		auto csbufsiz = OutputBufferSize(); //find csound buffer size and make a double array from it
		array_view<double const> obuf{ pCsBuffer, pCsBuffer + csbufsiz }; //build an array_view around it for easy access

		//Turn AudioFrame's buffer into an array_view<float> 
		auto audioBuf = frame.LockBuffer(AudioBufferAccessMode::ReadWrite);
		auto pRaw = audioBuf.CreateReference();
		auto pBufferByteAccess = pRaw.as<IMemoryBufferByteAccess>();//be sure we got a valid address

		ok = pBufferByteAccess->GetBuffer(&pBytes, &capacity);
		//Highly unlikely, but cannot proceed if ok not S_OK, something is very wrong if this occurs, so give up
		if (ok != S_OK) throw new hresult_error(ok, L"Unable to access audio buffer during PerformAudioFrame");

		pSamples = (float*)pBytes;					//be sure we got a valid address
		size = capacity / sizeof(float);			//translate byte count to float count and make a float array from it

		if (static_cast<long>(size) != csbufsiz) throw new hresult_invalid_argument(L"output buffer size of csound and size expected do not match");

		array_view<float> samples{ pSamples, pSamples+size };

		//copy csound's doubles into audio frame's floats, but only transfer while done stays false, else fill buffer with 0's
		std::transform(obuf.begin(), obuf.end(), samples.begin(), [](double sample) ->float {return static_cast<float>(sample); });

		if (e0dBFS() > 1.0) // Enforce AudioFrame's limit of max float values of 1.0
		{
			float f0dBfs = static_cast<float>(e0dBFS());
			std::transform(samples.cbegin(), samples.cend(), samples.begin(), [f0dBfs](float sample) {return sample / f0dBfs; });
		}
		return done; //frame now has csound's samples as floats in an AudioFrame
	}

	bool Csound64::HasTable(int32_t tableNumber) const noexcept
	{
		double* tblPtr = nullptr;//dummy to receive pointer to table data in csound which we don't need for this method
		return (m_csound != nullptr) ? (csoundGetTable(m_csound, &tblPtr, tableNumber) >= 0) : false;
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

	bool Csound64::HasChannels()
	{
		controlChannelInfo_t* channelInfos{ nullptr };
		int chnlcnt = csoundListChannels(m_csound, &channelInfos);
		if (chnlcnt > 0) csoundDeleteChannelList(m_csound, channelInfos);
		return chnlcnt > 0;
	}

	Windows::Foundation::Collections::IMapView<hstring, Csound64UwpComponent::ChannelType> Csound64::ListChannels()
	{
		std::unordered_map<winrt::hstring, winrt::Csound64UwpComponent::ChannelType> infos{};
		controlChannelInfo_t* channelInfos{ nullptr };
		int chnlcnt = csoundListChannels(m_csound, &channelInfos);
		if ((chnlcnt > 0) && channelInfos)
		{
			for (int i = 0; i < chnlcnt; ++i)
			{
				auto info = channelInfos[i];
				auto name = winrt::to_hstring(info.name);
				auto ct = static_cast<ChannelType>(info.type & 7);
				infos.emplace(name, ct);
			}
			csoundDeleteChannelList(m_csound, channelInfos);
		}
		Windows::Foundation::Collections::IMap<hstring, ChannelType> view{ winrt::single_threaded_map(std::move(infos)) };
		return view.GetView();
	}

	Csound64UwpComponent::ICsoundChannel Csound64::GetChannel(Csound64UwpComponent::ChannelType const& type, hstring const& name, bool isInput, bool isOutput)
	{
		Csound64UwpComponent::ICsoundChannel channel{ nullptr };
		switch (type)
		{
		case Csound64UwpComponent::ChannelType::ControlChannel:
		{
			channel = winrt::make<Csound64UwpComponent::implementation::CsoundControlChannel>();
			auto pChannel = winrt::get_self<CsoundControlChannel>(channel);
			pChannel->DeclareChannelParameters(m_csound, name, isInput, isOutput);
		}
		break;
		case Csound64UwpComponent::ChannelType::StringChannel:
		{
			channel = winrt::make<Csound64UwpComponent::implementation::CsoundStringChannel>();
			auto pChannel = winrt::get_self<CsoundStringChannel>(channel);
			pChannel->DeclareChannelParameters(m_csound, name, isInput, isOutput);
		}
		break;
		case Csound64UwpComponent::ChannelType::AudioChannel:
		{
			channel = winrt::make<Csound64UwpComponent::implementation::CsoundAudioChannel>();
			auto pChannel = winrt::get_self<CsoundAudioChannel>(channel);
			pChannel->DeclareChannelParameters(m_csound, name, isInput, isOutput);
		}
		break;
		case Csound64UwpComponent::ChannelType::PVSChannel:
		default:
			break;
		}
		return channel;
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

	int32_t Csound64::OpenMidiDataStream(hstring const& devicename, int32_t bufferSize)
	{
		auto name = winrt::to_string(devicename);
		if (m_midiStreams.contains(name) && (m_midiStreams[name].second != nullptr))
		{
			auto circbuf = m_midiStreams[name];
			std::lock_guard<std::mutex> guard(*circbuf.first);
			if (bufferSize == 0)
			{
				csoundFlushCircularBuffer(m_csound, circbuf.second);//keep current size
			} else
			{ 
				csoundDestroyCircularBuffer(m_csound, circbuf.second);
				m_midiStreams[name].second = nullptr;
			}
		} 
		if (!m_midiStreams.contains(name) || (m_midiStreams[name].second == nullptr))
		{
			auto size = (bufferSize > 0) ? bufferSize : 1024;
			auto buf = csoundCreateCircularBuffer(m_csound, size, sizeof(uint8_t));	
			if (!buf) return -3;
			m_midiStreams.emplace(winrt::to_string(devicename), std::pair<std::mutex*, void*>( new std::mutex, buf));
		}
		return 0;
	}

	void Csound64::AttachMidiCallbacks()
	{
		csoundSetHostImplementedMIDIIO(m_csound, 1); //zap internal midi devices
		csoundSetExternalMidiInOpenCallback(m_csound, MidiInOpen);
		csoundSetExternalMidiReadCallback(m_csound, MidiRead);
		csoundSetExternalMidiInCloseCallback(m_csound, MidiInClose);
		csoundSetExternalMidiOutOpenCallback(m_csound, MidiOutOpen);
		csoundSetExternalMidiWriteCallback(m_csound, MidiWrite);
		csoundSetExternalMidiOutCloseCallback(m_csound, MidiOutClose);
	}


	int32_t Csound64::AppendMidiData(hstring const& deviceName, array_view<uint8_t const> data)
	{
		auto name = winrt::to_string(deviceName);
		if (!m_midiStreams.contains(name) || (m_midiStreams[name].second == nullptr)) return -1;
		auto circbuf = m_midiStreams[name];
		std::lock_guard<std::mutex> guard(*circbuf.first);
		return csoundWriteCircularBuffer(m_csound, circbuf.second, data.data(), data.size());
	}

	int32_t Csound64::ReceiveMidiData(hstring const& deviceName, array_view<uint8_t> data)
	{
		auto name = winrt::to_string(deviceName);
		if (!m_midiStreams.contains(name) || (m_midiStreams[name].second == nullptr)) return -1;
		auto circbuf = m_midiStreams[name];
		std::lock_guard<std::mutex> guard(*circbuf.first);
		auto cnt = csoundReadCircularBuffer(m_csound, circbuf.second, data.data(), data.size());
		return cnt;
	}
	void Csound64::RaiseMidiDataAvailable(hstring const& deviceName, array_view<uint8_t const> data)
	{
		auto args = MidiDataAvailableEventArgs();
		args.LoadMidiData(deviceName, data);//populate a MidiDataAvailableEventArgs object
		m_midiDataAvailable(*this, args);//and throw the MidiDataAvailable event to anyone who cares
	}

	int32_t Csound64::CloseMidiDataStream(hstring const& deviceName)
	{
		auto name = winrt::to_string(deviceName);
		if (m_midiStreams.contains(name) && (m_midiStreams[name].second != nullptr))
		{
			auto circbuf = m_midiStreams[name];
			std::lock_guard<std::mutex> guard(*circbuf.first);
			csoundDestroyCircularBuffer(m_csound, circbuf.second);//csound releases circbuf.second, pointer only shared here
			m_midiStreams.erase(name);
		}
		return 0;
	}

}
// How to recover Csound64 object from csound* via csound callbacks:
//		auto ptr = csoundGetHostData(csound);
//		auto pCs = reinterpret_cast<winrt::Csound64UwpComponent::implementation::Csound64*>(ptr);
