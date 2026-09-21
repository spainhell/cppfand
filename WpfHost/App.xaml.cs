using System.Windows;

namespace WpfHost;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        StartOptions options;
        if (e.Args.Length >= 3)
        {
            options = new StartOptions
            {
                FandDir = e.Args[0].TrimEnd('\\'),
                WorkDir = e.Args[1].TrimEnd('\\'),
                RdbName = e.Args[2],
            };
        }
        else
        {
            var dlg = new StartWindow(StartOptions.Load());
            if (dlg.ShowDialog() != true || dlg.Result == null)
            {
                Shutdown();
                return;
            }
            options = dlg.Result;
        }

        var main = new MainWindow(options);
        MainWindow = main;
        main.Show();
    }
}
