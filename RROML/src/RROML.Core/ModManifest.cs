using System.Collections.Generic;

namespace RROML.Core
{
    public sealed class ModManifest
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public string EntryDll { get; set; }
        public string EntryLua { get; set; }
        public string Version { get; set; }
        public List<string> Dependencies { get; set; }

        public ModManifest()
        {
            Dependencies = new List<string>();
        }
    }
}
