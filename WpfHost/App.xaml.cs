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

            var defaults = StartOptions.Load();
            string? fandDir = StartOptions.FindFandDir();
            if (fandDir != null)
            {
                // cesty jsou jasne, na ty uz se neptame; zbyva jen uloha
                defaults.FandDir = fandDir;
                defaults.WorkDir = fandDir;
                defaults.RdbName = StartOptions.ResolveRdbName(fandDir, defaults.RdbName);
            }

            var dlg = new StartWindow(defaults, askForDirs: fandDir == null);
            if (dlg.ShowDialog() != true || dlg.Result == null)
            {
                Shutdown();
                return;
            }
            options = dlg.Result;
        }

        var main = new MainWindow(options);
        MainWindow = main;
        ShutdownMode = ShutdownMode.OnMainWindowClose;
        main.Show();
    }

    // <uloha>                                 -- obe cesty jsou slozka s FAND.CFG (jako 'ufand ucto2024')
    // <slozka> <uloha>                        -- slozka je i pracovni adresar
    // <slozka FANDu> <pracovni adresar> <uloha>
    // bez parametru -> null, tedy dialog
    private static StartOptions? FromArgs(string[] args)
    {
        string dir = StartOptions.FindFandDir() ?? Environment.CurrentDirectory.TrimEnd('\\');
        switch (args.Length)
        {
            case 0: return null;
            case 1: return Make(dir, dir, args[0]);
            case 2: return Make(args[0], args[0], args[1]);
            default: return Make(args[0], args[1], args[2]);
        }
    }

    private static StartOptions Make(string fandDir, string workDir, string rdbName) => new StartOptions
    {
        FandDir = fandDir.TrimEnd('\\'),
        WorkDir = workDir.TrimEnd('\\'),
        RdbName = rdbName,
    };
}
