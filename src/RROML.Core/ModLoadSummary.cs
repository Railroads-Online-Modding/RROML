namespace RROML.Core
{
    internal sealed class ModLoadSummary
    {
        public int CandidateCount { get; set; }
        public int LoadedModCount { get; set; }
        public int FailedModCount { get; set; }
        public int WarningCount { get; set; }
    }
}
