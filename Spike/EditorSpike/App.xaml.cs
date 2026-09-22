using System.Windows;

namespace EditorSpike;

public partial class App : Application
{
    /// <summary>Soubor z prikazove radky; bez nej se otevre vzorek z Ucta.</summary>
    public static string? StartFile { get; private set; }

    protected override void OnStartup(StartupEventArgs e)
    {
        if (e.Args.Length > 0) StartFile = e.Args[0];
        base.OnStartup(e);
    }
}
