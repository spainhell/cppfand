using System.IO;
using System.Windows;

namespace WpfHost;

public sealed class StartOptions
{
    public string FandDir { get; set; } = "";
    public string WorkDir { get; set; } = "";
    public string RdbName { get; set; } = "";

    // prosty textovy soubor klic=hodnota (bez zavislosti na JSON knihovne, aby vedle exe nic nebylo)
    private static string SettingsPath =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "cppfand", "wpfhost.txt");

    public static StartOptions Load()
    {
        var opt = new StartOptions();
        try
        {
            if (!File.Exists(SettingsPath)) return opt;
            foreach (string line in File.ReadAllLines(SettingsPath))
            {
                int eq = line.IndexOf('=');
                if (eq <= 0) continue;
                string key = line.Substring(0, eq).Trim();
                string value = line.Substring(eq + 1).Trim();
                switch (key)
                {
                    case "FandDir": opt.FandDir = value; break;
                    case "WorkDir": opt.WorkDir = value; break;
                    case "RdbName": opt.RdbName = value; break;
                }
            }
        }
        catch { }
        return opt;
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
            File.WriteAllLines(SettingsPath, new[] { $"FandDir={FandDir}", $"WorkDir={WorkDir}", $"RdbName={RdbName}" });
        }
        catch { }
    }
}

public partial class StartWindow : Window
{
    public StartOptions? Result { get; private set; }

    public StartWindow(StartOptions defaults)
    {
        InitializeComponent();
        FandDirBox.Text = defaults.FandDir;
        WorkDirBox.Text = defaults.WorkDir;
        RdbBox.Text = defaults.RdbName;
        if (FandDirBox.Text.Length == 0) FandDirBox.Text = AppContext.BaseDirectory.TrimEnd('\\');
        if (WorkDirBox.Text.Length == 0) WorkDirBox.Text = FandDirBox.Text;
    }

    private void BrowseFandDir(object sender, RoutedEventArgs e) => Browse(FandDirBox);
    private void BrowseWorkDir(object sender, RoutedEventArgs e) => Browse(WorkDirBox);

    private static void Browse(System.Windows.Controls.TextBox box)
    {
        string? initial = Directory.Exists(box.Text) ? box.Text : null;
#if NETFRAMEWORK
        // WPF v .NET Framework nema dialog pro slozku, pouzijeme Windows Forms
        using var dlg = new System.Windows.Forms.FolderBrowserDialog { SelectedPath = initial ?? "" };
        if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK) box.Text = dlg.SelectedPath;
#else
        var dlg = new Microsoft.Win32.OpenFolderDialog { InitialDirectory = initial };
        if (dlg.ShowDialog() == true) box.Text = dlg.FolderName;
#endif
    }

    private void Start(object sender, RoutedEventArgs e)
    {
        var opt = new StartOptions
        {
            FandDir = FandDirBox.Text.Trim().TrimEnd('\\'),
            WorkDir = WorkDirBox.Text.Trim().TrimEnd('\\'),
            RdbName = RdbBox.Text.Trim(),
        };
        if (!File.Exists(Path.Combine(opt.FandDir, "FAND.CFG")) || !File.Exists(Path.Combine(opt.FandDir, "FAND.RES")))
        {
            MessageBox.Show(this, "Ve složce musí být FAND.CFG a FAND.RES.", "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (!Directory.Exists(opt.WorkDir))
        {
            MessageBox.Show(this, "Pracovní adresář neexistuje.", "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (opt.RdbName.Length == 0 || opt.RdbName.IndexOfAny(new[] { '\\', '/', ':', '.' }) >= 0)
        {
            MessageBox.Show(this, "Název úlohy musí být identifikátor bez cesty a přípony.", "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        opt.Save();
        Result = opt;
        DialogResult = true;
    }
}
