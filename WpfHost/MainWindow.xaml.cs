using System.ComponentModel;
using System.Windows;
using System.Windows.Threading;

namespace WpfHost;

public partial class MainWindow : Window
{
    private readonly DispatcherTimer _poll;
    private readonly FieldEditOverlay _fieldEdit;
    private ushort[] _cells = new ushort[80 * 25];
    private ulong _version;
    private bool _started;
    private bool _endedReported;

    public MainWindow(StartOptions options)
    {
        InitializeComponent();
        _fieldEdit = new FieldEditOverlay(Overlay, Terminal);
        Terminal.StatusChanged += s => StatusText.Text = s;
        _fieldEdit.Status = s => StatusText.Text = s;
        Terminal.RedirectKey = (key, mods) =>
        {
            if (!TryBeginPendingEdit()) return false;
            _fieldEdit.InjectKey(key, mods);
            return true;
        };
        Terminal.RedirectText = text =>
        {
            if (!TryBeginPendingEdit()) return false;
            _fieldEdit.InjectText(text);
            return true;
        };
        Terminal.MouseAllowed = () => !_fieldEdit.IsEditing;
        Terminal.PendingPaste = text => { _pendingPaste = text; _pendingPasteAt = DateTime.UtcNow; };
        Title = $"C++ FAND – {options.RdbName}";

        _poll = new DispatcherTimer(DispatcherPriority.Render) { Interval = TimeSpan.FromMilliseconds(33) };
        _poll.Tick += (_, _) => Poll();

        Loaded += (_, _) =>
        {
            Native.FandSetScreenSize(HostSettings.ScreenCols, HostSettings.ScreenRows);
            int rc = Native.FandStart(options.FandDir, options.WorkDir, options.RdbName, options.Mode);
            if (rc != 0)
            {
                StatusText.Text = $"FandStart selhal ({rc})";
                return;
            }
            _started = true;
            StatusText.Text = $"{options.RdbName} · {options.WorkDir}";
            Terminal.Focus();
            _poll.Start();
        };
    }

    private void Poll()
    {
        ulong v = Native.FandGetScreen(_cells, _cells.Length, out var info);
        if (info.Cols * info.Rows > _cells.Length)
        {
            _cells = new ushort[info.Cols * info.Rows];
            v = Native.FandGetScreen(_cells, _cells.Length, out info);
        }
        if (v != _version)
        {
            _version = v;
            Terminal.Update(_cells, info);
        }

        TryBeginPendingEdit();
        TryBeginPendingTextEdit();

        if (_started && info.Running == 0 && !_endedReported)
        {
            _endedReported = true;
            _poll.Stop();
            _fieldEdit.End();
            string err = Native.LastError();
            int code = Native.FandExitCode();
            StatusText.Text = err.Length > 0 ? $"interpret skončil: {err}" : $"interpret skončil (kód {code})";
            // řádné ukončení programu zavře i okno; při chybě zůstane, aby bylo vidět hlášení
            if (err.Length == 0 && code == 0) Close();
        }
    }

    private bool _textEditOpen;

    /// <summary>
    /// Interpret čeká v RunTextEdit (Drivers/host.h) na editaci celého textu.
    /// Text se nevejde do struktury, takže se vyzvedává zvlášť podle TextLength.
    /// </summary>
    private void TryBeginPendingTextEdit()
    {
        if (_textEditOpen || !_started) return;
        if (Native.FandPollTextEdit(out var info) == 0) return;

        _textEditOpen = true;
        try
        {
            var bytes = new byte[Math.Max(info.TextLength, 1)];
            int len = Native.FandGetTextEditText(bytes, info.TextLength);
            string text = FandAttr.Decode(bytes, len);

            var dlg = new TextEditWindow(info, text) { Owner = this };
            dlg.ShowDialog();

            byte[] result = FandAttr.Encode(dlg.ResultText);
            byte[] word = FandAttr.Encode(dlg.ResultWord);
            Native.FandCompleteTextEdit(result, result.Length, dlg.ResultPos,
                info.Scroll, dlg.ResultUpdated ? 1 : 0, dlg.ResultKey, word, word.Length);
        }
        finally
        {
            _textEditOpen = false;
            Terminal.Focus();
        }
    }

    // vložení ze schránky zahájené mimo editaci: čeká, až FAND pole otevře
    private string? _pendingPaste;
    private DateTime _pendingPasteAt;

    /// <summary>Editor pole běží, nebo na něj interpret právě čeká a byl teď zobrazen.</summary>
    private bool TryBeginPendingEdit()
    {
        if (_fieldEdit.IsEditing) return true;
        if (!_started || Native.FandPollFieldEdit(out var req) == 0) return false;
        _fieldEdit.Begin(req);
        if (_pendingPaste != null)
        {
            if ((DateTime.UtcNow - _pendingPasteAt).TotalSeconds < 2) _fieldEdit.ApplyPaste(_pendingPaste);
            _pendingPaste = null;
        }
        return true;
    }

    protected override void OnClosing(CancelEventArgs e)
    {
        if (_started && Native.FandIsRunning() != 0)
        {
            // Křížek není platné ukončení: program si při příštím startu bude stěžovat na havárii.
            var answer = MessageBox.Show(this,
                "Program ještě běží. Ukončete ho jeho vlastní nabídkou (Esc až na hlavní menu a potvrdit ukončení).\n\n" +
                "Zavření okna odpovídá výpadku proudu a program to při příštím startu ohlásí jako havárii.\n\nZavřít okno přesto?",
                Title, MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No);
            if (answer != MessageBoxResult.Yes)
            {
                e.Cancel = true;
                Terminal.Focus();
                return;
            }
        }
        _poll.Stop();
        if (_fieldEdit.IsEditing) _fieldEdit.Complete(KeyCodes.Esc);
        if (Native.FandIsRunning() != 0)
        {
            Native.FandStop();
            // Esc do fronty, aby se interpret probral z čekání na klávesu
            Native.FandPushKey(0x1B, 0, 27, 0, 1);
            Native.FandWait(3000);
        }
        base.OnClosing(e);
    }
}
