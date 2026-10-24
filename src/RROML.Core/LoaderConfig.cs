using System;
using System.Collections.Generic;

namespace RROML.Core
{
    public sealed class LoaderConfig
    {
        public bool Enabled { get; set; }
        public bool VerboseLogging { get; set; }
        public bool ShowOverlay { get; set; }
        public string OverlayPosition { get; set; }
        public List<string> DisabledMods { get; set; }

        public LoaderConfig()
        {
            Enabled = true;
            VerboseLogging = true;
            ShowOverlay = true;
            OverlayPosition = "TopLeft";
            DisabledMods = new List<string>();
        }
    }
}
