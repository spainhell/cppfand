using System.IO;
using System.Reflection;
using System.Xml.Linq;

namespace WpfHost;

/// <summary>
/// Nastavení z &lt;appSettings&gt; v cppfand-wpf.exe.config (u .NET 10 cppfand-wpf.dll.config).
/// Čte se přímo jako XML, aby stačilo jedno řešení pro .NET Framework i .NET 10.
/// </summary>
public static class HostSettings
{
    private static Dictionary<string, string>? _values;

    /// <summary>Šířka obrazovky ve znacích; 0 = podle FAND.CFG (obvykle 80).</summary>
    public static int ScreenCols => GetInt("ScreenCols");

    /// <summary>Výška obrazovky ve znacích; 0 = podle FAND.CFG (obvykle 25).</summary>
    public static int ScreenRows => GetInt("ScreenRows");

    private static int GetInt(string key) =>
        Values.TryGetValue(key, out string? s) && int.TryParse(s.Trim(), out int v) && v > 0 ? v : 0;

    private static Dictionary<string, string> Values => _values ??= Load();

    private static Dictionary<string, string> Load()
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        string? file = ConfigFile();
        if (file == null) return result;
        try
        {
            var settings = XDocument.Load(file).Root?.Element("appSettings");
            if (settings == null) return result;
            foreach (var add in settings.Elements("add"))
            {
                string? key = (string?)add.Attribute("key");
                if (!string.IsNullOrEmpty(key)) result[key!] = (string?)add.Attribute("value") ?? "";
            }
        }
        catch (Exception)
        {
            // poškozený config: pokračujeme s výchozími hodnotami
        }
        return result;
    }

    private static string? ConfigFile()
    {
        // .NET Framework: cppfand-wpf.exe.config, .NET 10: cppfand-wpf.dll.config
        string assembly = Assembly.GetEntryAssembly()?.Location ?? "";
        string exe = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "cppfand-wpf.exe");
        foreach (string candidate in new[] { exe + ".config", assembly + ".config" })
        {
            if (candidate.Length > ".config".Length && File.Exists(candidate)) return candidate;
        }
        return null;
    }
}
