using System.IO;
using System.Windows;
using Microsoft.Win32;

namespace WpfHost;

/// <summary>
/// Akce nad výstupem sestavy nebo textem: tisk, uložení do PDF/DOCX/ODT a odeslání
/// e-mailem. Volá je prohlížeč textu (TextEditWindow) i požadavek na tisk
/// z interpretu (ASSIGN=LPT1, printtxt).
/// </summary>
public static class ReportActions
{
    public enum Format { Pdf, Docx, Odt }

    /// <summary>Tisk přes dialog Windows; false = zrušeno.</summary>
    public static bool Print(Window? owner, string text, string name, int copies = 1)
    {
        try
        {
            return ReportPrinter.PrintWithDialog(owner, ReportDocument.Parse(text), DefaultName(name), copies);
        }
        catch (Exception ex)
        {
            ShowError(owner, "Tisk se nezdařil", ex);
            return false;
        }
    }

    /// <summary>Uloží do zvoleného formátu; vrací cestu, nebo null při zrušení/chybě.</summary>
    public static string? SaveAs(Window? owner, string text, string name, Format format)
    {
        var (ext, filter) = format switch
        {
            Format.Pdf => (".pdf", "PDF (*.pdf)|*.pdf"),
            Format.Docx => (".docx", "Dokument Word (*.docx)|*.docx"),
            _ => (".odt", "Dokument OpenDocument (*.odt)|*.odt"),
        };
        var dialog = new SaveFileDialog
        {
            FileName = DefaultName(name) + ext,
            DefaultExt = ext,
            Filter = filter,
            AddExtension = true,
            OverwritePrompt = true,
        };
        if (dialog.ShowDialog(owner) != true) return null;
        try
        {
            Export(text, name, dialog.FileName, format);
            return dialog.FileName;
        }
        catch (Exception ex)
        {
            ShowError(owner, "Soubor se nepodařilo uložit", ex);
            return null;
        }
    }

    public static void Export(string text, string name, string path, Format format)
    {
        var doc = ReportDocument.Parse(text);
        switch (format)
        {
            case Format.Pdf: ReportPdf.Save(doc, path, DefaultName(name)); break;
            case Format.Docx: ReportOffice.SaveDocx(doc, path); break;
            default: ReportOffice.SaveOdt(doc, path); break;
        }
    }

    /// <summary>
    /// Odešle jako PDF přílohu: přes SMTP, když je nastavený server, jinak ve výchozím
    /// poštovním klientovi. Vrací popis výsledku pro stavový řádek.
    /// </summary>
    public static async Task<string> EmailAsync(Window owner, string text, string name)
    {
        string baseName = DefaultName(name);
        string pdf;
        try
        {
            string dir = Path.Combine(Path.GetTempPath(), "cppfand");
            pdf = Path.Combine(dir, baseName + ".pdf");
            Export(text, name, pdf, Format.Pdf);
        }
        catch (Exception ex)
        {
            ShowError(owner, "PDF pro e-mail se nepodařilo vytvořit", ex);
            return "";
        }

        var settings = MailSettings.Load();
        if (settings.UseSmtp)
        {
            var window = new SendMailWindow(settings, pdf, baseName) { Owner = owner };
            return window.ShowDialog() == true ? "e-mail odeslán" : "";
        }

        var (result, code) = await MailSender.SendViaClientAsync(pdf, baseName, "");
        switch (result)
        {
            case MailSender.MapiResult.Sent: return "e-mail předán poštovnímu klientovi";
            case MailSender.MapiResult.Cancelled: return "";
        }
        var answer = MessageBox.Show(owner,
            $"Výchozí poštovní klient se nepodařilo otevřít (MAPI kód {code}).\n\n" +
            "Nový Outlook a webová pošta Simple MAPI nepodporují. Můžete nastavit odesílání přes SMTP server, " +
            "nebo sestavu uložit jako PDF a přiložit ji ručně.\n\nOtevřít nastavení e-mailu?",
            "Odeslat e-mailem", MessageBoxButton.YesNo, MessageBoxImage.Warning);
        if (answer == MessageBoxResult.Yes && EditMailSettings(owner) && MailSettings.Load().UseSmtp)
            return await EmailAsync(owner, text, name);
        return "";
    }

    public static bool EditMailSettings(Window? owner)
    {
        var window = new MailSettingsWindow(MailSettings.Load()) { Owner = owner };
        return window.ShowDialog() == true;
    }

    /// <summary>
    /// Název souboru a tiskové úlohy: výstup sestavy (PRINTER.TXT) dostane
    /// „Sestava“ a datum, jiný text název svého souboru.
    /// </summary>
    public static string DefaultName(string name)
    {
        string n = (name ?? "").Trim();
        int slash = n.LastIndexOfAny(new[] { '\\', '/', ':' });
        if (slash >= 0) n = n.Substring(slash + 1);
        if (n.Length == 0 || n.StartsWith("PRINTER", StringComparison.OrdinalIgnoreCase) || n == "FAND")
            n = "Sestava " + DateTime.Now.ToString("yyyy-MM-dd HH.mm");
        else
            n = Path.GetFileNameWithoutExtension(n);
        foreach (char c in Path.GetInvalidFileNameChars()) n = n.Replace(c, '_');
        return n.Length > 0 ? n : "Sestava";
    }

    private static void ShowError(Window? owner, string what, Exception ex)
    {
        string text = what + ":\n" + ex.Message;
        if (owner != null) MessageBox.Show(owner, text, "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Error);
        else MessageBox.Show(text, "C++ FAND", MessageBoxButton.OK, MessageBoxImage.Error);
    }
}
