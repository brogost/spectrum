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
using System.Windows.Interop;

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
            base.DataContext = this;
        }

        private void FileOpen_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = "mp3";
            if (dlg.ShowDialog() == null)
                return;

            var filename = dlg.FileName;

            if (!File.Exists(filename))
                return;

            if (!NativeMethods.load_mp3(filename))
                return;

            timer.Tick += new EventHandler(timer_Tick);
            timer.Interval = TimeSpan.FromMilliseconds(1000 / 100);
            timer.Start();
        }

        void timer_Tick(object sender, EventArgs e)
        {
        }

        private System.Windows.Threading.DispatcherTimer timer = new System.Windows.Threading.DispatcherTimer();

        private void slider1_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var v = e.NewValue;
        }

        private void Window_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            switch (e.Key) {
                case Key.Add:
                    break;
                case Key.Subtract:
                    break;
                case Key.PageDown:
                    break;
                case Key.PageUp:
                    break;
                case Key.Space:
                    NativeMethods.set_paused(!NativeMethods.get_paused());
                    break;
                case Key.Left:
                    break;
                case Key.Right:
                    break;
            }
        }


        private void Window_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
        }

        private void Window_SizeChanged(object sender, System.Windows.SizeChangedEventArgs e)
        {
        }

        private void Play_Click(object sender, RoutedEventArgs e)
        {
            NativeMethods.start_mp3();
        }

        private void Stop_Click(object sender, RoutedEventArgs e)
        {
            NativeMethods.stop_mp3();
        }

        private void LeftCheck_Checked(object sender, RoutedEventArgs e)
        {
        }

        private void RightCheck_Checked(object sender, RoutedEventArgs e)
        {
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            PresentationSource source = PresentationSource.FromVisual(this);

            if (source != null) {
                DpiX = 96.0 * source.CompositionTarget.TransformToDevice.M11;
                DpiY = 96.0 * source.CompositionTarget.TransformToDevice.M22;
            }

            var h = new DxHost();
            h.CanvasWidth = (int)(DpiX * HostPlaceholder.ActualWidth / 96);
            h.CanvasHeight = (int)(DpiY * HostPlaceholder.ActualHeight / 96);
            HostPlaceholder.Child = h;
        }

        private void Export_Click(object sender, RoutedEventArgs e)
        {
/*
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
 */
        }

        public double DpiX { get; set; }
        public double DpiY { get; set; }

        public int CurSongPos { get; set; }
        public float CutOffValue { get; set; }
    }

    internal static class NativeMethods
    {
        [DllImport("HostedDx.dll")]
        public static extern IntPtr create_d3d(int width, int height, IntPtr parent);

        [DllImport("HostedDx.dll")]
        public static extern void destroy_d3d();

        [DllImport("HostedDx.dll")]
        public static extern bool load_mp3([MarshalAs(UnmanagedType.LPWStr)]String s);

        [DllImport("HostedDx.dll")]
        public static extern bool start_mp3();

        [DllImport("HostedDx.dll")]
        public static extern bool stop_mp3();

        [DllImport("HostedDx.dll")]
        public static extern bool get_paused();

        [DllImport("HostedDx.dll")]
        public static extern bool set_paused(bool state);

    }

    public class DxHost : HwndHost
    {
        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            IntPtr h = NativeMethods.create_d3d(CanvasWidth, CanvasHeight, hwndParent.Handle);
            return new HandleRef(this, h);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            NativeMethods.destroy_d3d();
        }

        public int CanvasWidth { get; set; }
        public int CanvasHeight { get; set; }
    }

}
