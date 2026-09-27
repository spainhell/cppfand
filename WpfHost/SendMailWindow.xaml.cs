using System.IO;
using System.Windows;

namespace WpfHost;

/// <summary>Odeslání sestavy přes SMTP (když je v nastavení vyplněný server).</summary>
public partial class SendMailWindow : Window
{
    private readonly MailSettings _settings;
    private readonly string _attachment;

    public SendMailWindow(MailSettings settings, string attachment, string subject)
    {
        InitializeComponent();
        _settings = settings;
        _attachment = attachment;
        SubjectBox.Text = subject;
        AttachmentText.Text = Path.GetFileName(attachment);
        Loaded += (_, _) => ToBox.Focus();
    }

    private async void Send_Click(object sender, RoutedEventArgs e)
    {
        if (ToBox.Text.Trim().Length == 0)
        {
            MessageBox.Show(this, "Vyplňte adresáta.", Title, MessageBoxButton.OK, MessageBoxImage.Warning);
            ToBox.Focus();
            return;
        }
        SendButton.IsEnabled = false;
        StatusText.Text = "odesílám…";
        try
        {
            await MailSender.SendViaSmtpAsync(_settings, ToBox.Text, SubjectBox.Text, BodyBox.Text, _attachment);
            DialogResult = true;
        }
        catch (Exception ex)
        {
            StatusText.Text = "";
            SendButton.IsEnabled = true;
            string detail = ex.InnerException != null ? ex.Message + "\n" + ex.InnerException.Message : ex.Message;
            MessageBox.Show(this, "E-mail se nepodařilo odeslat:\n" + detail, Title, MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }
}
