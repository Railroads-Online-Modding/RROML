using System;
using System.IO;
using MoonSharp.Interpreter;
using MoonSharp.Interpreter.Loaders;
using RROML.Abstractions;

namespace RROML.Core.Lua
{
    internal static class LuaModRunner
    {
        public static bool Run(ModCandidate candidate, IModContext context, FileLogger logger)
        {
            if (candidate == null || string.IsNullOrWhiteSpace(candidate.LuaPath) || !File.Exists(candidate.LuaPath))
            {
                logger.Warn("Lua mod candidate has no valid Lua path: " + (candidate != null ? candidate.LuaPath : "null"));
                return false;
            }

            var modId = GetCandidateId(candidate);
            var luaFileName = Path.GetFileName(candidate.LuaPath);
            var modRoot = candidate.ModRoot ?? Path.GetDirectoryName(candidate.LuaPath) ?? context.ModsPath;

            logger.Info("Loading Lua mod " + modId + " from " + candidate.LuaPath);

            var script = CreateScript(modRoot, logger);

            try
            {
                InjectRromlApi(script, candidate, context, logger, modId, modRoot);

                var code = File.ReadAllText(candidate.LuaPath);
                var result = script.DoString(code, null, luaFileName);

                var invoked = TryInvokeOnLoad(script, result, logger, modId);

                if (!invoked)
                {
                    logger.Info("Lua mod " + modId + " executed without explicit OnLoad (no OnLoad function found, treating as successful load).");
                }
                else
                {
                    logger.Info("Loaded Lua mod " + modId);
                }

                return true;
            }
            catch (InterpreterException ex)
            {
                logger.Error("Lua error in mod " + modId + ": " + ex.DecoratedMessage, ex);
                return false;
            }
            catch (Exception ex)
            {
                logger.Error("Failed to run Lua mod " + modId, ex);
                return false;
            }
        }

        private static Script CreateScript(string modRoot, FileLogger logger)
        {
            var script = new Script(CoreModules.Preset_Default);

            var loader = new FileSystemScriptLoader();
            loader.ModulePaths = new[]
            {
                Path.Combine(modRoot, "?.lua"),
                Path.Combine(modRoot, "?", "init.lua"),
                Path.Combine(modRoot, "?", ".lua"),
                "?.lua",
                "?/init.lua"
            };
            script.Options.ScriptLoader = loader;
            script.Options.DebugPrint = delegate(string s)
            {
                logger.Info("[lua] " + s);
            };

            return script;
        }

        private static void InjectRromlApi(Script script, ModCandidate candidate, IModContext context, FileLogger logger, string modId, string modRoot)
        {
            var rroml = new Table(script);
            var logTable = new Table(script);
            logTable["info"] = (Action<string>)delegate(string msg) { logger.Info("[lua:" + modId + "] " + (msg ?? string.Empty)); };
            logTable["warn"] = (Action<string>)delegate(string msg) { logger.Warn("[lua:" + modId + "] " + (msg ?? string.Empty)); };
            logTable["error"] = (Action<string>)delegate(string msg) { logger.Error("[lua:" + modId + "] " + (msg ?? string.Empty)); };

            var pathsTable = new Table(script);
            pathsTable["gameRoot"] = context.GameRootPath;
            pathsTable["loaderPath"] = context.LoaderPath;
            pathsTable["modsPath"] = context.ModsPath;
            pathsTable["modRoot"] = modRoot;
            pathsTable["configRoot"] = context.GetConfigPath(string.Empty).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);

            var modTable = new Table(script);
            modTable["id"] = candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Id) ? candidate.Manifest.Id : modId;
            modTable["name"] = candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Name) ? candidate.Manifest.Name : modId;
            modTable["version"] = candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Version) ? candidate.Manifest.Version : "1.0.0";
            modTable["root"] = modRoot;
            modTable["entry"] = candidate.LuaPath;

            rroml["log"] = logTable;
            rroml["paths"] = pathsTable;
            rroml["mod"] = modTable;
            rroml["getConfigPath"] = (Func<string, string>)delegate(string fileName)
            {
                if (string.IsNullOrWhiteSpace(fileName)) return context.GetConfigPath(string.Empty);
                return context.GetConfigPath(fileName);
            };
            rroml["getUserGameConfigPath"] = (Func<string, string>)delegate(string fileName)
            {
                if (string.IsNullOrWhiteSpace(fileName)) return context.GetUserGameConfigPath(string.Empty);
                return context.GetUserGameConfigPath(fileName);
            };
            rroml["version"] = RROMLVersion.Version;

            script.Globals["rroml"] = rroml;
            script.Globals["RROML"] = rroml;

            script.Globals["print"] = (Action<string>)delegate(string msg)
            {
                logger.Info("[lua:" + modId + "] " + (msg ?? string.Empty));
            };
        }

        private static bool TryInvokeOnLoad(Script script, DynValue scriptReturn, FileLogger logger, string modId)
        {
            var onLoad = script.Globals.Get("OnLoad");
            if (!onLoad.IsNil() && onLoad.Type == DataType.Function)
            {
                try
                {
                    var rroml = script.Globals.Get("rroml");
                    script.Call(onLoad, rroml);
                    logger.Info("Lua mod " + modId + " OnLoad() invoked (global).");
                    return true;
                }
                catch (Exception ex)
                {
                    logger.Error("Lua mod " + modId + " OnLoad threw", ex);
                    throw;
                }
            }

            var onLoadLower = script.Globals.Get("onLoad");
            if (!onLoadLower.IsNil() && onLoadLower.Type == DataType.Function)
            {
                try
                {
                    var rroml = script.Globals.Get("rroml");
                    script.Call(onLoadLower, rroml);
                    logger.Info("Lua mod " + modId + " onLoad() invoked (global lower).");
                    return true;
                }
                catch (Exception ex)
                {
                    logger.Error("Lua mod " + modId + " onLoad threw", ex);
                    throw;
                }
            }

            if (scriptReturn != null && scriptReturn.Type == DataType.Table)
            {
                var table = scriptReturn.Table;
                var tableOnLoad = table.Get("OnLoad");
                if (!tableOnLoad.IsNil() && tableOnLoad.Type == DataType.Function)
                {
                    try
                    {
                        var rroml = script.Globals.Get("rroml");
                        script.Call(tableOnLoad, rroml);
                        logger.Info("Lua mod " + modId + " returned table OnLoad() invoked.");
                        return true;
                    }
                    catch (Exception ex)
                    {
                        logger.Error("Lua mod " + modId + " table OnLoad threw", ex);
                        throw;
                    }
                }

                var tableOnLoadLower = table.Get("onLoad");
                if (!tableOnLoadLower.IsNil() && tableOnLoadLower.Type == DataType.Function)
                {
                    try
                    {
                        var rroml = script.Globals.Get("rroml");
                        script.Call(tableOnLoadLower, rroml);
                        logger.Info("Lua mod " + modId + " returned table onLoad() invoked.");
                        return true;
                    }
                    catch (Exception ex)
                    {
                        logger.Error("Lua mod " + modId + " table onLoad threw", ex);
                        throw;
                    }
                }
            }

            return false;
        }

        private static string GetCandidateId(ModCandidate candidate)
        {
            if (candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Id))
            {
                return candidate.Manifest.Id;
            }
            if (!string.IsNullOrWhiteSpace(candidate.LuaPath))
            {
                var fileName = Path.GetFileNameWithoutExtension(candidate.LuaPath);
                if (IsGenericLuaFileName(fileName) && !string.IsNullOrWhiteSpace(candidate.ModRoot))
                {
                    var folderName = Path.GetFileName(candidate.ModRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
                    if (!string.IsNullOrWhiteSpace(folderName) && !string.Equals(folderName, "Mods", StringComparison.OrdinalIgnoreCase))
                    {
                        return folderName;
                    }
                }
                return fileName;
            }
            return Path.GetFileNameWithoutExtension(candidate.AssemblyPath ?? "UnknownLuaMod");
        }

        private static bool IsGenericLuaFileName(string fileName)
        {
            return string.Equals(fileName, "main", StringComparison.OrdinalIgnoreCase)
                || string.Equals(fileName, "init", StringComparison.OrdinalIgnoreCase)
                || string.Equals(fileName, "mod", StringComparison.OrdinalIgnoreCase);
        }
    }
}
