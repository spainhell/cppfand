using System.Windows;

namespace WpfHost;

public partial class MailSettingsWindow : Window
{
    private readonly MailSettings _settings;

    public MailSettingsWindow(MailSettings settings)
    {
        InitializeComponent();
        _settings = settings;
        HostBox.Text = settings.SmtpHost;
        PortBox.Text = settings.SmtpPort.ToString();
        SslBox.IsChecked = settings.SmtpSsl;
        UserBox.Text = settings.SmtpUser;
        PasswordBox.Password = settings.SmtpPassword;
        FromBox.Text = settings.From;
        Loaded += (_, _) => HostBox.Focus();
    }

    private void Ok_Click(object sender, RoutedEventArgs e)
    {
        string host = HostBox.Text.Trim();
        if (!int.TryParse(PortBox.Text.Trim(), out int port) || port <= 0 || port > 65535)
        {
            MessageBox.Show(this, "Port musí být číslo 1–65535.", Title, MessageBoxButton.OK, MessageBoxImage.Warning);
            PortBox.Focus();
            return;
        }
        if (host.Length > 0 && FromBox.Text.Trim().Length == 0)
        {
            MessageBox.Show(this, "Pro odesílání přes SMTP vyplňte adresu odesílatele.", Title, MessageBoxButton.OK, MessageBoxImage.Warning);
            FromBox.Focus();
            return;
        }
        _settings.SmtpHost = host;
        _settings.SmtpPort = port;
        _settings.SmtpSsl = SslBox.IsChecked == true;
        _settings.SmtpUser = UserBox.Text.Trim();
        _settings.SmtpPassword = PasswordBox.Password;
        _settings.From = FromBox.Text.Trim();
        try
        {
            _settings.Save();
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, "Nastavení se nepodařilo uložit: " + ex.Message, Title, MessageBoxButton.OK, MessageBoxImage.Error);
            return;
        }
        DialogResult = true;
    }
}
