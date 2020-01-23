# Csound64UwpComponent
Provides the ability to access 64-bit [*Csound*](https://csound.com) as a
[Windows 10 UWP](https://docs.microsoft.com/en-us/windows/uwp/get-started/universal-application-platform-guide)
component.  This makes it possible to control *Csound* from a pure
Windows 10 based program using the new [audio](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/audio-graphs)
and [midi](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/midi) capabilities
available now in Windows 10.  As a Windows 10 component
and thus a set of [*COM*](https://en.wikipedia.org/wiki/Component_Object_Model)
objects, *Csound* can now be accessed from any Windows 10 complient
language be it *javascript*, *c++*, or any *.net* language such as *C#*, *VB* or *F#*.

It is presumed that you would use this code in a UWP project that you are developing,
so installation of this tool consists of downloading this github solution,
building it in Visual Studio, and then referencing it from your own projects.
The point of this component is to run audio, file access, user interaction,
and midi from your Windows 10 compliant universal app.
As such, this component bypasses *Csound*'s own audio, midi, messaging, and file processing
using Windows 10's own audio, midi, storage and UI facilities directly from your app.

#### Tools Used:
The component itself is written using the
[**c++/WinRT**](https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/)
tools which allows a seamless flow between *csound*'s *c*-based
[*API*](https://csound.com/docs/api/index.html)
and the Windows 10 [*ABI*](https://en.wikipedia.org/wiki/Application_binary_interface).
*Csound*'s attributes are projected as object properties,
callbacks are projected as events, and functions as object methods.
Windows 10's "AudioGraph", having its own audio thread,
simplifies the multi-threading programming usually needed to
host *Csound* interactively in an application.  It uses the Windows 10 "AudioFrame" object
to effortlessly and safely share audio memory between *Csound*
and a host program - even from *.net*'s managed environment.

There are no dependencies beyond *Csound* itself which must be installed separately.
The entire project is developed in the
["Csound64.idl"](https://docs.microsoft.com/en-us/uwp/midl-3/) file.
This is then used to generate a Windows 10 metadata file: "Csound64.winmd"
as well as signatures to use in the *c++* headers and source files used to develop these components.
The build's resulting ".dll", ".lib",".winmd" along with *Csound*'s "csound64.dll"
file (assuming a shell path variable that points to *csound*'s "/bin" directory) is enough
to start programming a UWP app using *Csound* for its audio engine.

 
#### Installation:
1. The 64-bit Windows version of [*Csound 6.13*](https://csound.com/download.html) or later must be installed and set up.  Using the default installer is sufficient including its setting of standard environment variables including "path".
2. You should have Windows 10, version 1903 or later installed and its corresponding SDK. 
3. [*Visual Studio 2019*](https://visualstudio.microsoft.com/) version 16.4.3 or higher must be installed. The community edition is fine.
4. *Visual C++* along with the [*C++/WinRT*](https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/get-started) component of Visual Studio must be included via the Visual Studio Installer.  Neither of these is installed by default.
5. Download this github project to **Visual Studio 2019** and open the "Csound64UwpComponent.sln" file.
6. Build the solution.  Once built, you can reference the component in your own projects.
7. Any executable you build should have a copy of *Csound*'s csound64.dll in its APPX directory.
8. If *Csound*'s ".bin" directory is discoverable (via the "path" environemnt variable, typically), you should be good to go.
  
#### Using the components:

The *C#* unit test app included in the github download shows some simple ways to use
the "Csound64" uwp component.  This test app is not meant to be released with any app you create.
Look at the little "CsoundRunner" class in this file to see a bare bones way to use 
the component for realtime audio and audio file creation.

Instantiating the "Csound64UwpComponent.Csound64" object will connect you to *Csound*
and give you access to the properties and methods that interact with *Csound*.
This object also serves as a factory to give you access to other objects which represent
*Csound* own data structures such as its parameter, table and channel structures.
There is an event for which you can provide a handler to when you want to receive
"midi out" data.
*Csound* is meant to be accessed via an
[AudioFrameInputNode](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/audio-graphs#audio-frame-input-node)
object through its "QuantumStarted" event.

#### License:
This component is free and without warranty.  It is licensed under the same terms as *Csound*
itself: the [**GNU Lesser General Public License**](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
