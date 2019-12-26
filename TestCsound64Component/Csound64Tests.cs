
using System;
using System.Text;
using System.Runtime.InteropServices;
using Windows.Media;
using System.Threading.Tasks;
//using System.Diagnostics;
using Windows.Foundation;
using Windows.Storage;
using Windows.Media.Audio;
using Windows.Media.MediaProperties;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Csound64UwpComponent;
using Windows.UI.Core;
using Windows.ApplicationModel.Core;
using Windows.Media.Render;

[ComImport]
[Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
unsafe interface IMemoryBufferByteAccess
{
    void GetBuffer(out byte* buffer, out uint capacity);
}
namespace TestCsound64Component
{
    [TestClass]
    public class TestCsound64
    {
        [TestMethod]
        public void TestCsound64Defaults()
        {
            var cs = new Csound64();
            Assert.IsNotNull(cs);
            Assert.IsTrue(cs.Version > 6000);
            Assert.IsTrue(cs.ApiVersion >= 400);
            Assert.AreEqual(440.0, cs.A4);
            Assert.AreEqual(44100.0, cs.Sr);
            Assert.AreEqual(10U, cs.Ksmps);
            Assert.AreEqual(cs.Kr, (cs.Sr / cs.Ksmps));
            Assert.AreEqual(32768, cs.e0dBFS);
            Assert.AreEqual(1U, cs.Nchnls);
            Assert.AreEqual(1U, cs.NchnlsInput);
            Assert.IsFalse(cs.Debug);
            cs.Debug = true;
            Assert.IsTrue(cs.Debug);
            cs.Debug = false;
            Assert.IsFalse(cs.Debug);

            var parms = cs.GetCsoundParameters();
            Assert.IsNotNull(parms);
            Assert.AreEqual(135, (int)parms.MessageLevel);
            parms.MessageLevel = MessageLevels.Amplitudes | MessageLevels.RangeViolations | MessageLevels.Warnings;
            parms.SampleRateOverride = 48000.0;
            parms.e0dbfsOverride = 1.0;
            parms.BufferFrames = 480; //tell csound to make buffer 480 frames long
            cs.SetCsoundParameters(parms);
            var parms2 = cs.GetCsoundParameters();
            Assert.IsNotNull(parms2);
            Assert.AreEqual(7, (int)parms2.MessageLevel);
            Assert.AreEqual(48000.0, parms.SampleRateOverride);
            Assert.AreEqual(44100.0, cs.Sr); //doesn't take effect until compile
            Assert.IsTrue(cs.SetSingleParameter("-m 4"));
            Assert.AreEqual(4, (int)cs.MessageLevel);
            var obsiz = cs.OutputBufferSize;
            var orc = "instr 1 \n asin oscil 0dbfs/4, p4 \n out asin \nendin\n";
            //orc = "instr 1 \n a1 rand 0dbfs/4 \n out a1 \n endin\n";
            var r = cs.CompileOrc(orc);
            Assert.IsTrue(r == 0, TestCsound64.GetMessages(cs));
            Assert.AreEqual(48000.0, cs.Sr); //now parms override has taken effect
            var score = "i1 0 .25 480 .25 -1 \ne \n";
            r = cs.ReadScore(score);
            Assert.AreEqual(480, cs.OutputBufferSize); //before start
            var rs = cs.Start((int)cs.OutputBufferSize); //use OutputBufferFrames set by parameters
            Assert.AreEqual(480, cs.OutputBufferSize); //after start
            double[] samps = new double[cs.Ksmps * cs.Nchnls];
            Assert.AreEqual(0.0, cs.ScoreTimeInSeconds);
            for (var i=0; i<48; ++i)
            {
                Assert.IsFalse(cs.PerformKsmps(samps));
                Assert.IsTrue(cs.ScoreTimeInSeconds > 0.0);
            }

            var bsiz = cs.OutputBufferSize;
            double[] bsamps = new double[bsiz*cs.Nchnls];
            while (!cs.PerformBuffer(bsamps))
            {
                Assert.IsTrue(cs.ScoreTimeInSamples > 0);
            }
            Assert.AreEqual(0.25, cs.ScoreTimeInSeconds);
            var msg = TestCsound64.GetMessages(cs);
            Assert.IsTrue(msg.Length > 0);

        }

        [TestMethod]
        unsafe public void TestCs64PerformAudioFrame()
        {
            var cs = new Csound64();
            Assert.IsNotNull(cs);
            cs.SetSingleParameter("-b 480");
            var orc = "sr=48000\nksmps=10\nnchnls=1\n0dbfs=1.0\n\ninstr 1 \n asin oscil 0dbfs/4, p4 \n out asin \nendin\n";
            //orc = "instr 2 \n a1 rand 0dbfs/4 \n out a1 \n endin\n";
            Assert.IsTrue(CsoundStatus.OK == cs.CompileOrc(orc));
            var score = "i1 0 .25 480 .25 -1 \ne \n";
            Assert.IsTrue(cs.ReadScore(score) == CsoundStatus.OK);
            Assert.AreEqual(480, cs.OutputBufferSize); //confirm -b worked
            Assert.IsTrue(cs.Start((int)cs.OutputBufferSize) == CsoundStatus.OK);
            var bsiz = cs.OutputBufferSize; //-b is still used as a default
            Assert.AreEqual(480, bsiz);
            float[] bsamps = new float[bsiz];
            AudioFrame samps = new AudioFrame((uint)(bsiz * cs.Nchnls * sizeof(float)));
            while (!cs.PerformAudioFrame(samps))
            {
                var buffer = samps.LockBuffer(AudioBufferAccessMode.Read);
                IMemoryBufferReference reference = buffer.CreateReference();
                ((IMemoryBufferByteAccess)reference).GetBuffer(out byte* dataInBytes, out uint capacityInBytes);
                float* dataInFloat = (float*)dataInBytes;
                for (var i = 0; i < bsiz; ++i) bsamps[i] = dataInFloat[i];
                
                samps = new AudioFrame((uint)(bsiz * cs.Nchnls * sizeof(float)));

            }
            Assert.IsTrue(cs.ScoreTimeInSamples >= 0.25);

            cs.Stop();

            var msgs = new StringBuilder();
            //          MessageAttrs atts;
            while (cs.HasMessages())
            {
                msgs.Append(cs.NextMessage(out MessageAttrs atts));
                var x = atts;
            }
            var msg = msgs.ToString();
        }

        [TestMethod]
        public void TestCsoundTable()
        {
            var cs = new Csound64();
            var orc = "instr 1 \n asin oscil p5, p4, p6 \n out asin \nendin\n";
            var r = cs.CompileOrc(orc);
            Assert.IsTrue(r == 0);
            Assert.AreEqual(44100.0, cs.Sr); //defaults are in effect
            Assert.AreEqual(10U, cs.Ksmps);
            var score = "f1 0 1024 10 1 \ni1 0 .25 480 .25 1 \ne \n";
            r = cs.ReadScore(score);
            var rs = cs.Start(0); //default sr/1000
         //   Assert.AreEqual(441, cs.OutputBufferSize); //default works?
            double[] samps = new double[cs.Ksmps * cs.Nchnls];
            Assert.IsFalse(cs.PerformKsmps(samps));

            var f1 = cs.GetTable(1);
            Assert.IsTrue(f1.Size > 0);
            Assert.AreEqual(1, f1.Number);
            Assert.AreEqual(1024, f1.Size);
            Assert.AreEqual(10, f1.GenNbr);
            Assert.IsTrue(f1.IsNormalized);
            var data = f1.Contents;
            Assert.AreEqual(f1.Size, data.Length);
            var parms = f1.Parameters;
            Assert.AreEqual(2, parms.Length);
            Assert.AreEqual(1, parms[1]);

            cs.InputMessage("f4 0 -9 -2 0 2 10 100 0");
            Assert.IsFalse(cs.PerformKsmps(samps));
            var f4 = cs.GetTable(4);
            Assert.IsTrue(f4.Size >= 0);
            Assert.AreEqual(4, f4.Number);
            Assert.AreEqual(9, f4.Size);
            Assert.AreEqual(2, f4.GenNbr);
            Assert.IsFalse(f4.IsNormalized);
            var f4dat = f4.Contents;
            Assert.AreEqual(f4.Size, f4dat.Length); 
            var f4prm = f4.Parameters;
            Assert.AreEqual(6, f4prm.Length);

            double[] f3parms = new double[] { 3, 0, 128, 16, 1, 128, 1, 0};
            var result = cs.ScoreEvent(ScoreEventTypes.Function, f3parms);
            Assert.IsTrue(r == 0);
            Assert.IsFalse(cs.PerformKsmps(samps));
            var f3 = cs.GetTable(3);
            Assert.IsTrue(f3.Size >= 0);
            Assert.AreEqual(3, f3.Number);
            Assert.AreEqual(128, f3.Size);
            Assert.AreEqual(16, f3.GenNbr);
            Assert.IsTrue(f3.IsNormalized);
            var f3dat = f3.Contents;
            Assert.AreEqual(f3.Size, f3dat.Length);
            var f3prm = f3.Parameters;
            Assert.AreEqual(5, f3prm.Length);

            var msg = new StringBuilder();
            MessageAttrs attrs;
            while (cs.HasMessages()) { msg.Append(cs.NextMessage(out attrs)); }
            var m = msg.ToString();

        }

        [TestMethod]
        public void TestCsound64Expressions()
        {
            var cs = new Csound64();
            var r = cs.EvalCode("i1=2 + 2 \n return i1\n");
            Assert.AreEqual(4, r);
            r = cs.EvalCode("i1=cpspch(8.09)\nreturn i1");
            Assert.AreEqual(440.0, Math.Round(r, MidpointRounding.AwayFromZero));
            r = cs.EvalCode("i1 pow 2, 10\nreturn i1");
            Assert.AreEqual(1024.0, r);
        }

        [TestMethod]
        public async Task TestCs64DefaultFileOut()
        {
            var settings = new AudioGraphSettings(AudioRenderCategory.Media);
            var aresult = await AudioGraph.CreateAsync(settings);
            Assert.IsTrue(AudioGraphCreationStatus.Success == aresult.Status);
            var audio = aresult.Graph;
            var bsize = audio.SamplesPerQuantum; //how many frames of samples expected during QuantumStarted event


            var path = KnownFolders.MusicLibrary;
            Assert.IsNotNull(path);
            var file = await path.CreateFileAsync("TestWavWrite.wav", CreationCollisionOption.ReplaceExisting);
            Assert.IsNotNull(file);
            var wavProps = MediaEncodingProfile.CreateWav(AudioEncodingQuality.High);
            var fresult = await audio.CreateFileOutputNodeAsync(file, wavProps);
            Assert.IsTrue(AudioFileNodeCreationStatus.Success == fresult.Status);
            var output = fresult.FileOutputNode;

            var cs = new CsoundRunner();
            Assert.IsNotNull(cs);
            var input = cs.AttchNode(audio);
            Assert.IsNotNull(input);
            input.AddOutgoingConnection(output);

            var nchnls = output.EncodingProperties.ChannelCount;
            var orc = string.Format("sr={0}\nksmps={1}\nnchnls={2}\n0dbfs=1\ninstr 1 \n asin oscil 0dbfs/4, p4 \n {3} \nendin\n",
                output.EncodingProperties.SampleRate, 10, nchnls, (nchnls == 2) ? "outs asin,asin" : "out asin") ;
            var score = "i1 0 0.5 480 .25 -1 \ne \n";
            Assert.IsTrue(CsoundStatus.OK == cs.CompileForPlay(orc, score));

            var done = cs.Start();
            output.Start();
            input.Start();
            audio.Start();
            await done; //wait for csound to finish
            output.Stop();
            input.Stop();
            cs.Stop();
            audio.Stop();
            var fr = await output.FinalizeAsync();
            Assert.IsTrue(fr == Windows.Media.Transcoding.TranscodeFailureReason.None);
            Assert.AreEqual(.5, cs.ScoreTime);
            cs.Dispose();
       }

        [TestMethod]
        public async Task TestCs64CustomFileOut()
        {
            var settings = new AudioGraphSettings(AudioRenderCategory.Media);
            settings.EncodingProperties = AudioEncodingProperties.CreatePcm(44100, 1, 32);
            settings.EncodingProperties.Subtype = "Float";
            var aresult = await AudioGraph.CreateAsync(settings);
            Assert.IsTrue((AudioGraphCreationStatus.Success == aresult.Status));
            var audio = aresult.Graph;
            var bsize = audio.SamplesPerQuantum; //how many frames of samples expected during QuantumStarted event


            var path = KnownFolders.MusicLibrary;
            Assert.IsNotNull(path);
            var file = await path.CreateFileAsync("TestCustomFileOut.mp3", CreationCollisionOption.ReplaceExisting);
            Assert.IsNotNull(file);


            var wavProps = MediaEncodingProfile.CreateMp3(AudioEncodingQuality.Medium); //.wav files cannot handle metadata
            //wavProps.Audio.Subtype = "PCM"; 
            wavProps.Audio.ChannelCount = 1;
            //wavProps.Audio.BitsPerSample = 16;
            var fresult = await audio.CreateFileOutputNodeAsync(file, wavProps);
            Assert.IsTrue(AudioFileNodeCreationStatus.Success == fresult.Status);
            var output = fresult.FileOutputNode;

            var cs = new CsoundRunner();
            Assert.IsNotNull(cs);
            var input = cs.AttchNode(audio);
            Assert.IsNotNull(input);
            input.AddOutgoingConnection(output);

            var nchnls = output.EncodingProperties.ChannelCount;
            var orc = string.Format("sr={0}\nksmps={1}\nnchnls={2}\n0dbfs=1\ninstr 1 \n asin oscil 0dbfs/4, p4 \n {3} \nendin\n",
                output.EncodingProperties.SampleRate, 9, nchnls, (nchnls == 2) ? "outs asin,asin" : "out asin");
            var score = "i1 0 1.0 480 .25 -1 \ne \n";
            Assert.IsTrue(CsoundStatus.OK == cs.CompileForPlay(orc, score));

            var done = cs.Start();
            input.Start();
            output.Start();
            audio.Start();
            await done; //wait for csound to finish
            output.Stop();
            input.Stop();
            cs.Stop();
            audio.Stop();
            var fr = await output.FinalizeAsync();
            Assert.IsTrue(fr == Windows.Media.Transcoding.TranscodeFailureReason.None);
            Assert.AreEqual(1.0, cs.ScoreTime);
            var mp = await file.Properties.GetMusicPropertiesAsync();
            mp.Title = "My Custom";
            mp.Artist = "Richard Henninger";
            await mp.SavePropertiesAsync();
            cs.Dispose();
        }

         [TestMethod]
        public async Task TestCs64DefaultAudioOut()
        {
            var settings = new AudioGraphSettings(Windows.Media.Render.AudioRenderCategory.Media);
            var aresult = await AudioGraph.CreateAsync(settings);
            Assert.IsTrue(AudioGraphCreationStatus.Success == aresult.Status);
            var audio = aresult.Graph;

            var oresult = await audio.CreateDeviceOutputNodeAsync();
            Assert.IsTrue(oresult.Status == AudioDeviceNodeCreationStatus.Success);
            var output = oresult.DeviceOutputNode;

            var cs = new CsoundRunner();
            Assert.IsNotNull(cs);
            var input = cs.AttchNode(audio);
            Assert.IsNotNull(input);
            input.AddOutgoingConnection(output);

            var nchnls = output.EncodingProperties.ChannelCount;
            var orc = string.Format("sr={0}\nksmps={1}\nnchnls={2}\n0dbfs=1\ninstr 1 \n asin oscil 0dbfs/4, p4 \n {3} \nendin\n",
                output.EncodingProperties.SampleRate, 10, nchnls, (nchnls == 2) ? "outs asin,asin" : "out asin");
            var score = "i1 0 .50 480 .25 -1 \ne \n";
            Assert.IsTrue(CsoundStatus.OK == cs.CompileForPlay(orc, score));

            var done = cs.Start();
            output.Start();
            input.Start();
            audio.Start();
            //await Task.Delay(1);
            await done; //wait for csound to finish
            output.Stop();
            input.Stop();
            cs.Stop();
            audio.Stop();
            Assert.AreEqual(.5, cs.ScoreTime);
            cs.Dispose();

        }



        /************************************************************************************************************/
        /*                       Utility Routines and Classes To Support Tests                                      */
        /************************************************************************************************************/
        public static string GetMessages(Csound64 csound)
        {
            var msgs = new StringBuilder();
            if (csound != null)
            {
                while (csound.HasMessages())
                {
                    msgs.Append(csound.NextMessage(out MessageAttrs atts));
                }
            }
            return msgs.ToString();
        }

        /************************************************************************************************************/
        /*              Helper class to facilitate generating music samples for a project.                          */
        /************************************************************************************************************/

        public class CsoundRunner : IDisposable
        {
            Csound64 m_csound = new Csound64();
            AudioFrameInputNode m_inputNode = null;
            TaskCompletionSource<bool> m_done;
            int m_frameCount = 0;

            public CsoundRunner()
            {
            }
            public AudioFrameInputNode AttchNode(AudioGraph audio)
            {
                m_frameCount = audio.SamplesPerQuantum;
                m_inputNode = audio.CreateFrameInputNode();
                m_inputNode.Stop();
                //maybe keep a reference to node for cleanup and reference to audioProps
                var audioProps = m_inputNode.EncodingProperties;
                var nchnls = audioProps.ChannelCount;
                var sr = audioProps.SampleRate; //need to get these to Orchestra
                m_inputNode.QuantumStarted += GetQuantumFromCsound64;
                return m_inputNode;
            }

            public CsoundStatus CompileForPlay(string orc, string sco)
            {
                var result = m_csound.CompileOrc(orc);
                if (result == CsoundStatus.OK)
                {
                    result = m_csound.ReadScore(sco);
                }
                return result;
            }

            public Task<bool> Start()
            {
                var status = (m_csound != null)  ? m_csound.Start(m_frameCount) : CsoundStatus.UnspecifiedError;
                m_done = new TaskCompletionSource<bool>();
                return m_done.Task;
            }

            public double ScoreTime { get { return m_csound.ScoreTimeInSeconds; } }

            public void Stop()
            {
                if (m_csound != null) m_csound.Stop();
            }
            public void GetQuantumFromCsound64(AudioFrameInputNode sender, FrameInputNodeQuantumStartedEventArgs args)
            {
                uint numSamplesNeeded = (uint)args.RequiredSamples;

                if ((numSamplesNeeded != 0) && (m_csound != null))
                {
                    uint capacity = numSamplesNeeded * (uint)m_csound.Nchnls * sizeof(float); //need to insure that nchnls and quantum channel assumptions match
                    AudioFrame audioData = new AudioFrame(capacity);
                    var done = m_csound.PerformAudioFrame(audioData);
                    if (!done)
                    {
                        sender.AddFrame(audioData);
                    }
                    else
                    {
                        m_done.SetResult(true);
                    }
                }
            }

            public string GetMessages()
            {
                return TestCsound64.GetMessages(m_csound);
             }

            public void Dispose()
            {
                m_done = null;
                if (m_inputNode != null)
                {
                    m_inputNode.QuantumStarted -= GetQuantumFromCsound64;
                    m_inputNode = null;
                }
               if (m_csound != null)
                {
                    m_csound.Cleanup();
                    GetMessages();
                    m_csound.Dispose();
                    m_csound = null;
                }
            }
        }
    }
}
