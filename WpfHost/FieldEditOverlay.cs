using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace WpfHost;

/// <summary>
/// Editace jednořádkového pole FANDu v běžném WPF TextBoxu položeném přes mřížku.
/// Interpret (DataEditor::EditTxt) čeká, dokud nevrátíme text, pozici kurzoru
/// a klávesu, kterou uživatel editaci ukončil. Klávesy, které TextBox zvládá sám
/// (psaní, šipky vlevo/vpravo, Home/End, Delete, Backspace, schránka), se
/// nepředávají; ostatní (Enter, Esc, Tab, šipky nahoru/dolů, PgUp/PgDn, F-klávesy,
/// Ctrl/Alt kombinace) editaci ukončí a FAND je zpracuje jako v konzoli.
/// </summary>
internal sealed class FieldEditOverlay
{
    private readonly Canvas _canvas;
    private readonly TerminalControl _terminal;
    private TextBox? _box;
    private Native.FieldEditRequest _request;
    private bool _insertMode = true;
    private DispatcherTimer? _timeout;
    private bool _completed;

    public FieldEditOverlay(Canvas canvas, TerminalControl terminal)
    {
        _canvas = canvas;
        _terminal = terminal;
    }

    public bool IsEditing => _box != null;

    /// <summary>Krátké hlášení o stavu editace (do stavového řádku).</summary>
    public Action<string>? Status { get; set; }

    private Native.FieldType Type => (Native.FieldType)_request.FieldType;
    private string Mask => Cp852.Decode(_request.Mask).Trim();

    public void Begin(Native.FieldEditRequest request)
    {
        End();
        _request = request;
        _insertMode = request.InsertMode != 0;
        _completed = false;

        byte attr = request.Attr;
        var box = new TextBox
        {
            Text = Cp852.Decode(request.Text),
            MaxLength = request.MaxLen,
            FontFamily = _terminal.FontFamilyUsed,
            FontSize = _terminal.FontSizePx,
            Padding = new Thickness(0),
            Margin = new Thickness(0),
            BorderThickness = new Thickness(0),
            Background = new SolidColorBrush(TerminalControl.PaletteColor(attr >> 4)),
            Foreground = new SolidColorBrush(TerminalControl.PaletteColor(attr & 0x0F)),
            CaretBrush = new SolidColorBrush(TerminalControl.PaletteColor(attr & 0x0F)),
            Width = request.Width * _terminal.CellWidth,
            Height = _terminal.CellHeight,
            VerticalContentAlignment = VerticalAlignment.Center,
            AcceptsReturn = false,
            AcceptsTab = false,
            TextWrapping = TextWrapping.NoWrap,
            SnapsToDevicePixels = true,
        };
        TextOptions.SetTextFormattingMode(box, TextFormattingMode.Display);
        if (request.Star != 0)
        {
            // heslo: text zobrazíme hvězdičkami přes FontFamily nelze, použijeme skrytí barvou
            box.Foreground = box.Background;
        }

        Canvas.SetLeft(box, request.X * _terminal.CellWidth);
        Canvas.SetTop(box, request.Y * _terminal.CellHeight);

        box.PreviewKeyDown += OnPreviewKeyDown;
        box.PreviewTextInput += OnPreviewTextInput;
        DataObject.AddPastingHandler(box, OnPasting);
        box.LostKeyboardFocus += (_, _) => { /* FAND vyžaduje výsledek, fokus vracíme */ if (_box != null && !_completed) Dispatcher.CurrentDispatcher.BeginInvoke(new Action(() => _box?.Focus())); };

        _box = box;
        int caret = Math.Max(0, Math.Min(box.Text.Length, request.Pos - 1));
        if (request.DelOnFirstKey != 0) box.SelectAll();
        else box.CaretIndex = caret;
        // fokus až po zařazení do vizuálního stromu, dřív by Focus() selhal
        box.Loaded += (_, _) => Keyboard.Focus(box);
        _canvas.Children.Add(box);
        Status?.Invoke($"pole [{request.X},{request.Y}] {Type} šířka {request.Width} max {request.MaxLen} maska '{Mask}' text '{box.Text.TrimEnd()}'");

        if (request.TimeoutMs > 0)
        {
            _timeout = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(request.TimeoutMs) };
            _timeout.Tick += (_, _) => Complete(KeyCodes.Esc);
            _timeout.Start();
        }
    }

    public void End()
    {
        _timeout?.Stop();
        _timeout = null;
        if (_box != null)
        {
            _canvas.Children.Remove(_box);
            _box = null;
        }
    }

    /// <summary>Předá výsledek interpretu a odstraní editor. Volat i při zavírání okna.</summary>
    public void Complete(ushort key)
    {
        if (_box == null || _completed) return;
        _completed = true;
        string text = _box.Text;
        if (text.Length > _request.MaxLen) text = text.Substring(0, _request.MaxLen);
        int pos = Math.Min(_box.CaretIndex, text.Length) + 1;
        End();
        Native.FandCompleteFieldEdit(Cp852.EncodeZ(text), pos, _insertMode ? 1 : 0, key);
        Status?.Invoke($"konec editace klávesou 0x{key:X4}, text '{text.TrimEnd()}'");
        _terminal.Focus();
    }

    // --- klávesy -----------------------------------------------------------------

    /// <summary>Text, který přišel do terminálu dřív, než editor dostal fokus.</summary>
    public void InjectText(string text) => InsertLimited(Filter(text));

    /// <summary>
    /// Vložení ze schránky, které začalo mimo editaci: první znak už FAND zpracoval
    /// (pole tím vymazal a znak vložil), teď nahradíme obsah celým vkládaným textem.
    /// </summary>
    public void ApplyPaste(string fullText)
    {
        if (_box == null) return;
        string text = Type == Native.FieldType.Date ? NormalizeDate(fullText) : Filter(fullText);
        if (text.Length > _request.MaxLen) text = text.Substring(0, _request.MaxLen);
        _box.Text = text;
        _box.CaretIndex = text.Length;
        Status?.Invoke($"vloženo '{text}'");
    }

    /// <summary>Vloží text místo výběru a hlídá maximální délku pole (programové vložení MaxLength nehlídá).</summary>
    private void InsertLimited(string text)
    {
        if (_box == null || text.Length == 0) return;
        int room = _request.MaxLen - (_box.Text.Length - _box.SelectionLength);
        if (room <= 0) return;
        if (text.Length > room) text = text.Substring(0, room);
        int i = _box.SelectionStart;
        _box.SelectedText = text;
        _box.SelectionLength = 0;
        _box.CaretIndex = Math.Min(i + text.Length, _box.Text.Length);
    }

    /// <summary>Klávesa, která přišla do terminálu dřív, než editor dostal fokus.</summary>
    public void InjectKey(Key key, ModifierKeys mods)
    {
        if (_box == null) return;
        bool ctrl = (mods & ModifierKeys.Control) != 0;
        bool alt = (mods & ModifierKeys.Alt) != 0;
        if (!ctrl && !alt)
        {
            switch (key)
            {
                case Key.Back:
                    if (_box.SelectionLength > 0) _box.SelectedText = "";
                    else if (_box.CaretIndex > 0) { int i = _box.CaretIndex; _box.Text = _box.Text.Remove(i - 1, 1); _box.CaretIndex = i - 1; }
                    return;
                case Key.Delete:
                    if (_box.SelectionLength > 0) _box.SelectedText = "";
                    else if (_box.CaretIndex < _box.Text.Length) { int i = _box.CaretIndex; _box.Text = _box.Text.Remove(i, 1); _box.CaretIndex = i; }
                    return;
                case Key.Left: if (_box.CaretIndex > 0) _box.CaretIndex--; return;
                case Key.Right: if (_box.CaretIndex < _box.Text.Length) _box.CaretIndex++; return;
                case Key.Home: _box.CaretIndex = 0; return;
                case Key.End: _box.CaretIndex = _box.Text.Length; return;
                case Key.Insert: _insertMode = !_insertMode; return;
            }
        }
        else if (ctrl && !alt)
        {
            switch (key)
            {
                case Key.A: _box.SelectAll(); return;
                case Key.C: CopyToClipboard(); return;
                case Key.X: if (_box.SelectionLength > 0) { try { Clipboard.SetText(_box.SelectedText); } catch { } _box.SelectedText = ""; } return;
                case Key.V: PasteClipboard(); return;
            }
        }
        HandleKey(key, mods);
    }

    /// <summary>Ctrl+C: výběr, nebo bez výběru celý obsah pole.</summary>
    private void CopyToClipboard()
    {
        if (_box == null) return;
        string text = _box.SelectionLength > 0 ? _box.SelectedText : _box.Text.Trim();
        try { Clipboard.SetText(text); Status?.Invoke($"zkopírováno '{text}'"); } catch { }
    }

    private void PasteClipboard()
    {
        string text;
        try { text = Clipboard.GetText(); } catch { return; }
        int nl = text.IndexOfAny(new[] { '\r', '\n' });
        if (nl >= 0) text = text.Substring(0, nl);
        InsertLimited(Type == Native.FieldType.Date ? NormalizeDate(text) : Filter(text));
    }

    private void OnPreviewKeyDown(object sender, KeyEventArgs e)
    {
        Key key = e.Key == Key.System ? e.SystemKey : e.Key;
        if (key == Key.None || KeyCodes.IsModifierKey(key)) return;
        e.Handled = HandleKey(key, Keyboard.Modifiers);
    }

    /// <summary>Vrací true, když klávesu zpracoval (přepnutí vkládání nebo ukončení editace).</summary>
    private bool HandleKey(Key key, ModifierKeys mods)
    {
        bool ctrl = (mods & ModifierKeys.Control) != 0;
        bool alt = (mods & ModifierKeys.Alt) != 0;
        bool shift = (mods & ModifierKeys.Shift) != 0;

        bool terminating = key is Key.Enter or Key.Escape or Key.Tab or Key.Up or Key.Down or Key.PageUp or Key.PageDown
            or Key.F1 or Key.F2 or Key.F3 or Key.F4 or Key.F5 or Key.F6 or Key.F7 or Key.F8 or Key.F9 or Key.F10 or Key.F11 or Key.F12;

        if (!ctrl && !alt)
        {
            if (key == Key.Insert && !shift)
            {
                _insertMode = !_insertMode;
                return true;
            }
            // psaní, šipky vlevo/vpravo, Home/End, Delete, Backspace, Shift+Insert zvládá TextBox sám
            if (!terminating) return false;
        }
        else if (ctrl && !alt)
        {
            switch (key)
            {
                case Key.C when _box!.SelectionLength == 0:
                    CopyToClipboard(); // bez výběru se kopíruje celé pole
                    return true;
                case Key.C or Key.V or Key.X or Key.A or Key.Z or Key.Y or Key.Left or Key.Right or Key.Home or Key.End or Key.Back or Key.Delete or Key.Insert:
                    return false; // schránka, výběr, undo, pohyb po slovech
            }
            // ostatní Ctrl kombinace dostane FAND
        }
        else if (alt && ctrl)
        {
            return false; // AltGr znaky
        }
        // Alt kombinace dostane FAND

        // vše ostatní editaci ukončí a klávesu dostane FAND
        ushort code = KeyCodes.Encode(key, mods);
        if (ctrl && !alt && key >= Key.A && key <= Key.Z)
        {
            // Ctrl+písmeno v kódování KeyCombination: 0x8200 | písmeno
            code = (ushort)(KeyCodes.Virtual | KeyCodes.Ctrl | (KeyInterop.VirtualKeyFromKey(key) & 0xFF));
        }
        Complete(code);
        return true;
    }

    private void OnPreviewTextInput(object sender, TextCompositionEventArgs e)
    {
        if (_box == null) return;
        string filtered = Filter(e.Text);
        if (filtered.Length == 0) { e.Handled = true; return; }

        if (!_insertMode && _box.SelectionLength == 0 && _box.CaretIndex < _box.Text.Length)
        {
            // přepis: nahradíme znak pod kurzorem
            int i = _box.CaretIndex;
            _box.Text = _box.Text.Remove(i, 1).Insert(i, filtered);
            _box.CaretIndex = i + filtered.Length;
            e.Handled = true;
            return;
        }
        if (filtered != e.Text)
        {
            int i = _box.SelectionStart;
            _box.SelectedText = filtered;
            _box.SelectionLength = 0;
            _box.CaretIndex = i + filtered.Length;
            e.Handled = true;
        }
    }

    private void OnPasting(object sender, DataObjectPastingEventArgs e)
    {
        if (_box == null || !e.DataObject.GetDataPresent(DataFormats.UnicodeText)) { e.CancelCommand(); return; }
        string text = (string)e.DataObject.GetData(DataFormats.UnicodeText);
        int nl = text.IndexOfAny(new[] { '\r', '\n' });
        if (nl >= 0) text = text.Substring(0, nl);
        text = Type == Native.FieldType.Date ? NormalizeDate(text) : Filter(text);
        e.CancelCommand();
        InsertLimited(text);
    }

    private static bool IsDigit(char c) => c >= '0' && c <= '9';

    /// <summary>Filtr znaků podle typu pole (stejně jako DataEditor::EditTxt).</summary>
    private string Filter(string text)
    {
        Func<char, bool> ok = Type switch
        {
            Native.FieldType.Numeric => IsDigit,
            Native.FieldType.Fixed => c => IsDigit(c) || c is '.' or ',' or '-',
            Native.FieldType.Real => c => IsDigit(c) || c is '.' or ',' or '-' or '+' or 'e' or 'E',
            _ => c => c >= ' ',
        };
        return new string(text.Where(ok).ToArray());
    }

    /// <summary>
    /// Datum ze schránky převede na tvar masky (DD.MM.YY / DD.MM.YYYY):
    /// přijme 20012024, 20.1.2024, 2024-01-20, 20/01/24 ...
    /// </summary>
    private string NormalizeDate(string text)
    {
        string mask = Mask.Length > 0 ? Mask.ToUpperInvariant() : "DD.MM.YY";
        text = text.Trim();
        int d = 0, m = 0, y = -1;
        var parts = Regex.Split(text, @"[.\-/ ]+").Where(p => p.Length > 0).ToArray();
        if (parts.Length == 3 && parts.All(p => p.All(IsDigit)))
        {
            if (parts[0].Length == 4) { y = int.Parse(parts[0]); m = int.Parse(parts[1]); d = int.Parse(parts[2]); }
            else { d = int.Parse(parts[0]); m = int.Parse(parts[1]); y = int.Parse(parts[2]); }
        }
        else if (parts.Length == 1 && text.All(IsDigit) && (text.Length == 6 || text.Length == 8))
        {
            d = int.Parse(text.Substring(0, 2)); m = int.Parse(text.Substring(2, 2)); y = int.Parse(text.Substring(4));
        }
        else
        {
            return Filter(text);
        }
        if (d is < 1 or > 31 || m is < 1 or > 12 || y < 0) return Filter(text);
        bool longYear = mask.Contains("YYYY");
        if (longYear && y < 100) y += y < 70 ? 2000 : 1900;
        if (!longYear && y >= 100) y %= 100;
        char sep = mask.Where(c => !char.IsLetter(c)).DefaultIfEmpty('.').First();
        string dd = mask.Contains("DD") ? d.ToString("00") : d.ToString();
        string mm = mask.Contains("MM") ? m.ToString("00") : m.ToString();
        string yy = longYear ? y.ToString("0000") : y.ToString("00");
        return mask.StartsWith("Y") ? $"{yy}{sep}{mm}{sep}{dd}" : $"{dd}{sep}{mm}{sep}{yy}";
    }
}
