using System;
using System.IO;
using RROML.Abstractions;

namespace ExampleMod
{
    public sealed class ExampleMod : IRromlMod
    {
        public string Id { get { return "ExampleMod"; } }
        public string Name { get { return "Example Mod (C#)"; } }
        public string Version { get { return "1.0.0"; } }

        public void OnLoad(IModContext context)
        {
            var log = context.Logger;

            log.Info("[" + Name + " v" + Version + "] OnLoad called");
            log.Info("  Game root : " + context.GameRootPath);
            log.Info("  Loader path: " + context.LoaderPath);
            log.Info("  Mods path  : " + context.ModsPath);
            log.Info("  Mod id     : " + Id);
            log.Info("  Mod version: " + Version);

            var myConfig = context.GetConfigPath("settings.json");
            var userConfig = context.GetUserGameConfigPath("Engine.ini");
            log.Info("  My config path  : " + myConfig);
            log.Info("  User game config: " + userConfig);
            log.Info("  Config folder   : " + Path.GetDirectoryName(myConfig));

            try
            {
                if (!File.Exists(myConfig))
                {
                    log.Info("  No existing config, creating default at " + myConfig);
                    var dir = Path.GetDirectoryName(myConfig);
                    if (!Directory.Exists(dir))
                    {
                        Directory.CreateDirectory(dir);
                    }

                    var defaultJson = "{\n  \"enabled\": true,\n  \"message\": \"Hello from C# ExampleMod!\"\n}\n";
                    File.WriteAllText(myConfig, defaultJson);
                    log.Info("  Default config written");
                }
                else
                {
                    log.Info("  Config already exists, skipping creation");
                    var text = File.ReadAllText(myConfig);
                    log.Info("  Config preview: " + text.Substring(0, Math.Min(80, text.Length)).Replace("\r", "").Replace("\n", " "));
                }
            }
            catch (Exception ex)
            {
                log.Warn("  Could not write/read config: " + ex.Message);
                log.Error("Config error details", ex);
            }

            var sum = 0;
            for (var i = 1; i <= 5; i++) sum += i;
            log.Info("  C# loop test sum 1..5 = " + sum);

            var greeting = string.Format("Hello from {0}! RROML {1} is running .NET {2}", Id, "1.2.0", Environment.Version);
            log.Info("  " + greeting);

            log.Info("[" + Name + "] OnLoad finished - mod is active");
        }
    }
}
