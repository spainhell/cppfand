using System.IO;
using System.Windows;

namespace WpfHost;

public sealed class StartOptions
{
    public string FandDir { get; set; } = "";
    public string WorkDir { get; set; } = "";

    /// <summary>Úloha; v režimu <see cref="ModeText"/> cesta k textovému souboru.</summary>
    public string RdbName { get; set; } = "";

    /// <summary>
    /// Třetí parametr PC-FANDu (paramstr[2]). Větví se podle něj runfand.cpp:389
    /// stejně jako originál v RUNFAND.PAS:364.
    /// </summary>
    public string Mode { get; set; } = "";

    public const string ModeDebug = "D";
    public const string ModeText = "T";

    // Ma slozka to, co FAND potrebuje ke startu?
    public static bool HasFandFiles(string dir) =>
        dir.Length > 0 && File.Exists(Path.Combine(dir, "FAND.CFG")) && File.Exists(Path.Combine(dir, "FAND.RES"));

    // Uloha se obvykle nasazuje tak, ze hostitel lezi primo v jejim adresari,
    // takze FAND.CFG a FAND.RES hledame vedle exe a teprve potom v aktualnim
    // adresari. Kdyz nejsou ani tam, nevime a musime se zeptat.
    public static string? FindFandDir()
    {
        string exeDir = AppContext.BaseDirectory.TrimEnd('\\');
        if (HasFandFiles(exeDir)) return exeDir;
        string cwd = Environment.CurrentDirectory.TrimEnd('\\');
        if (HasFandFiles(cwd)) return cwd;
        return null;
    }

    // Ulozena uloha plati jen tehdy, kdyz v teto slozce opravdu je; jinak vezmeme
    // jedinou .RDB, kterou tam najdeme (vic jich byva jen vyjimecne).
    public static string ResolveRdbName(string dir, string saved)
    {
        try
        {
            if (saved.Length > 0 && File.Exists(Path.Combine(dir, saved + ".RDB"))) return saved;
            string[] found = Directory.GetFiles(dir, "*.RDB");
            return found.Length == 1 ? Path.GetFileNameWithoutExtension(found[0]) : saved;
        }
        catch { return saved; }
    }

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

    // askForDirs == false: FAND.CFG a FAND.RES uz nekdo nasel, ptame se jen na ulohu
    public StartWindow(StartOptions defaults, bool askForDirs)
    {
        InitializeComponent();
        FandDirBox.Text = defaults.FandDir;
        WorkDirBox.Text = defaults.WorkDir;
        RdbBox.Text = defaults.RdbName;
        if (FandDirBox.Text.Length == 0) FandDirBox.Text = AppContext.BaseDirectory.TrimEnd('\\');
        if (WorkDirBox.Text.Length == 0) WorkDirBox.Text = FandDirBox.Text;

        if (!askForDirs)
        {
            Title = "C++ FAND – výběr úlohy";
            foreach (UIElement row in new UIElement[] { FandDirLabel, FandDirBox, FandDirBrowse, WorkDirLabel, WorkDirBox, WorkDirBrowse })
                row.Visibility = Visibility.Collapsed;
            HintText.Text = $"Úloha se hledá v {FandDirBox.Text}. Zadat ji lze i na příkazové řádce: cppfand-wpf.exe <úloha>";
        }

        Loaded += (_, _) => { RdbBox.Focus(); RdbBox.SelectAll(); };
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
        if (!StartOptions.HasFandFiles(opt.FandDir))
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
