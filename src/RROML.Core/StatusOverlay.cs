using System;
using System.Runtime.InteropServices;

namespace RROML.Core
{
    internal static class StatusOverlay
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private delegate void SetStatusDelegate(string text, int statusKind, int autoHideMs);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate void SetPositionDelegate(int position);

        private static FileLogger _logger;
        private static SetStatusDelegate _setStatus;
        private static SetPositionDelegate _setPosition;

        public static void Initialize(string loaderPath, FileLogger logger)
        {
            _logger = logger;
            var proxy = GetModuleHandle("XINPUT1_3.dll");
            if (proxy == IntPtr.Zero)
            {
                logger.Warn("In-game status overlay is unavailable because the XInput proxy is not loaded.");
                return;
            }

            var setStatus = GetProcAddress(proxy, "RROML_SetOverlayStatusW");
            var setPosition = GetProcAddress(proxy, "RROML_SetOverlayPosition");
            if (setStatus == IntPtr.Zero || setPosition == IntPtr.Zero)
            {
                logger.Warn("In-game status overlay exports were not found.");
                return;
            }

            _setStatus = (SetStatusDelegate)Marshal.GetDelegateForFunctionPointer(setStatus, typeof(SetStatusDelegate));
            _setPosition = (SetPositionDelegate)Marshal.GetDelegateForFunctionPointer(setPosition, typeof(SetPositionDelegate));
            logger.Info("In-game status overlay connected.");
        }

        public static void ShowLoading(LoaderConfig config)
        {
            Publish("RROML Loading...", 0, config, 0);
        }

        public static void ShowSuccess(LoaderConfig config, int loadedModCount)
        {
            Publish("RROML Loaded Successfully, " + loadedModCount + " mods loaded.", 1, config, 4000);
        }

        public static void ShowFailure(LoaderConfig config)
        {
            Publish("RROML Failed to Load", 2, config, 6000);
        }

        public static void ShowWarning(LoaderConfig config, int loadedModCount)
        {
            Publish("RROML Loaded with Warnings, " + loadedModCount + " mods loaded.", 3, config, 5000);
        }

        private static void Publish(string message, int statusKind, LoaderConfig config, int autoHideMs)
        {
            if (config == null || !config.ShowOverlay || _setStatus == null)
            {
                return;
            }

            try
            {
                _setPosition(string.Equals(config.OverlayPosition, "TopRight", StringComparison.OrdinalIgnoreCase) ? 1 : 0);
                _setStatus(message, statusKind, autoHideMs);
                if (_logger != null)
                {
                    _logger.Info("In-game overlay state updated: " + message);
                }
            }
            catch (Exception exception)
            {
                if (_logger != null)
                {
                    _logger.Warn("Could not update in-game status overlay: " + exception.Message);
                }
            }
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr GetModuleHandle(string moduleName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr module, string procName);
    }
}
