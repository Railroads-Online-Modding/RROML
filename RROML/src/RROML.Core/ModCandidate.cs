namespace RROML.Core
{
    internal enum ModKind
    {
        Dll,
        Lua
    }

    internal sealed class ModCandidate
    {
        public string ModRoot { get; set; }
        public string AssemblyPath { get; set; }
        public string LuaPath { get; set; }
        public ModKind Kind { get; set; }
        public ModManifest Manifest { get; set; }
    }
}
