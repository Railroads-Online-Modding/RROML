using System;

namespace RROML.Abstractions
{
    public interface IRromlMod
    {
        string Id { get; }
        string Name { get; }
        string Version { get; }
        void OnLoad(IModContext context);
    }

    public interface IModContext
    {
        string GameRootPath { get; }
        string LoaderPath { get; }
        string ModsPath { get; }
        string UserGameConfigRootPath { get; }
        IModLogger Logger { get; }
        string GetConfigPath(string fileName);
        string GetUserGameConfigPath(string fileName);
    }

    public interface IModLogger
    {
        void Info(string message);
        void Warn(string message);
        void Error(string message);
        void Error(string message, Exception exception);
    }
}