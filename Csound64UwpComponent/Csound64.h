#pragma once
#include "Csound64.g.h"

#include "csound.h"
#include "CsoundParameters.h"
#include "CsoundTable.h"
#include "MidiDataAvailableEventArgs.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <utility>

/*
	This file provides a Windows 10 UWP component to access Csound via its Csound API.
	It is designed to facilitate use in Windows 10 Apps whiching wish to use the new
	Windows 10  Audio and Midi API's.  It is hard coded to use 64-bit CSOUND: csound64.dll.
	The method signatures and COM bridge code for this component were generated
	from Microsoft MIDL 3.0 file 'csound64.idl via C++/WinRT tools.

	This UWP component is copyright (C) 2019 by Richard Henninger under the same license 
	and terms as Csound itself as stated below.
	
	Csound is copyright (C) 2003 2005 2008 2013 by John ffitch, Istvan Varga,
												   Mike Gogins, Victor Lazzarini,
												   Andres Cabrera, Steven Yi
 
	The Csound Library is free software; you can redistribute it
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	Csound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with Csound; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
	02110-1301 USA
*/

namespace winrt::Csound64UwpComponent::implementation
{
    struct Csound64 : Csound64T<Csound64>
    {
        Csound64();
		virtual ~Csound64();
		void Close() noexcept;

        int32_t Version() const noexcept;
        int32_t ApiVersion() const noexcept;
		Csound64UwpComponent::InternalSamplePrecision SamplePrecision() const noexcept;

        uint32_t Ksmps() const noexcept;
		uint32_t Nchnls() const noexcept;
		uint32_t Nchnls_i() const noexcept;
        double Sr() const noexcept;
        double Kr() const noexcept;
        double e0dBFS() const noexcept;
        double A4() const noexcept;

        bool Debug() const noexcept;
        void Debug(bool value) noexcept;
		Csound64UwpComponent::MessageLevels MessageLevel() const noexcept;
		void MessageLevel(Csound64UwpComponent::MessageLevels const& value) noexcept;
		bool HasMessages() const noexcept;
		hstring NextMessage(Csound64UwpComponent::MessageAttrs& attr);

        void Reset();
		Csound64UwpComponent::CsoundStatus CompileCsd(hstring const& csd);
		Csound64UwpComponent::CsoundStatus CompileOrc(hstring const& orc);
		double EvalCode(hstring const& code);
		Csound64UwpComponent::CsoundStatus ReadScore(hstring const& sco);

		Csound64UwpComponent::CsoundStatus Start(int32_t frameCount);//how know -b, bools for windows audio/midi
		bool IsScorePending() const noexcept;
		void IsScorePending(bool value) noexcept;
		double ScoreTimeInSeconds() const noexcept;
		int64_t ScoreTimeInSamples() const noexcept;
		double ScoreOffsetSeconds() const noexcept;
		void ScoreOffsetSeconds(double value) noexcept;

		bool PerformKsmps(array_view<double> spout, array_view<double const> spin);
		int64_t  OutputBufferSize() const noexcept;
		int64_t InputBufferSize() const noexcept;
		bool PerformBuffer(array_view<double> buffer, array_view<double const> spin);
		bool PerformAudioFrame(Windows::Media::AudioFrame const& frame, Windows::Media::AudioFrame const& inFrame);

		bool HasTable(int32_t tableNumber) const noexcept;
		Csound64UwpComponent::CsoundTable GetTable(int32_t tableNumber);
		bool UpdateTable(Csound64UwpComponent::CsoundTable const& table);

		bool HasChannels();
		Windows::Foundation::Collections::IMapView<hstring, Csound64UwpComponent::ChannelType> ListChannels();
		Csound64UwpComponent::ICsoundChannel GetChannel(Csound64UwpComponent::ChannelType const& type, hstring const& name, bool isInput, bool isOutput);

		int32_t OpenMidiDataStream(hstring const& devicename, int32_t bufferSize);
		int32_t AppendMidiData(hstring const& deviceName, array_view<uint8_t const> data);
		int32_t ReceiveMidiData(hstring const& deviceName, array_view<uint8_t> data);
		int32_t CloseMidiDataStream(hstring const& deviceName);
		void RaiseMidiDataAvailable(hstring const& deviceName, array_view<uint8_t const> data);

		winrt::event_token MidiDataAvailable(Windows::Foundation::TypedEventHandler<Csound64UwpComponent::Csound64, Csound64UwpComponent::MidiDataAvailableEventArgs> const& handler);
		void MidiDataAvailable(winrt::event_token const& token) noexcept;

		void InputMessage(hstring const& msg);
		Csound64UwpComponent::CsoundStatus ScoreEvent(Csound64UwpComponent::ScoreEventTypes const& type, array_view<double const> p);
		Csound64UwpComponent::CsoundStatus ScoreEventAbsolute(Csound64UwpComponent::ScoreEventTypes const& type, array_view<double const> p, double timeOffset);
		Csound64UwpComponent::CsoundStatus KillInstance(double insno, hstring const& insName, Csound64UwpComponent::InstanceKillModes const& mode, bool allowRelease);

		void RewindScore() noexcept;
        void Stop() noexcept;
		Csound64UwpComponent::CsoundStatus Cleanup() noexcept;

		Csound64UwpComponent::CsoundParameters GetCsoundParameters();
		bool SetSingleParameter(hstring const& argStr);
		void SetCsoundParameters(Csound64UwpComponent::CsoundParameters const& params);

		/* Internal routines not visible via metadata*/
		void DetachFromCsound(); //closes items we use attached to csound
		void AttachMidiCallbacks();
	private:
		CSOUND* m_csound{};
		winrt::event<Windows::Foundation::TypedEventHandler<Csound64UwpComponent::Csound64, Csound64UwpComponent::MidiDataAvailableEventArgs>> m_midiDataAvailable;
		std::unordered_map<std::string, std::pair<std::mutex*, void*>> m_midiStreams{};
    };
}
namespace winrt::Csound64UwpComponent::factory_implementation
{
    struct Csound64 : Csound64T<Csound64, implementation::Csound64>
    {
    };
}
