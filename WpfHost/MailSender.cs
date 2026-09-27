using System.IO;
using System.Net;
using System.Net.Mail;
using System.Runtime.InteropServices;

namespace WpfHost;

/// <summary>Odeslání souboru e-mailem: výchozí poštovní klient (Simple MAPI) nebo SMTP.</summary>
public static class MailSender
{
    // --- výchozí poštovní klient (MAPISendMailW, Windows 8+) -------------------

    public enum MapiResult { Sent, Cancelled, Failed }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct MapiMessageW
    {
        public uint Reserved;
        public string? Subject;
        public string? NoteText;
        public string? MessageType;
        public string? DateReceived;
        public string? ConversationId;
        public uint Flags;
        public IntPtr Originator;
        public uint RecipCount;
        public IntPtr Recips;
        public uint FileCount;
        public IntPtr Files;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct MapiFileDescW
    {
        public uint Reserved;
        public uint Flags;
        public uint Position;
        public string PathName;
        public string FileName;
        public IntPtr FileType;
    }

    [DllImport("mapi32.dll", CharSet = CharSet.Unicode)]
    private static extern uint MAPISendMailW(IntPtr session, IntPtr uiParam, ref MapiMessageW message, uint flags, uint reserved);

    private const uint MapiLogonUi = 0x1;
    private const uint MapiDialog = 0x8;
    private const uint MapiUserAbort = 1;

    /// <summary>
    /// Otevře nový e-mail ve výchozím klientovi s přílohou. Blokuje, dokud ho uživatel
    /// neodešle nebo nezavře; volá se proto z vlastního vlákna (viz SendViaClientAsync).
    /// </summary>
    private static (MapiResult Result, uint Code) SendViaClient(string attachment, string subject, string body)
    {
        var file = new MapiFileDescW
        {
            Position = 0xFFFFFFFF,
            PathName = attachment,
            FileName = Path.GetFileName(attachment),
        };
        IntPtr files = Marshal.AllocHGlobal(Marshal.SizeOf<MapiFileDescW>());
        try
        {
            Marshal.StructureToPtr(file, files, false);
            var message = new MapiMessageW { Subject = subject, NoteText = body, FileCount = 1, Files = files };
            uint code = MAPISendMailW(IntPtr.Zero, IntPtr.Zero, ref message, MapiLogonUi | MapiDialog, 0);
            return code switch
            {
                0 => (MapiResult.Sent, 0),
                MapiUserAbort => (MapiResult.Cancelled, code),
                _ => (MapiResult.Failed, code),
            };
        }
        catch (Exception ex) when (ex is DllNotFoundException or EntryPointNotFoundException)
        {
            return (MapiResult.Failed, 0xFFFFFFFF);
        }
        finally
        {
            Marshal.DestroyStructure<MapiFileDescW>(files);
            Marshal.FreeHGlobal(files);
        }
    }

    /// <summary>MAPI chce jednovláknový apartment a okno klienta je modální, proto vlastní STA vlákno.</summary>
    public static Task<(MapiResult Result, uint Code)> SendViaClientAsync(string attachment, string subject, string body)
    {
        var tcs = new TaskCompletionSource<(MapiResult, uint)>();
        var thread = new Thread(() =>
        {
            try { tcs.SetResult(SendViaClient(attachment, subject, body)); }
            catch (Exception ex) { tcs.SetException(ex); }
        });
        thread.SetApartmentState(ApartmentState.STA);
        thread.IsBackground = true;
        thread.Start();
        return tcs.Task;
    }

    // --- SMTP ---------------------------------------------------------------

    public static async Task SendViaSmtpAsync(MailSettings settings, string to, string subject, string body, string attachment)
    {
        using var message = new MailMessage { From = new MailAddress(settings.From.Trim()), Subject = subject, Body = body };
        foreach (string address in to.Split(new[] { ',', ';' }, StringSplitOptions.RemoveEmptyEntries))
            message.To.Add(address.Trim());
        // .NET Framework by jinak poslal application/octet-stream
        string mediaType = attachment.EndsWith(".pdf", StringComparison.OrdinalIgnoreCase) ? "application/pdf" : "application/octet-stream";
        var file = new Attachment(attachment, mediaType) { Name = Path.GetFileName(attachment) };
        file.ContentDisposition!.FileName = file.Name;
        message.Attachments.Add(file);

        using var client = new SmtpClient(settings.SmtpHost.Trim(), settings.SmtpPort)
        {
            EnableSsl = settings.SmtpSsl,
            DeliveryMethod = SmtpDeliveryMethod.Network,
        };
        if (settings.SmtpUser.Trim().Length > 0)
            client.Credentials = new NetworkCredential(settings.SmtpUser.Trim(), settings.SmtpPassword);
        await client.SendMailAsync(message);
    }
}
