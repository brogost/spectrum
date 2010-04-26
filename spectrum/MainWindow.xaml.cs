using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;

namespace spectrum
{

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            var result = FMOD.Factory.System_Create(ref system);
            FmodCheck(result);

            uint version = 0;
            result = system.getVersion(ref version);
            FmodCheck(result);
            if (version < FMOD.VERSION.number) {
                MessageBox.Show("Error!  You are using an old version of FMOD " + version.ToString("X") + ".  This program requires " + FMOD.VERSION.number.ToString("X") + ".");
                //Application.Exit();
            }

            result = system.init(32, FMOD.INITFLAGS.NORMAL, (IntPtr)null);
            FmodCheck(result);
        }

        private void FmodCheck(FMOD.RESULT result)
        {
            if (result != FMOD.RESULT.OK) {
                //timer.Stop();
                MessageBox.Show("FMOD error! " + result + " - " + FMOD.Error.String(result));
                Environment.Exit(-1);
            }
        }

        private void FileOpen_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = "mp3";
            if (dlg.ShowDialog() == null) {
                return;
            }

            var filename = dlg.FileName;
            var result = system.createSound(filename, FMOD.MODE.SOFTWARE | FMOD.MODE._2D, ref sound);
            //var result = system.createStream(filename, FMOD.MODE.SOFTWARE | FMOD.MODE._2D, ref sound);
            FmodCheck(result);

            uint totalBytes = 0;
            sound.getLength(ref totalBytes, FMOD.TIMEUNIT.PCMBYTES);
            uint lengthInMs = 0;
            sound.getLength(ref lengthInMs, FMOD.TIMEUNIT.MS);
            FMOD.SOUND_TYPE type = FMOD.SOUND_TYPE.UNKNOWN;
            FMOD.SOUND_FORMAT format = FMOD.SOUND_FORMAT.NONE;
            int channels = 0;
            int bits = 0;
            sound.getFormat(ref type, ref format, ref channels, ref bits);

            IntPtr ptr1 = new IntPtr();
            IntPtr ptr2 = new IntPtr();
            uint len1 = 0;
            uint len2 = 0;
            var res = sound.@lock(0, totalBytes, ref ptr1, ref ptr2, ref len1, ref len2);
            FmodCheck(res);
            var numSamples = totalBytes / (bits / 8 * channels);
            var sampleSize = bits / 8 * channels;
            var cur = 0;
            byte [] tmp = new byte[len1];
            Marshal.Copy(ptr1, tmp, 0, (int)len1);
            for (var i = 0; i < numSamples; ++i) {
                float l = (float)BitConverter.ToInt16(tmp, cur) / 65536;
                float r = (float)BitConverter.ToInt16(tmp, cur + 2) / 65536;
                canvas1.left_amp.Add(l);
                canvas1.right_amp.Add(r);
                cur += sampleSize;
            }
            sound.unlock(ptr1, ptr2, len1, len2);
            result = system.playSound(FMOD.CHANNELINDEX.FREE, sound, false, ref channel);
            canvas1.channel = channel;
            canvas1.sound = sound;
            canvas1.SampleRate = 44100;
            canvas1.ScaleFactor = 128;


            canvas1.InitVisuals();
            canvas1.InvalidateVisual();

            timer.Tick += new EventHandler(timer_Tick);
            timer.Interval = TimeSpan.FromMilliseconds(100);
            timer.Start();
        }

        void timer_Tick(object sender, EventArgs e)
        {
            uint pos = 0;
            uint len = 0;
            channel.getPosition(ref pos, FMOD.TIMEUNIT.MS);
            sound.getLength(ref len, FMOD.TIMEUNIT.MS);
            canvas1.SongPos = pos / (float)len;
            canvas1.InvalidateVisual();
            UpdateText();
        }

        private System.Windows.Threading.DispatcherTimer timer = new System.Windows.Threading.DispatcherTimer();

        private FMOD.System system = null;
        private FMOD.Sound sound = null;
        private FMOD.Channel channel = null;

        private void slider1_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var v = e.NewValue;
        }

        private void UpdateText()
        {
            Scale.Text = canvas1.ScaleFactor.ToString();
            Offset.Text = canvas1.OffsetInMs.ToString();

        }

        private void Window_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            switch (e.Key) {
                case Key.Add:
                    canvas1.ScaleFactor = canvas1.ScaleFactor * 2;
                    break;
                case Key.Subtract:
                    canvas1.ScaleFactor = canvas1.ScaleFactor > 1 ? canvas1.ScaleFactor / 2 : 1;
                    break;
                case Key.PageDown:
                    canvas1.ForwardPage();
                    break;
                case Key.PageUp:
                    canvas1.BackPage();
                    break;
                case Key.Space:
                    bool paused = false;
                    channel.getPaused(ref paused);
                    channel.setPaused(!paused);
                    break;
            }
            canvas1.InitVisuals();
        }


        private void Window_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            canvas1.Clicked();
        }
    }

    public class MyCanvas : Canvas
    {
        public void Clicked()
        {
            if (channel == null) {
                return;
            }
            var p = Mouse.GetPosition(this);
            uint ms = PixelToMs((uint)p.X);
            channel.setPosition(ms, FMOD.TIMEUNIT.MS);
        }

        private uint PixelToMs(uint pixel)
        {
            long t = (long)(pixel * ms_per_pixel) * ScaleFactor;
            return (uint)(OffsetInMs + (t >> 8));
        }

        private uint DistToMs(uint pixels)
        {
            long t = (long)(pixels * ms_per_pixel) * ScaleFactor;
            return (uint)(t >> 8);
        }

        public void BackPage()
        {
            if (channel == null) {
                return;
            }
            uint ofs = DistToMs(ActualPixelWidth());
            uint pos = 0;
            channel.getPosition(ref pos, FMOD.TIMEUNIT.MS);
            uint new_pos = ofs > pos ? 0 : pos - ofs;
            channel.setPosition(new_pos, FMOD.TIMEUNIT.MS);
            OffsetInMs = new_pos;
        }

        public void ForwardPage()
        {
            if (channel == null) {
                return;
            }
            uint ofs = DistToMs(ActualPixelWidth());
            uint pos = 0;
            uint len = 0;
            sound.getLength(ref len, FMOD.TIMEUNIT.MS);
            channel.getPosition(ref pos, FMOD.TIMEUNIT.MS);
            uint new_pos = pos + ofs > len ? len : pos + ofs;
            channel.setPosition(new_pos, FMOD.TIMEUNIT.MS);
            OffsetInMs = new_pos;
        }

        private uint MsToPixel(uint ms)
        {
            long num = (ms - OffsetInMs) << 8;
            long denom = (ms_per_pixel * ScaleFactor);
            return (uint)(num / denom);
        }

        public void AddVisual(Visual visual)
        {
            visuals.Add(visual);
            base.AddVisualChild(visual);
            base.AddLogicalChild(visual);
        }

        public void DeleteVisual(Visual visual)
        {
            visuals.Remove(visual);
            base.RemoveVisualChild(visual);
            base.RemoveLogicalChild(visual);
        }

        public void RemoveAllVisuals()
        {
            foreach (var v in visuals) {
                base.RemoveVisualChild(v);
                base.RemoveLogicalChild(v);
            }
            visuals.Clear();
        }

        uint MsToIndex(uint ms)
        {
            long t = (long)ms * (long)SampleRate / 1000;
            return (uint)t;
        }

        uint ActualPixelWidth()
        {
            return (uint)(ActualWidth * dpiX / 96);
        }

        public void InitVisuals()
        {
            RemoveAllVisuals();

            uint start_ms = OffsetInMs;
            uint end_ms = PixelToMs(ActualPixelWidth());
            var prev = new Point(0, Height - Height * left_amp[(int)MsToIndex(start_ms)]);
            var black_pen = new Pen(Brushes.Black, 1);
            for (int i = 1; i < ActualWidth; ++i) {
                float t = i / (float)ActualWidth;
                int idx = (int)MsToIndex((uint)((1 - t) * start_ms + t * end_ms));
                if (idx >= left_amp.Count) {
                    break;
                }
                var cur = new Point(i, Height - Height * left_amp[idx]);
                var v = new DrawingVisual();
                using (DrawingContext dc = v.RenderOpen()) {
                    dc.DrawLine(black_pen, prev, cur);
                    dc.Close();
                }
                AddVisual(v);
                prev = cur;
            }
        }

        protected override void OnRender(DrawingContext dc)
        {
            if (channel == null) {
                return;
            }
            PresentationSource source = PresentationSource.FromVisual(this);

            if (source != null) {
                dpiX = 96.0 * source.CompositionTarget.TransformToDevice.M11;
                dpiY = 96.0 * source.CompositionTarget.TransformToDevice.M22;
            }

            uint pos = 0;
            channel.getPosition(ref pos, FMOD.TIMEUNIT.MS);
            if (pos > PixelToMs(ActualPixelWidth())) {
                OffsetInMs = pos;
                InitVisuals();
            }

            base.OnRender(dc);
            var cur_pos = MsToPixel(pos);
            var red_pen = new Pen(Brushes.Red, 1);
            dc.DrawLine(red_pen, new Point(cur_pos, 0), new Point(cur_pos, Height));
        }

        private double dpiX = 96;
        private double dpiY = 96;

        public float SongPos { get; set; }
        public List<float> left_amp = new List<float>();
        public List<float> right_amp = new List<float>();
        public FMOD.Sound sound = null;
        public FMOD.Channel channel = null;

        public int SampleRate {get; set; }


        // in 24.8 fixed point
        public uint ScaleFactor;
        private uint scale_factor = 1;

        public uint OffsetInMs { get; set; }

        private uint ms_per_pixel = 1;

        private List<Visual> visuals = new List<Visual>();
        protected override int VisualChildrenCount { get { return visuals.Count; } }
        protected override Visual GetVisualChild(int index) { return visuals[index]; }

    }

}
