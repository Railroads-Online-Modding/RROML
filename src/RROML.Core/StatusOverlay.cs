using System;
using System.Diagnostics;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;

namespace RROML.Core
{
    internal static class StatusOverlay
    {
        private sealed class OverlayState
        {
            public string Message;
            public int StatusKind;
            public string Position;
            public int AutoHideMs;
            public DateTime UpdatedAtUtc;
        }

        private static readonly object Gate = new object();
        private static OverlayHost _host;
        private static FileLogger _logger;

        public static void Initialize(string loaderPath, FileLogger logger)
        {
            lock (Gate)
            {
                _logger = logger;
                if (_host != null)
                {
                    return;
                }

                _host = new OverlayHost(logger);
                _host.Start();
            }
        }

        public static void ShowLoading(LoaderConfig config)
        {
            if (config == null || !config.ShowOverlay)
            {
                return;
            }

            Publish("RROML Loading...", 0, config, 0);
        }

        public static void ShowSuccess(LoaderConfig config, int loadedModCount)
        {
            if (config == null || !config.ShowOverlay)
            {
                return;
            }

            Publish("RROML Loaded Successfully, " + loadedModCount + " mods loaded.", 1, config, 4000);
        }

        public static void ShowFailure(LoaderConfig config)
        {
            if (config == null || !config.ShowOverlay)
            {
                return;
            }

            Publish("RROML Failed to Load", 2, config, 6000);
        }

        public static void ShowWarning(LoaderConfig config, int loadedModCount)
        {
            if (config == null || !config.ShowOverlay)
            {
                return;
            }

            Publish("RROML Loaded with Warnings, " + loadedModCount + " mods loaded.", 3, config, 5000);
        }

        private static void Publish(string message, int statusKind, LoaderConfig config, int autoHideMs)
        {
            OverlayHost host;
            lock (Gate)
            {
                host = _host;
            }

            if (host == null)
            {
                throw new InvalidOperationException("Status overlay was not initialized.");
            }

            host.Publish(new OverlayState
            {
                Message = message,
                StatusKind = statusKind,
                Position = string.IsNullOrWhiteSpace(config.OverlayPosition) ? "TopLeft" : config.OverlayPosition,
                AutoHideMs = autoHideMs,
                UpdatedAtUtc = DateTime.UtcNow
            });

            if (_logger != null)
            {
                _logger.Info("Overlay state updated: " + message);
            }
        }

        private sealed class OverlayHost
        {
            private readonly FileLogger _logger;
            private readonly ManualResetEvent _ready;
            private Thread _thread;
            private OverlayForm _form;

            public OverlayHost(FileLogger logger)
            {
                _logger = logger;
                _ready = new ManualResetEvent(false);
            }

            public void Start()
            {
                _thread = new Thread(ThreadMain);
                _thread.IsBackground = true;
                _thread.Name = "RROML Overlay";
                _thread.SetApartmentState(ApartmentState.STA);
                _thread.Start();
                _ready.WaitOne();
            }

            public void Publish(OverlayState state)
            {
                if (!_ready.WaitOne(5000) || _form == null || !_form.IsHandleCreated)
                {
                    throw new InvalidOperationException("Overlay form did not initialize.");
                }

                _form.BeginInvoke((MethodInvoker)delegate
                {
                    _form.ApplyState(state);
                });
            }

            private void ThreadMain()
            {
                try
                {
                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    _form = new OverlayForm(_logger);
                    _ready.Set();
                    Application.Run(_form);
                }
                catch (Exception exception)
                {
                    _logger.Error("Overlay host thread failed.", exception);
                    _ready.Set();
                }
            }
        }

        private sealed class OverlayForm : Form
        {
            private readonly FileLogger _logger;
            private readonly System.Windows.Forms.Timer _timer;
            private readonly Font _font;
            private OverlayState _state;
            private IntPtr _gameWindow;
            private DateTime? _hideAtUtc;
            private IntPtr _lastLoggedWindow;

            public OverlayForm(FileLogger logger)
            {
                _logger = logger;
                _timer = new System.Windows.Forms.Timer();
                _timer.Interval = 100;
                _timer.Tick += OnTick;
                _font = new Font("Segoe UI", 12f, FontStyle.Bold, GraphicsUnit.Point);

                FormBorderStyle = FormBorderStyle.None;
                ShowInTaskbar = false;
                StartPosition = FormStartPosition.Manual;
                TopMost = true;
                BackColor = Color.Black;
                ForeColor = Color.White;
                Opacity = 0.9d;
                Width = 460;
                Height = 52;
                DoubleBuffered = true;
            }

            protected override bool ShowWithoutActivation
            {
                get { return true; }
            }

            protected override CreateParams CreateParams
            {
                get
                {
                    const int WsExToolWindow = 0x00000080;
                    const int WsExTopMost = 0x00000008;
                    const int WsExNoActivate = 0x08000000;
                    const int WsExTransparent = 0x00000020;
                    const int WsExLayered = 0x00080000;
                    var createParams = base.CreateParams;
                    createParams.ExStyle |= WsExToolWindow | WsExTopMost | WsExNoActivate | WsExTransparent | WsExLayered;
                    return createParams;
                }
            }

            protected override void OnLoad(EventArgs e)
            {
                base.OnLoad(e);
                Hide();
                _timer.Start();
            }

            protected override void OnShown(EventArgs e)
            {
                base.OnShown(e);
                Hide();
            }

            protected override void Dispose(bool disposing)
            {
                if (disposing)
                {
                    _timer.Dispose();
                    _font.Dispose();
                }

                base.Dispose(disposing);
            }

            public void ApplyState(OverlayState state)
            {
                _state = state;
                _hideAtUtc = state.AutoHideMs > 0 ? (DateTime?)DateTime.UtcNow.AddMilliseconds(state.AutoHideMs) : null;
                Invalidate();
                UpdateVisibility();
            }

            protected override void OnPaint(PaintEventArgs e)
            {
                base.OnPaint(e);
                e.Graphics.Clear(Color.FromArgb(215, 18, 18, 18));

                if (_state == null || string.IsNullOrWhiteSpace(_state.Message))
                {
                    return;
                }

                using (var borderPen = new Pen(GetAccentColor(), 2f))
                using (var textBrush = new SolidBrush(GetAccentColor()))
                using (var format = new StringFormat())
                {
                    var rect = new Rectangle(1, 1, Width - 3, Height - 3);
                    e.Graphics.DrawRectangle(borderPen, rect);
                    var textRect = new RectangleF(16f, 12f, Width - 32f, Height - 24f);
                    format.Alignment = StringAlignment.Near;
                    format.LineAlignment = StringAlignment.Center;
                    e.Graphics.DrawString(_state.Message, _font, textBrush, textRect, format);
                }
            }

            private void OnTick(object sender, EventArgs e)
            {
                UpdateVisibility();
            }

            private void UpdateVisibility()
            {
                if (_state == null || string.IsNullOrWhiteSpace(_state.Message))
                {
                    Hide();
                    return;
                }

                if (_hideAtUtc.HasValue && DateTime.UtcNow >= _hideAtUtc.Value)
                {
                    Hide();
                    return;
                }

                var gameWindow = FindGameWindow();
                if (gameWindow == IntPtr.Zero)
                {
                    Hide();
                    return;
                }

                if (NativeMethods.GetForegroundWindow() != gameWindow)
                {
                    Hide();
                    return;
                }

                NativeMethods.RECT rect;
                if (!NativeMethods.GetWindowRect(gameWindow, out rect))
                {
                    Hide();
                    return;
                }

                _gameWindow = gameWindow;
                var margin = 18;
                var topOffset = 72;
                var x = rect.Left + margin;
                if (string.Equals(_state.Position, "TopRight", StringComparison.OrdinalIgnoreCase))
                {
                    x = rect.Right - Width - margin;
                }

                var y = rect.Top + topOffset;
                if (Location.X != x || Location.Y != y)
                {
                    Location = new Point(x, y);
                }

                if (!Visible)
                {
                    Show();
                }

                if (WindowState == FormWindowState.Minimized)
                {
                    WindowState = FormWindowState.Normal;
                }

                NativeMethods.SetWindowPos(Handle, NativeMethods.HwndTopMost, x, y, Width, Height, NativeMethods.SwpNoActivate | NativeMethods.SwpShowWindow);
                Invalidate();
            }

            private IntPtr FindGameWindow()
            {
                if (_gameWindow != IntPtr.Zero && NativeMethods.IsWindow(_gameWindow))
                {
                    return _gameWindow;
                }

                var processId = Process.GetCurrentProcess().Id;
                IntPtr found = IntPtr.Zero;
                NativeMethods.EnumWindows(delegate(IntPtr handle, IntPtr lParam)
                {
                    int windowProcessId;
                    NativeMethods.GetWindowThreadProcessId(handle, out windowProcessId);
                    if (windowProcessId != processId || !NativeMethods.IsWindowVisible(handle))
                    {
                        return true;
                    }

                    if (NativeMethods.GetWindow(handle, 4) != IntPtr.Zero)
                    {
                        return true;
                    }

                    found = handle;
                    return false;
                }, IntPtr.Zero);

                if (found != IntPtr.Zero && found != _lastLoggedWindow)
                {
                    _lastLoggedWindow = found;
                    _logger.Info("Overlay attached to game window 0x" + found.ToString("X"));
                }

                return found;
            }

            private Color GetAccentColor()
            {
                if (_state == null)
                {
                    return Color.FromArgb(214, 110, 34);
                }

                if (_state.StatusKind == 2)
                {
                    return Color.FromArgb(201, 84, 84);
                }

                if (_state.StatusKind == 3)
                {
                    return Color.FromArgb(232, 197, 72);
                }

                return Color.FromArgb(214, 110, 34);
            }
        }

        private static class NativeMethods
        {
            public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

            public static readonly IntPtr HwndTopMost = new IntPtr(-1);
            public const uint SwpNoActivate = 0x0010;
            public const uint SwpShowWindow = 0x0040;

            [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
            public struct RECT
            {
                public int Left;
                public int Top;
                public int Right;
                public int Bottom;
            }

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern bool IsWindowVisible(IntPtr hWnd);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern bool IsWindow(IntPtr hWnd);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern IntPtr GetForegroundWindow();

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern int GetWindowThreadProcessId(IntPtr hWnd, out int processId);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern IntPtr GetWindow(IntPtr hWnd, uint command);

            [System.Runtime.InteropServices.DllImport("user32.dll")]
            public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, uint flags);
        }
    }
}






