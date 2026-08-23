using System;
using System.IO;

namespace RROML.Core
{
    public static class Bootstrap
    {
        private static bool _hasRun;

        public static void Initialize()
        {
            InitializeCore();
        }

        public static int InitializeForProxy(string _)
        {
            InitializeCore();
            return 0;
        }

        private static void InitializeCore()
        {
            LoaderConfig config = null;
            FileLogger logger = null;
            string loaderPath = null;

            if (_hasRun)
            {
                return;
            }

            _hasRun = true;

            try
            {
                var gameRoot = FindGameRoot();
                loaderPath = Path.Combine(gameRoot, "RROML");
                var modsPath = Path.Combine(gameRoot, "Mods");
                var logsPath = Path.Combine(loaderPath, "Logs");
                Directory.CreateDirectory(loaderPath);
                Directory.CreateDirectory(modsPath);
                Directory.CreateDirectory(logsPath);

                var logPath = Path.Combine(logsPath, "rroml.log");
                logger = new FileLogger(logPath);
                logger.Info("RROML bootstrap starting. Version " + RROMLVersion.Version + " (" + RROMLVersion.Engine + " Lua " + RROMLVersion.LuaVersion + ")");
                logger.Info("Game root: " + gameRoot);
                logger.Info("Loader path: " + loaderPath);
                logger.Info("Mods path: " + modsPath);

                var configPath = Path.Combine(loaderPath, "RROML.config.json");
                logger.Info("Loading config from " + configPath);
                config = SimpleJson.ReadFile<LoaderConfig>(configPath) ?? new LoaderConfig();
                if (!File.Exists(configPath))
                {
                    logger.Warn("Config file did not exist. Writing a new default config.");
                    SimpleJson.WriteFile(configPath, config);
                }

                logger.Info("Config loaded. Enabled=" + config.Enabled + ", ShowOverlay=" + config.ShowOverlay + ", OverlayPosition=" + config.OverlayPosition);
                StatusOverlay.Initialize(loaderPath, logger);
                TryShowOverlay(logger, config, delegate { StatusOverlay.ShowLoading(config); }, "show loading overlay");

                if (!config.Enabled)
                {
                    logger.Warn("RROML is disabled in config.");
                    TryShowOverlay(logger, config, delegate { StatusOverlay.ShowFailure(config); }, "show failure overlay for disabled loader");
                    return;
                }

                logger.Info("Creating mod loader.");
                var loader = new ModLoader(config, logger, gameRoot, loaderPath, modsPath);
                logger.Info("Starting mod load pass.");
                var summary = loader.LoadAll();
                logger.Info("Mod load pass finished. Candidates=" + summary.CandidateCount + ", Loaded=" + summary.LoadedModCount + ", Failed=" + summary.FailedModCount);
                logger.Info("RROML bootstrap finished.");
                TryShowOverlay(logger, config, delegate { StatusOverlay.ShowSuccess(config, summary.LoadedModCount); }, "show success overlay");
            }
            catch (Exception exception)
            {
                if (logger != null)
                {
                    logger.Error("RROML bootstrap failed.", exception);
                }
                else
                {
                    WriteEmergencyError(exception);
                }

                try
                {
                    if (loaderPath != null && logger != null)
                    {
                        StatusOverlay.Initialize(loaderPath, logger);
                    }

                    TryShowOverlay(logger, config ?? new LoaderConfig(), delegate { StatusOverlay.ShowFailure(config ?? new LoaderConfig()); }, "show failure overlay");
                }
                catch
                {
                }
            }
        }

        private static void TryShowOverlay(FileLogger logger, LoaderConfig config, Action action, string stepName)
        {
            if (config == null || !config.ShowOverlay)
            {
                return;
            }

            try
            {
                logger.Info("Trying to " + stepName + ".");
                action();
                logger.Info("Overlay step succeeded: " + stepName + ".");
            }
            catch (Exception exception)
            {
                logger.Warn("Overlay step failed: " + stepName + ". Overlay will be ignored for this run.");
                logger.Error("Overlay exception details.", exception);
            }
        }

        private static void WriteEmergencyError(Exception exception)
        {
            try
            {
                var emergencyPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "RROML_bootstrap_error.txt");
                File.WriteAllText(emergencyPath, exception.ToString());
            }
            catch
            {
            }
        }

        private static string FindGameRoot()
        {
            var baseDirectory = AppDomain.CurrentDomain.BaseDirectory;
            var current = new DirectoryInfo(baseDirectory);

            while (current != null)
            {
                var arrExe = Path.Combine(current.FullName, "arr.exe");
                if (File.Exists(arrExe))
                {
                    return current.FullName;
                }

                current = current.Parent;
            }

            throw new DirectoryNotFoundException("Could not locate Railroads Online root from " + baseDirectory);
        }
    }
}
