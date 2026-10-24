using System.IO;
using System.Text;
using System.Web.Script.Serialization;

namespace RROML.Core
{
    internal static class SimpleJson
    {
        private static readonly JavaScriptSerializer Serializer = new JavaScriptSerializer();

        public static T ReadFile<T>(string path) where T : class
        {
            if (!File.Exists(path))
            {
                return null;
            }

            var text = File.ReadAllText(path, Encoding.UTF8);
            if (string.IsNullOrWhiteSpace(text))
            {
                return null;
            }

            return Serializer.Deserialize<T>(text);
        }

        public static void WriteFile(string path, object value)
        {
            var text = Serializer.Serialize(value);
            File.WriteAllText(path, text, Encoding.UTF8);
        }
    }
}
