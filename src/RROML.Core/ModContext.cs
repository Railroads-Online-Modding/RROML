using System;
using System.IO;
using RROML.Abstractions;

namespace RROML.Core
{
    internal sealed class ModContext : IModContext
    {
        private readonly string _configRoot;
        private readonly string _userGameConfigRoot;

        public ModContext(string gameRootPath, string loaderPath, string modsPath, string configRoot, IModLogger logger)
        {
            GameRootPath = gameRootPath;
            LoaderPath = loaderPath;
            ModsPath = modsPath;
            _configRoot = configRoot;
            _userGameConfigRoot = ResolveUserGameConfigRoot(gameRootPath);
            Logger = logger;
        }

        public string GameRootPath { get; private set; }
        public string LoaderPath { get; private set; }
        public string ModsPath { get; private set; }
        public string UserGameConfigRootPath { get { return _userGameConfigRoot; } }
        public IModLogger Logger { get; private set; }

        public string GetConfigPath(string fileName)
        {
            if (!Directory.Exists(_configRoot))
            {
                Directory.CreateDirectory(_configRoot);
            }

            return Path.Combine(_configRoot, fileName);
        }

        public string GetUserGameConfigPath(string fileName)
        {
            if (!Directory.Exists(_userGameConfigRoot))
            {
                Directory.CreateDirectory(_userGameConfigRoot);
            }

            return Path.Combine(_userGameConfigRoot, fileName);
        }

        private static string ResolveUserGameConfigRoot(string gameRootPath)
        {
            var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            var candidates = new[]
            {
                Path.Combine(localAppData, "arr", "Saved", "Config", "Windows"),
                Path.Combine(localAppData, "arr", "Saved", "Config", "WindowsNoEditor"),
                Path.Combine(gameRootPath, "arr", "Saved", "Config", "Windows")
            };

            for (var i = 0; i < candidates.Length; i++)
            {
                if (Directory.Exists(candidates[i]))
                {
                    return candidates[i];
                }
            }

            return candidates[0];
        }
    }
}