using System.Windows;

namespace WpfHost;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        StartOptions? options = FromArgs(e.Args);
        if (options != null)
        {
            if (!StartOptions.HasFandFiles(options.FandDir))
            {
                MessageBox.Show($"Ve složce {options.FandDir} není FAND.CFG a FAND.RES.",
                    "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Warning);
                Shutdown();
                return;
            }
        }
        else
        {
            // WPF si za MainWindow bere prvni vytvorene okno, takze pri
            // ShutdownMode=OnMainWindowClose by zavreni dialogu ukoncilo celou
            // aplikaci jeste driv, nez staci vzniknout MainWindow.
            ShutdownMode = ShutdownMode.OnExplicitShutdown;

            string? fandDir = StartOptions.FindFandDir();
            if (fandDir != null)
            {
                // Bez parametru se jde rovnou do hlavniho menu FANDu, jako to delal
                // PC-FAND: runfand.cpp:389 preskoci vetveni a vykresli plochu s menu.
                // Ulohu si uzivatel vybere tam, ptat se na ni predem netreba.
                options = new StartOptions { FandDir = fandDir, WorkDir = fandDir };
            }
            else
            {
                // Slozku s FAND.CFG a FAND.RES jsme nenasli, bez ni se spustit neda
                var dlg = new StartWindow(StartOptions.Load(), askForDirs: true);
                if (dlg.ShowDialog() != true || dlg.Result == null)
                {
                    Shutdown();
                    return;
                }
                options = dlg.Result;
            }
        }

        var main = new MainWindow(options);
        MainWindow = main;
        ShutdownMode = ShutdownMode.OnMainWindowClose;
        main.Show();
    }

    // <uloha>                                 -- obe cesty jsou slozka s FAND.CFG (jako 'ufand ucto2024')
    // <slozka> <uloha>                        -- slozka je i pracovni adresar
    // <slozka FANDu> <pracovni adresar> <uloha>
    // za tim volitelne D nebo T jako v PC-FANDu, viz ParseMode
    // bez parametru -> null, tedy dialog
    private static StartOptions? FromArgs(string[] args)
    {
        string mode = ParseMode(ref args);

        string dir = StartOptions.FindFandDir() ?? Environment.CurrentDirectory.TrimEnd('\\');
        StartOptions? options;
        switch (args.Length)
        {
            case 0: options = null; break;
            case 1: options = Make(dir, dir, args[0]); break;
            case 2: options = Make(args[0], args[0], args[1]); break;
            default: options = Make(args[0], args[1], args[2]); break;
        }

        if (options != null) options.Mode = mode;
        return options;
    }

    /// <summary>
    /// Odřízne z konce poslední parametr, je-li to režim PC-FANDu: D ladicí běh
    /// nebo T editace textového souboru. Rozhoduje se podle přesné shody, takže
    /// úloha jménem "D" by se musela zadat i s cestou; jiný způsob to nemá,
    /// protože původní FAND bral režim prostě jako druhý parametr.
    /// </summary>
    private static string ParseMode(ref string[] args)
    {
        if (args.Length < 2) return "";

        string last = args[args.Length - 1];
        if (!last.Equals(StartOptions.ModeDebug, StringComparison.OrdinalIgnoreCase) &&
            !last.Equals(StartOptions.ModeText, StringComparison.OrdinalIgnoreCase))
            return "";

        var rest = new string[args.Length - 1];
        Array.Copy(args, rest, rest.Length);
        args = rest;
        return last.ToUpperInvariant();
    }

    private static StartOptions Make(string fandDir, string workDir, string rdbName) => new StartOptions
    {
        FandDir = fandDir.TrimEnd('\\'),
        WorkDir = workDir.TrimEnd('\\'),
        RdbName = rdbName,
    };
}
