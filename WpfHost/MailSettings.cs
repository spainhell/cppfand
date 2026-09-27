using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace WpfHost;

/// <summary>
/// Nastavení odesílání e-mailu přes SMTP, pro každého uživatele Windows zvlášť
/// v %AppData%\cppfand\mail.txt. Když je vyplněný server, sestavy se posílají
/// přes něj; jinak přes výchozí poštovní klient (Simple MAPI).
/// Heslo je zašifrované DPAPI, takže ho přečte jen stejný uživatel na stejném počítači.
/// </summary>
public sealed class MailSettings
{
    public string SmtpHost { get; set; } = "";
    public int SmtpPort { get; set; } = 587;
    /// <summary>STARTTLS (port 587). SmtpClient neumí implicitní TLS na portu 465.</summary>
    public bool SmtpSsl { get; set; } = true;
    public string SmtpUser { get; set; } = "";
    public string SmtpPassword { get; set; } = "";
    public string From { get; set; } = "";

    public bool UseSmtp => SmtpHost.Trim().Length > 0;

    private static string FilePath =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "cppfand", "mail.txt");

    public static MailSettings Load()
    {
        var s = new MailSettings();
        try
        {
            if (!File.Exists(FilePath)) return s;
            foreach (string line in File.ReadAllLines(FilePath, Encoding.UTF8))
            {
                int eq = line.IndexOf('=');
                if (eq <= 0) continue;
                string key = line.Substring(0, eq).Trim(), value = line.Substring(eq + 1).Trim();
                switch (key)
                {
                    case "SmtpHost": s.SmtpHost = value; break;
                    case "SmtpPort": if (int.TryParse(value, out int port) && port > 0) s.SmtpPort = port; break;
                    case "SmtpSsl": s.SmtpSsl = value != "0" && !value.Equals("false", StringComparison.OrdinalIgnoreCase); break;
                    case "SmtpUser": s.SmtpUser = value; break;
                    case "SmtpPassword": s.SmtpPassword = Unprotect(value); break;
                    case "From": s.From = value; break;
                }
            }
        }
        catch (Exception)
        {
            // poškozený soubor: výchozí nastavení (výchozí poštovní klient)
        }
        return s;
    }

    public void Save()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
        var lines = new[]
        {
            "SmtpHost=" + SmtpHost.Trim(),
            "SmtpPort=" + SmtpPort,
            "SmtpSsl=" + (SmtpSsl ? "1" : "0"),
            "SmtpUser=" + SmtpUser.Trim(),
            "SmtpPassword=" + Protect(SmtpPassword),
            "From=" + From.Trim(),
        };
        File.WriteAllLines(FilePath, lines, new UTF8Encoding(false));
    }

    // --- DPAPI (CryptProtectData), bez balíčku System.Security.Cryptography.ProtectedData ---

    [StructLayout(LayoutKind.Sequential)]
    private struct DataBlob
    {
        public int Size;
        public IntPtr Data;
    }

    [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool CryptProtectData(ref DataBlob input, string? description, IntPtr entropy,
        IntPtr reserved, IntPtr prompt, int flags, out DataBlob output);

    [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool CryptUnprotectData(ref DataBlob input, IntPtr description, IntPtr entropy,
        IntPtr reserved, IntPtr prompt, int flags, out DataBlob output);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr mem);

    private const int CryptProtectUiForbidden = 0x1;

    private static string Protect(string plain)
    {
        if (plain.Length == 0) return "";
        byte[] result = Transform(Encoding.UTF8.GetBytes(plain), protect: true);
        return result.Length == 0 ? "" : Convert.ToBase64String(result);
    }

    private static string Unprotect(string stored)
    {
        if (stored.Length == 0) return "";
        try { return Encoding.UTF8.GetString(Transform(Convert.FromBase64String(stored), protect: false)); }
        catch (FormatException) { return ""; }
    }

    private static byte[] Transform(byte[] data, bool protect)
    {
        var handle = GCHandle.Alloc(data, GCHandleType.Pinned);
        try
        {
            var input = new DataBlob { Size = data.Length, Data = handle.AddrOfPinnedObject() };
            DataBlob output;
            bool ok = protect
                ? CryptProtectData(ref input, "cppfand SMTP", IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, CryptProtectUiForbidden, out output)
                : CryptUnprotectData(ref input, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, CryptProtectUiForbidden, out output);
            if (!ok) return Array.Empty<byte>();
            try
            {
                var result = new byte[output.Size];
                Marshal.Copy(output.Data, result, 0, output.Size);
                return result;
            }
            finally { LocalFree(output.Data); }
        }
        finally { handle.Free(); }
    }
}
