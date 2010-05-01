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
using System.Globalization;

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
                float l = (float)BitConverter.ToInt16(tmp, cur) / 32768;
                float r = (float)BitConverter.ToInt16(tmp, cur + 2) / 32768;
                canvas1.left_amp.Add(l);
                canvas1.right_amp.Add(r);
                cur += sampleSize;
            }
            sound.unlock(ptr1, ptr2, len1, len2);
            canvas1.sound = sound;
            canvas1.SampleRate = 44100;
            canvas1.ScaleFactor = 128;


            canvas1.CreateVisuals();
            canvas1.InvalidateVisual();

            timer.Tick += new EventHandler(timer_Tick);
            timer.Interval = TimeSpan.FromMilliseconds(1000 / 25);
            timer.Start();
        }

        void timer_Tick(object sender, EventArgs e)
        {
            if (channel == null) {
                return;
            }
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
                    canvas1.ScaleFactor = canvas1.ScaleFactor > 1 ? canvas1.ScaleFactor / 2 : 1;
                    break;
                case Key.Subtract:
                    canvas1.ScaleFactor = canvas1.ScaleFactor * 2;
                    break;
                case Key.PageDown:
                    canvas1.ForwardPage();
                    break;
                case Key.PageUp:
                    canvas1.BackPage();
                    break;
                case Key.Space:
                    bool paused = false;
                    if (channel == null) {
                        start_playing();
                    } else {
                        channel.getPaused(ref paused);
                        channel.setPaused(!paused);
                    }
                    break;
                case Key.Right:
                    canvas1.Right();
                    break;
            }
            canvas1.CreateVisuals();
        }


        private void Window_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            canvas1.Clicked();
        }

        private void Window_SizeChanged(object sender, System.Windows.SizeChangedEventArgs e)
        {
            canvas1.CreateVisuals();
        	// TODO: Add event handler implementation here.
        }

        private void Play_Click(object sender, RoutedEventArgs e)
        {
            start_playing();
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            if (channel == null) {
                return;
            }

            channel.stop();
        }

        private void start_playing()
        {
            var result = system.playSound(FMOD.CHANNELINDEX.FREE, sound, false, ref channel);
            canvas1.channel = channel;
            channel.setVolume(0);
        }

        private void LeftCheck_Checked(object sender, RoutedEventArgs e)
        {
            canvas1.DisplayLeft = (bool)LeftCheck.IsChecked;
            canvas1.CreateVisuals();
        }

        private void RightCheck_Checked(object sender, RoutedEventArgs e)
        {
            canvas1.DisplayRight = (bool)RightCheck.IsChecked;
            canvas1.CreateVisuals();
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

        public void Right()
        {

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
            long num = (long)(ms - OffsetInMs) << 8;
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

        // generate tick marks at nice intervals, using "Nice Numbers For Graph Labels" in Graphic Gems
        private void generate_nice_ticks(float min_value, float max_value, int num_ticks, out List<float> ticks)
        {
            var range = nice_num(max_value - min_value, false);
            var d = nice_num(range / (num_ticks - 1), true);
            var graph_min = Math.Floor(min_value / d) * d;
            var graph_max = Math.Ceiling(max_value / d) * d;
            var nfrac = Math.Max(-Math.Floor(Math.Log10(d)), 0);
            ticks = new List<float>();
            ticks.Clear();
            var x = graph_min;
            while (x <= graph_max + 0.5 * d) {
                ticks.Add((float)x);
                x += d;
            }
        }

        private float nice_num(float x, bool round)
        {
            var e = Math.Floor(Math.Log10(x));
            var f = x / Math.Pow(10, e);
            float nf;
            if (round) {
                if (f < 1.5) nf = 1;
                else if (f < 3) nf = 2;
                else if (f < 7) nf = 5;
                else nf = 10;
            } else {
                if (f <= 1) nf = 1;
                else if (f <= 2) nf = 2;
                else if (f <= 5) nf = 5;
                else nf = 10;
            }

            return (float)(nf * Math.Pow(10, e));
        }

        private float amp_to_pixel(float v)
        {
            return (float)(Height / 2 - Height/2 * v);
        }

        public void CreateVisuals()
        {
            if (left_amp.Count == 0) {
                return;
            }

            RemoveAllVisuals();
            var left_pen = new Pen(Brushes.YellowGreen, 1);
            var right_pen = new Pen(Brushes.OrangeRed, 1);

            uint screen_size_in_ms = DistToMs(ActualPixelWidth());
            uint start_ms = OffsetInMs;
            uint end_ms = PixelToMs(ActualPixelWidth());

            var idx = (int)MsToIndex(start_ms);
            var left_prev = new Point(0, amp_to_pixel(left_amp[idx]));
            var right_prev = new Point(0, amp_to_pixel(right_amp[idx]));
            for (int i = 1; i < ActualWidth; ++i) {
                float t = i / (float)ActualWidth;
                idx = (int)MsToIndex((uint)((1 - t) * start_ms + t * end_ms));
                if (idx >= left_amp.Count) {
                    break;
                }
               
                var left_cur = new Point(i, amp_to_pixel(left_amp[idx]));
                var right_cur = new Point(i, amp_to_pixel(right_amp[idx]));
                var v = new DrawingVisual();
                using (DrawingContext dc = v.RenderOpen()) {
                    uint cur_ms = PixelToMs((uint)i);
                    if (DisplayLeft) {
                        dc.DrawLine(left_pen, left_prev, left_cur);
                    }
                    if (DisplayRight) {
                        dc.DrawLine(right_pen, right_prev, right_cur);
                    }
                    dc.Close();
                }
                AddVisual(v);
                left_prev = left_cur;
                right_prev = right_cur;
            }
        }

        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Brushes.DarkGray, null, new Rect(0,0, ActualWidth, ActualHeight));

            var culture = CultureInfo.GetCultureInfo("en-us");
            var typeface = new Typeface("Verdana");

            // create x screens of data to be able to scroll without reloading
            List<float> ticks;
            generate_nice_ticks(PixelToMs(0), PixelToMs(ActualPixelWidth()), 10, out ticks);

            var gray_pen = new Pen(Brushes.AliceBlue, 1);
            foreach (var t in ticks) {
                var cur_x = MsToPixel((uint)t);
                var top = new Point(cur_x, ActualHeight);
                var bottom = new Point(cur_x, 0);
                dc.DrawLine(gray_pen, top, bottom);
                dc.DrawText(new FormattedText(String.Format("{0:F2}s", t/1000), culture, FlowDirection.LeftToRight, typeface, 12, Brushes.Black), new Point(cur_x + 10, 20));
            }

            PresentationSource source = PresentationSource.FromVisual(this);

            if (source != null) {
                dpiX = 96.0 * source.CompositionTarget.TransformToDevice.M11;
                dpiY = 96.0 * source.CompositionTarget.TransformToDevice.M22;
            }

            if (channel == null) {
                return;
            }

            uint pos = 0;
            channel.getPosition(ref pos, FMOD.TIMEUNIT.MS);
            if (pos > PixelToMs(ActualPixelWidth())) {
                OffsetInMs = pos;
                CreateVisuals();
            }

            base.OnRender(dc);
            var cur_pos = MsToPixel(pos);
            var red_pen = new Pen(Brushes.Red, 1);
            dc.DrawLine(red_pen, new Point(cur_pos, 0), new Point(cur_pos, Height));
        }

        public bool DisplayLeft { get; set; }
        public bool DisplayRight { get; set; }

        private double dpiX = 96;
        private double dpiY = 96;

        public float SongPos { get; set; }
        public List<float> left_amp = new List<float>();        // in the range [-1..1]
        public List<float> right_amp = new List<float>();
        public FMOD.Sound sound = null;
        public FMOD.Channel channel = null;

        public int SampleRate {get; set; }


        // in 24.8 fixed point
        public uint ScaleFactor;
        public uint OffsetInMs { get; set; }
        public uint ScrollOffset { get; set; }

        private uint ms_per_pixel = 1;

        private List<Visual> visuals = new List<Visual>();
        protected override int VisualChildrenCount { get { return visuals.Count; } }
        protected override Visual GetVisualChild(int index) { return visuals[index]; }


    }

}
