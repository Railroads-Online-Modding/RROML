using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using RROML.Abstractions;
using RROML.Core.Lua;

namespace RROML.Core
{
    internal sealed class ModLoader
    {
        private readonly LoaderConfig _config;
        private readonly FileLogger _logger;
        private readonly string _gameRootPath;
        private readonly string _loaderPath;
        private readonly string _modsPath;

        public ModLoader(LoaderConfig config, FileLogger logger, string gameRootPath, string loaderPath, string modsPath)
        {
            _config = config;
            _logger = logger;
            _gameRootPath = gameRootPath;
            _loaderPath = loaderPath;
            _modsPath = modsPath;
        }

        public ModLoadSummary LoadAll()
        {
            var summary = new ModLoadSummary();
            var candidates = FindCandidates();
            summary.CandidateCount = candidates.Count;
            _logger.Info("Found " + candidates.Count + " mod candidate(s).");

            foreach (var candidate in OrderCandidates(candidates))
            {
                try
                {
                    summary.LoadedModCount += LoadOne(candidate);
                }
                catch (Exception exception)
                {
                    summary.FailedModCount++;
                    var path = candidate.Kind == ModKind.Lua ? candidate.LuaPath : candidate.AssemblyPath;
                    _logger.Error("Failed to load mod: " + path, exception);
                }
            }

            return summary;
        }

        private List<ModCandidate> FindCandidates()
        {
            var result = new List<ModCandidate>();
            if (!Directory.Exists(_modsPath))
            {
                Directory.CreateDirectory(_modsPath);
                return result;
            }

            foreach (var dllPath in Directory.GetFiles(_modsPath, "*.dll", SearchOption.TopDirectoryOnly))
            {
                result.Add(new ModCandidate
                {
                    ModRoot = _modsPath,
                    AssemblyPath = dllPath,
                    Kind = ModKind.Dll,
                    Manifest = null
                });
            }

            foreach (var luaPath in Directory.GetFiles(_modsPath, "*.lua", SearchOption.TopDirectoryOnly))
            {
                result.Add(new ModCandidate
                {
                    ModRoot = _modsPath,
                    LuaPath = luaPath,
                    Kind = ModKind.Lua,
                    Manifest = null
                });
            }

            foreach (var directory in Directory.GetDirectories(_modsPath))
            {
                var manifestPath = Path.Combine(directory, "mod.json");
                var manifest = SimpleJson.ReadFile<ModManifest>(manifestPath);
                bool handled = false;

                if (manifest != null)
                {
                    bool hasDll = !string.IsNullOrWhiteSpace(manifest.EntryDll);
                    bool hasLua = !string.IsNullOrWhiteSpace(manifest.EntryLua);

                    if (hasLua && hasDll)
                    {
                        _logger.Warn("Folder " + directory + " defines both EntryDll and EntryLua. Loading both.");
                    }

                    if (hasLua)
                    {
                        var luaCandidate = TryCreateLuaCandidate(directory, manifest, manifest.EntryLua);
                        if (luaCandidate != null)
                        {
                            result.Add(luaCandidate);
                            handled = true;
                        }
                    }

                    if (hasDll)
                    {
                        var dllCandidate = TryCreateDllCandidate(directory, manifest, manifest.EntryDll);
                        if (dllCandidate != null)
                        {
                            result.Add(dllCandidate);
                            handled = true;
                        }
                    }

                    if (handled)
                    {
                        continue;
                    }

                    // Manifest exists but no valid entry -> guess
                    _logger.Warn("Manifest in " + directory + " has no valid EntryDll or EntryLua. Attempting to guess entry.");
                }

                // No manifest or guess fallback: try to find any dll or lua
                var guessedDll = Directory.GetFiles(directory, "*.dll", SearchOption.TopDirectoryOnly).FirstOrDefault();
                var guessedLua = FindGuessedLua(directory);

                if (guessedDll != null && guessedLua != null)
                {
                    // Prefer to add both as separate candidates with different ids (file name based)
                    result.Add(new ModCandidate { ModRoot = directory, AssemblyPath = guessedDll, Kind = ModKind.Dll, Manifest = manifest });
                    result.Add(new ModCandidate { ModRoot = directory, LuaPath = guessedLua, Kind = ModKind.Lua, Manifest = manifest });
                    continue;
                }

                if (guessedDll != null)
                {
                    result.Add(new ModCandidate { ModRoot = directory, AssemblyPath = guessedDll, Kind = ModKind.Dll, Manifest = manifest });
                    continue;
                }

                if (guessedLua != null)
                {
                    result.Add(new ModCandidate { ModRoot = directory, LuaPath = guessedLua, Kind = ModKind.Lua, Manifest = manifest });
                    continue;
                }

                _logger.Warn("Skipping folder with no mod.json entry and no top-level DLL/Lua: " + directory);
            }

            return result;
        }

        private ModCandidate TryCreateLuaCandidate(string directory, ModManifest manifest, string entryLua)
        {
            var luaPath = Path.Combine(directory, entryLua);
            if (!File.Exists(luaPath))
            {
                _logger.Warn("Skipping folder Lua mod because entry Lua is missing: " + luaPath);
                return null;
            }
            return new ModCandidate { ModRoot = directory, LuaPath = luaPath, Kind = ModKind.Lua, Manifest = manifest };
        }

        private ModCandidate TryCreateDllCandidate(string directory, ModManifest manifest, string entryDll)
        {
            var dllPath = Path.Combine(directory, entryDll);
            if (!File.Exists(dllPath))
            {
                _logger.Warn("Skipping folder mod because entry DLL is missing: " + dllPath);
                return null;
            }
            return new ModCandidate { ModRoot = directory, AssemblyPath = dllPath, Kind = ModKind.Dll, Manifest = manifest };
        }

        private static string FindGuessedLua(string directory)
        {
            var candidates = new[] { "main.lua", "init.lua", "mod.lua" };
            foreach (var name in candidates)
            {
                var path = Path.Combine(directory, name);
                if (File.Exists(path)) return path;
            }
            return Directory.GetFiles(directory, "*.lua", SearchOption.TopDirectoryOnly).FirstOrDefault();
        }

        private List<ModCandidate> OrderCandidates(List<ModCandidate> candidates)
        {
            var ordered = new List<ModCandidate>();
            var byId = new Dictionary<string, ModCandidate>(StringComparer.OrdinalIgnoreCase);
            var visited = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);

            foreach (var candidate in candidates)
            {
                var candidateId = GetCandidateId(candidate);
                if (byId.ContainsKey(candidateId))
                {
                    _logger.Warn("Duplicate mod id detected. Keeping first and skipping later candidate: " + candidateId);
                    continue;
                }

                byId[candidateId] = candidate;
            }

            foreach (var pair in byId)
            {
                Visit(pair.Key, byId, visited, ordered);
            }

            return ordered;
        }

        private void Visit(string candidateId, Dictionary<string, ModCandidate> byId, Dictionary<string, int> visited, List<ModCandidate> ordered)
        {
            int state;
            if (visited.TryGetValue(candidateId, out state))
            {
                if (state == 1)
                {
                    _logger.Warn("Dependency cycle detected while ordering mods around " + candidateId + ". Continuing with best-effort order.");
                }
                return;
            }

            ModCandidate candidate;
            if (!byId.TryGetValue(candidateId, out candidate))
            {
                return;
            }

            visited[candidateId] = 1;
            foreach (var dependencyId in GetDependencies(candidate))
            {
                ModCandidate dependency;
                if (!byId.TryGetValue(dependencyId, out dependency))
                {
                    _logger.Warn("Mod " + candidateId + " declares missing dependency " + dependencyId + ". It will still be attempted after available dependencies.");
                    continue;
                }

                Visit(dependencyId, byId, visited, ordered);
            }

            visited[candidateId] = 2;
            if (!ordered.Contains(candidate))
            {
                ordered.Add(candidate);
            }
        }

        private int LoadOne(ModCandidate candidate)
        {
            var candidateId = GetCandidateId(candidate);
            var displayName = candidate.Kind == ModKind.Lua ? candidate.LuaPath : candidate.AssemblyPath;
            var fileNameWithoutExt = Path.GetFileNameWithoutExtension(displayName ?? candidateId);

            if (IsDisabled(candidate, fileNameWithoutExt) || IsDisabled(candidate, candidateId))
            {
                _logger.Info("Skipping disabled mod: " + fileNameWithoutExt + " (" + candidateId + ")");
                return 0;
            }

            var missingDependencies = GetDependencies(candidate)
                .Where(IsDependencyDisabled)
                .ToArray();
            if (missingDependencies.Length > 0)
            {
                _logger.Warn("Skipping mod " + candidateId + " because dependencies are disabled: " + string.Join(", ", missingDependencies));
                return 0;
            }

            if (candidate.Kind == ModKind.Lua)
            {
                return LoadLuaOne(candidate, candidateId);
            }

            return LoadDllOne(candidate, candidateId);
        }

        private int LoadDllOne(ModCandidate candidate, string candidateId)
        {
            var loadedCount = 0;
            var assemblyFileName = Path.GetFileNameWithoutExtension(candidate.AssemblyPath);
            var assembly = Assembly.LoadFrom(candidate.AssemblyPath);
            var modTypes = assembly.GetTypes()
                .Where(type => typeof(IRromlMod).IsAssignableFrom(type) && !type.IsAbstract && type.IsClass)
                .ToArray();

            if (modTypes.Length == 0)
            {
                _logger.Warn("No IRromlMod implementations found in " + candidate.AssemblyPath);
                return 0;
            }

            foreach (var modType in modTypes)
            {
                IRromlMod mod = null;

                try
                {
                    mod = (IRromlMod)Activator.CreateInstance(modType);
                }
                catch (Exception exception)
                {
                    _logger.Error("Could not create mod type " + modType.FullName, exception);
                    continue;
                }

                if (IsDisabled(candidate, mod.Id) || IsDisabled(candidate, mod.Name))
                {
                    _logger.Info("Skipping disabled mod instance: " + mod.Name);
                    continue;
                }

                var configRoot = Path.Combine(_loaderPath, "Configs", SanitizeName(mod.Id ?? mod.Name ?? assemblyFileName));
                var context = new ModContext(_gameRootPath, _loaderPath, _modsPath, configRoot, _logger);

                _logger.Info("Loading mod " + mod.Name + " (" + mod.Version + ")");
                mod.OnLoad(context);
                _logger.Info("Loaded mod " + mod.Name);
                loadedCount++;
            }

            return loadedCount;
        }

        private int LoadLuaOne(ModCandidate candidate, string candidateId)
        {
            var safeName = SanitizeName(candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Id) ? candidate.Manifest.Id : candidateId);
            var configRoot = Path.Combine(_loaderPath, "Configs", safeName);
            var context = new ModContext(_gameRootPath, _loaderPath, _modsPath, configRoot, _logger);

            if (IsDisabled(candidate, safeName))
            {
                _logger.Info("Skipping disabled Lua mod instance: " + safeName);
                return 0;
            }

            var success = LuaModRunner.Run(candidate, context, _logger);
            return success ? 1 : 0;
        }

        private IEnumerable<string> GetDependencies(ModCandidate candidate)
        {
            if (candidate.Manifest == null || candidate.Manifest.Dependencies == null)
            {
                return new string[0];
            }

            return candidate.Manifest.Dependencies.Where(item => !string.IsNullOrWhiteSpace(item));
        }

        private bool IsDependencyDisabled(string dependencyId)
        {
            return _config.DisabledMods != null && _config.DisabledMods.Any(item => string.Equals(item, dependencyId, StringComparison.OrdinalIgnoreCase));
        }

        private bool IsDisabled(ModCandidate candidate, string name)
        {
            if (string.IsNullOrWhiteSpace(name) || _config.DisabledMods == null)
            {
                return false;
            }

            return _config.DisabledMods.Any(item => string.Equals(item, name, StringComparison.OrdinalIgnoreCase));
        }

        private static string GetCandidateId(ModCandidate candidate)
        {
            if (candidate.Manifest != null && !string.IsNullOrWhiteSpace(candidate.Manifest.Id))
            {
                return candidate.Manifest.Id;
            }

            if (candidate.Kind == ModKind.Lua && !string.IsNullOrWhiteSpace(candidate.LuaPath))
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

            if (!string.IsNullOrWhiteSpace(candidate.AssemblyPath))
            {
                return Path.GetFileNameWithoutExtension(candidate.AssemblyPath);
            }

            if (!string.IsNullOrWhiteSpace(candidate.LuaPath))
            {
                return Path.GetFileNameWithoutExtension(candidate.LuaPath);
            }

            return "UnknownMod";
        }

        private static bool IsGenericLuaFileName(string fileName)
        {
            return string.Equals(fileName, "main", StringComparison.OrdinalIgnoreCase)
                || string.Equals(fileName, "init", StringComparison.OrdinalIgnoreCase)
                || string.Equals(fileName, "mod", StringComparison.OrdinalIgnoreCase);
        }

        private static string SanitizeName(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return "UnknownMod";
            }

            foreach (var invalidChar in Path.GetInvalidFileNameChars())
            {
                value = value.Replace(invalidChar, '_');
            }

            return value;
        }
    }
}
