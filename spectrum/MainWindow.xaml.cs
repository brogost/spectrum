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
using System.Threading;
using System.IO;
using System.Reflection;

namespace spectrum
{

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        internal static class NativeMethods
        {
            [DllImport("HostedDx.dll")]
            public static extern int funky_test();
        }

        public MainWindow()
        {
            InitializeComponent();
            var tg = new TransformGroup();
            tg.Children.Add(new ScaleTransform());
            tg.Children.Add(new RotateTransform());
            tg.Children.Add(new TranslateTransform());
            canvas1.RenderTransform = tg;

            //canvas1.MouseWheel += new MouseWheelEventHandler(canvas1_MouseWheel);
            //canvas1.MouseLeftButtonDown += new MouseButtonEventHandler(canvas1_MouseLeftButtonDown);
            //canvas1.MouseLeftButtonUp += new MouseButtonEventHandler(canvas1_MouseLeftButtonUp);
            //canvas1.MouseMove += new MouseEventHandler(canvas1_MouseMove);

            var result = FMOD.Factory.System_Create(ref system);
            FmodCheck(result);

            uint version = 0;
            result = system.getVersion(ref version);
            FmodCheck(result);
            if (version < FMOD.VERSION.number) {
                MessageBox.Show("Error!  You are using an old version of FMOD " + version.ToString("X") + ".  This program requires " + FMOD.VERSION.number.ToString("X") + ".");
                //Application.Exit();
            }

            base.DataContext = this;

            result = system.init(32, FMOD.INITFLAGS.NORMAL, (IntPtr)null);
            FmodCheck(result);
        }

        void canvas1_MouseMove(object sender, MouseEventArgs e)
        {
            if (!canvas1.IsMouseCaptured) return;

            var TT = (TranslateTransform)((TransformGroup)canvas1.RenderTransform).Children.First(tr => tr is TranslateTransform);
            Vector vMM = start - e.GetPosition(canvas1);
            TT.X = origin.X - vMM.X;
            TT.Y = origin.Y - vMM.Y;
        }

        void canvas1_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            canvas1.ReleaseMouseCapture();
        }

        private Point origin;
        private Point start;

        void canvas1_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            canvas1.CaptureMouse();
            var TT = (TranslateTransform)((TransformGroup)canvas1.RenderTransform).Children.First(tr => tr is TranslateTransform);
            start = e.GetPosition(canvas1);
            origin = new Point(TT.X, TT.Y);
        }

        void canvas1_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            var tg = (TransformGroup)canvas1.RenderTransform;
            var s = (ScaleTransform)tg.Children.First(x => x is ScaleTransform);

            double zoom = e.Delta > 0 ? .2 : -.2;
            s.ScaleX += zoom;
            s.ScaleY += zoom;
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
            timer.Interval = TimeSpan.FromMilliseconds(1000 / 100);
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
            CurSongPos = (int)(1000.0 * canvas1.SongPos);
            //canvas1.InvalidateVisual();
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

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            int a = NativeMethods.funky_test();
            //var l = AdornerLayer.GetAdornerLayer(canvas1);
            //l.Add(new SimpleCircleAdorner(canvas1));
        }

        private void Canvas_Loaded(object sender, RoutedEventArgs e)
        {
            PresentationSource source = PresentationSource.FromVisual(this);

            if (source != null) {
                canvas1.DpiX = 96.0 * source.CompositionTarget.TransformToDevice.M11;
                canvas1.DpiY = 96.0 * source.CompositionTarget.TransformToDevice.M22;
            }
        }

        private void Export_Click(object sender, RoutedEventArgs e)
        {
            var f = CutOffValue;
            canvas1.create_cutoffs(f);
            var h = new HashSet<uint>();
            foreach (var i in canvas1.left_cut_off_indices) {
                h.Add(canvas1.IndextoMs((uint)i));
            }
            var s = new StreamWriter("time.hpp");
            uint prev = 0;
            s.WriteLine("#pragma once");
            s.WriteLine("int timestamps[] = {");
            foreach (var i in h) {
                if (i - prev > 100)
                    s.WriteLine("{0},", i);
                prev = i;
            }
            s.WriteLine("};");
            s.Close();
        }

        public int CurSongPos { get; set; }
        public float CutOffValue { get; set; }
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

        public float value_to_db(float value)
        {
            return (float)(20 * Math.Log10(value));
        }

        public void create_cutoffs(float value)
        {
            left_cut_off_indices.Clear();
            right_cut_off_indices.Clear();

            for (int i = 0; i < left_amp.Count(); ++i) {
                if (value_to_db(left_amp[i]) > value) {
                    left_cut_off_indices.Add(i);
                }
            }

            for (int i = 0; i < right_amp.Count(); ++i) {
                if (value_to_db(right_amp[i]) > value) {
                    right_cut_off_indices.Add(i);
                }
            }

            //InvalidateVisual();
        }

        public uint MsToIndex(uint ms)
        {
            long t = (long)ms * (long)SampleRate / 1000;
            return (uint)t;
        }

        public uint IndextoMs(uint index)
        {
            long t = 1000 * (long)index / SampleRate;
            return (uint)t;
        }

        public uint ActualPixelWidth()
        {
            return (uint)(ActualWidth * DpiX / 96);
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

        private void visual_inner(List<float> pts, HashSet<int> selected, Pen pen)
        {
            if (pts.Count == 0)
                return;

            uint screen_size_in_ms = DistToMs(ActualPixelWidth());
            uint start_ms = OffsetInMs;
            uint end_ms = PixelToMs(ActualPixelWidth());

            var idx = (int)MsToIndex(start_ms);
            var cur = new Point(0, amp_to_pixel(pts[idx]));

            var g = new StreamGeometry();
            var c = g.Open();
            c.BeginFigure(cur, true, false);

            var selected_pen = new Pen(Brushes.LightBlue, 1);

            var v = new DrawingVisual();
            using (DrawingContext dc = v.RenderOpen()) {
                for (int i = 1; i < ActualWidth; ++i) {
                    float t = i / (float)ActualWidth;
                    idx = (int)MsToIndex((uint)((1 - t) * start_ms + t * end_ms));
                    if (idx >= pts.Count)
                        break;

                    cur = new Point(i, amp_to_pixel(pts[idx]));
                    c.LineTo(cur, true, false);

                    if (selected.Contains(idx))
                        dc.DrawEllipse(null, selected_pen, cur, 2, 2);
                }
                c.Close();
                dc.DrawGeometry(null, pen, g);
                dc.Close();
                AddVisual(v);
            }

        }

        public void CreateVisuals()
        {

            if (left_amp.Count == 0) {
                return;
            }

            RemoveAllVisuals();
            var left_pen = new Pen(Brushes.YellowGreen, 1);
            var right_pen = new Pen(Brushes.OrangeRed, 1);

            if (DisplayLeft)
                visual_inner(left_amp, left_cut_off_indices, left_pen);
            if (DisplayRight)
                visual_inner(right_amp, right_cut_off_indices, right_pen);

        }

        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Brushes.DarkGray, null, new Rect(0,0, ActualWidth, ActualHeight));

            var culture = CultureInfo.GetCultureInfo("en-us");
            var typeface = new Typeface("Verdana");

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

            // draw dB
            int db_lines = 10;
            for (int i = 0; i < db_lines; ++i) {
                var cur = (float)i / (db_lines-1);
                var db = 20 * Math.Log10(1-cur);
                var y = cur * ActualHeight;
                dc.DrawLine(gray_pen, new Point(0, y), new Point(ActualWidth, y));
                dc.DrawText(new FormattedText(String.Format("{0:F2} db", db), culture, FlowDirection.LeftToRight, typeface, 12, Brushes.Black), 
                    new Point(10, y));
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

            // draw cur pos
            var cur_pos = MsToPixel(pos);
            var red_pen = new Pen(Brushes.Red, 1);
            dc.DrawLine(red_pen, new Point(cur_pos, 0), new Point(cur_pos, Height));
        }

        public bool DisplayLeft { get; set; }
        public bool DisplayRight { get; set; }

        public double DpiX { get; set; }
        public double DpiY { get; set; }

        public float SongPos { get; set; }

        public HashSet<int> left_cut_off_indices = new HashSet<int>();
        public HashSet<int> right_cut_off_indices = new HashSet<int>();
        public List<float> left_amp = new List<float>();    // in the range [-1..1]
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
