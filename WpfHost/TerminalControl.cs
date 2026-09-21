using System.Globalization;
using System.Text;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace WpfHost;

/// <summary>
/// Mřížka znaků (80×25) kreslená z bufferu FANDu. Klávesy posílá interpretu
/// ve tvaru, jaký by dostal z konzole (KEY_EVENT_RECORD).
/// </summary>
public sealed class TerminalControl : FrameworkElement
{
    // 16 barev CGA/VGA (index = dolní nibble atributu)
    private static readonly Color[] Palette =
    {
        Color.FromRgb(0, 0, 0), Color.FromRgb(0, 0, 170), Color.FromRgb(0, 170, 0), Color.FromRgb(0, 170, 170),
        Color.FromRgb(170, 0, 0), Color.FromRgb(170, 0, 170), Color.FromRgb(170, 85, 0), Color.FromRgb(170, 170, 170),
        Color.FromRgb(85, 85, 85), Color.FromRgb(85, 85, 255), Color.FromRgb(85, 255, 85), Color.FromRgb(85, 255, 255),
        Color.FromRgb(255, 85, 85), Color.FromRgb(255, 85, 255), Color.FromRgb(255, 255, 85), Color.FromRgb(255, 255, 255),
    };
    private static readonly SolidColorBrush[] Brushes = Palette.Select(c => { var b = new SolidColorBrush(c); b.Freeze(); return b; }).ToArray();

    public static Color PaletteColor(int index) => Palette[index & 0x0F];

    private int _cols = 80;
    private int _rows = 25;
    private ushort[] _cells = new ushort[80 * 25];
    private int _cursorX, _cursorY, _cursorSize = 1;
    private int _fieldX = -1, _fieldY = -1, _fieldLen;
    private bool _cursorVisible;
    private bool _blinkOn = true;
    private readonly DispatcherTimer _blink;

    private double _fontSize = 18;
    private GlyphTypeface? _primary;
    private readonly List<GlyphTypeface> _fallbacks = new();
    private double _cellWidth = 10, _cellHeight = 20, _baseline = 15;
    private readonly Dictionary<char, (GlyphTypeface face, ushort glyph)> _glyphCache = new();

    private int _lastVirtualKey;

    public event Action<string>? StatusChanged;

    /// <summary>
    /// Před odesláním klávesy interpretu: hostitel může zjistit, že interpret mezitím
    /// požádal o editaci pole, editor zobrazit a klávesu poslat jemu (vrátí true).
    /// </summary>
    public Func<Key, ModifierKeys, bool>? RedirectKey { get; set; }
    public Func<string, bool>? RedirectText { get; set; }

    /// <summary>
    /// Vrátí false, když myš interpretu posílat nemáme – typicky když hostitel
    /// právě převzal editaci pole a kliknutí patří jeho editoru.
    /// </summary>
    public Func<bool>? MouseAllowed { get; set; }

    public TerminalControl()
    {
        Focusable = true;
        FocusVisualStyle = null;
        SnapsToDevicePixels = true;
        UseLayoutRounding = true;
        TextOptions.SetTextFormattingMode(this, TextFormattingMode.Display);
        TextOptions.SetTextRenderingMode(this, TextRenderingMode.ClearType);
        ResolveFonts();
        _blink = new DispatcherTimer(DispatcherPriority.Render) { Interval = TimeSpan.FromMilliseconds(450) };
        _blink.Tick += (_, _) => { _blinkOn = !_blinkOn; if (_cursorVisible) InvalidateVisual(); };
        _blink.Start();
    }

    public int Cols => _cols;
    public int Rows => _rows;
    public double CellWidth => _cellWidth;
    public double CellHeight => _cellHeight;
    public double FontSizePx => _fontSize;
    public FontFamily FontFamilyUsed { get; private set; } = new FontFamily("Consolas");

    public double FontSizeSetting
    {
        get => _fontSize;
        set
        {
            _fontSize = Math.Max(8, Math.Min(72, value));
            MeasureCell();
            InvalidateMeasure();
            InvalidateVisual();
        }
    }

    /// <summary>Znak a atribut buňky (0-based).</summary>
    public (char ch, byte attr) CellAt(int x, int y)
    {
        if (x < 0 || y < 0 || x >= _cols || y >= _rows) return (' ', 7);
        ushort c = _cells[y * _cols + x];
        return (Cp852.ToChar((byte)(c & 0xFF)), (byte)(c >> 8));
    }

    public void Update(ushort[] cells, Native.ScreenInfo info)
    {
        bool sizeChanged = info.Cols != _cols || info.Rows != _rows;
        _cols = info.Cols;
        _rows = info.Rows;
        if (_cells.Length != _cols * _rows) _cells = new ushort[_cols * _rows];
        Array.Copy(cells, _cells, Math.Min(cells.Length, _cells.Length));
        _cursorX = info.CursorX;
        _cursorY = info.CursorY;
        _cursorVisible = info.CursorVisible != 0;
        _cursorSize = info.CursorSize;
        _fieldX = info.FieldX;
        _fieldY = info.FieldY;
        _fieldLen = info.FieldLen;
        _blinkOn = true;
        if (sizeChanged) InvalidateMeasure();
        InvalidateVisual();
    }

    /// <summary>Text celé obrazovky (pro kopírování do schránky).</summary>
    public string ScreenText()
    {
        var sb = new StringBuilder();
        for (int y = 0; y < _rows; y++)
        {
            for (int x = 0; x < _cols; x++) sb.Append(Cp852.ToChar((byte)(_cells[y * _cols + x] & 0xFF)));
            sb.AppendLine();
        }
        return sb.ToString();
    }

    // --- písmo ---------------------------------------------------------------

    private void ResolveFonts()
    {
        _primary = null;
        _fallbacks.Clear();
        foreach (var name in new[] { "Cascadia Mono", "Consolas", "Lucida Console", "Courier New" })
        {
            var tf = new Typeface(new FontFamily(name), FontStyles.Normal, FontWeights.Normal, FontStretches.Normal);
            if (tf.TryGetGlyphTypeface(out var gtf) && gtf.FamilyNames.Values.Any(v => v.StartsWith(name, StringComparison.OrdinalIgnoreCase)))
            {
                if (_primary == null) { _primary = gtf; FontFamilyUsed = new FontFamily(name); }
                else _fallbacks.Add(gtf);
            }
        }
        foreach (var name in new[] { "Segoe UI Symbol", "Segoe UI" })
        {
            var tf = new Typeface(new FontFamily(name), FontStyles.Normal, FontWeights.Normal, FontStretches.Normal);
            if (tf.TryGetGlyphTypeface(out var gtf)) _fallbacks.Add(gtf);
        }
        if (_primary == null)
        {
            var tf = new Typeface("Consolas");
            tf.TryGetGlyphTypeface(out _primary);
        }
        _glyphCache.Clear();
        MeasureCell();
    }

    private void MeasureCell()
    {
        if (_primary == null) return;
        // šířka buňky = šířka glyfu 'W' v monospace písmu, výška z metrik písma
        ushort gi = _primary.CharacterToGlyphMap.TryGetValue('W', out var g) ? g : (ushort)0;
        _cellWidth = Math.Round(_primary.AdvanceWidths[gi] * _fontSize);
        _cellHeight = Math.Round(_primary.Height * _fontSize);
        _baseline = Math.Round(_primary.Baseline * _fontSize);
        _glyphCache.Clear();
    }

    private (GlyphTypeface face, ushort glyph) Glyph(char c)
    {
        if (_glyphCache.TryGetValue(c, out var hit)) return hit;
        (GlyphTypeface, ushort) result;
        if (_primary!.CharacterToGlyphMap.TryGetValue(c, out var g)) result = (_primary, g);
        else
        {
            result = (_primary, _primary.CharacterToGlyphMap.TryGetValue(' ', out var sp) ? sp : (ushort)0);
            foreach (var f in _fallbacks)
            {
                if (f.CharacterToGlyphMap.TryGetValue(c, out var fg)) { result = (f, fg); break; }
            }
        }
        _glyphCache[c] = result;
        return result;
    }

    // --- layout a kreslení ----------------------------------------------------

    protected override Size MeasureOverride(Size availableSize)
        => new(_cols * _cellWidth, _rows * _cellHeight);

    protected override void OnRender(DrawingContext dc)
    {
        if (_primary == null) return;
        double pixelsPerDip = VisualTreeHelper.GetDpi(this).PixelsPerDip;

        // pozadí celé plochy černé (pro případ nesouladu rozměrů)
        dc.DrawRectangle(Brushes[0], null, new Rect(0, 0, _cols * _cellWidth, _rows * _cellHeight));

        var indices = new List<ushort>(_cols);
        var advances = new List<double>(_cols);
        var blocks = new List<(int x, char ch)>();
        ushort spaceGlyph = _primary.CharacterToGlyphMap.TryGetValue(' ', out var sg) ? sg : (ushort)0;

        for (int y = 0; y < _rows; y++)
        {
            int x = 0;
            while (x < _cols)
            {
                ushort first = _cells[y * _cols + x];
                byte attr = (byte)(first >> 8);
                char firstChr = Cp852.ToChar((byte)(first & 0xFF));
                var face = IsBlock(firstChr) ? _primary : Glyph(firstChr).face;
                int runStart = x;
                bool allSpaces = true;
                indices.Clear();
                advances.Clear();
                blocks.Clear();
                while (x < _cols)
                {
                    ushort c = _cells[y * _cols + x];
                    if ((byte)(c >> 8) != attr) break;
                    char chr = Cp852.ToChar((byte)(c & 0xFF));
                    if (IsBlock(chr))
                    {
                        // plné a stínované bloky kreslíme sami jako obdélníky (bez mezer mezi glyfy)
                        if (face != _primary) break;
                        blocks.Add((x, chr));
                        indices.Add(spaceGlyph);
                    }
                    else
                    {
                        var (f, gi) = Glyph(chr);
                        if (f != face) break;
                        if (chr != ' ') allSpaces = false;
                        indices.Add(gi);
                    }
                    advances.Add(_cellWidth);
                    x++;
                }
                int len = x - runStart;
                var bg = Brushes[(attr >> 4) & 0x0F];
                var fg = Brushes[attr & 0x0F];
                var rect = new Rect(runStart * _cellWidth, y * _cellHeight, len * _cellWidth, _cellHeight);
                dc.DrawRectangle(bg, null, rect);
                foreach (var (bx, bch) in blocks) DrawBlock(dc, bx, y, bch, attr & 0x0F);
                if (allSpaces) continue;
                var origin = new Point(runStart * _cellWidth, y * _cellHeight + _baseline);
                var run = new GlyphRun(face, 0, false, _fontSize, (float)pixelsPerDip,
                    indices.ToArray(), origin, advances.ToArray(), null, null, null, null, null, null);
                dc.DrawGlyphRun(fg, run);
            }
        }

        // kurzor: podtržítko (normální) nebo spodní polovina buňky (velký = přepis)
        if (_cursorVisible && _blinkOn && _cursorX >= 0 && _cursorX < _cols && _cursorY >= 0 && _cursorY < _rows)
        {
            byte attr = (byte)(_cells[_cursorY * _cols + _cursorX] >> 8);
            var fg = Brushes[attr & 0x0F];
            double h = _cursorSize >= 50 ? _cellHeight / 2 : Math.Max(2, Math.Round(_cellHeight / 8));
            dc.DrawRectangle(fg, null, new Rect(_cursorX * _cellWidth, (_cursorY + 1) * _cellHeight - h, _cellWidth, h));
        }
    }

    private static bool IsBlock(char c) => c is '█' or '▀' or '▄' or '▌' or '▐' or '░' or '▒' or '▓';

    private readonly Dictionary<int, SolidColorBrush> _shadeBrushes = new();

    private void DrawBlock(DrawingContext dc, int x, int y, char ch, int fgIndex)
    {
        double left = x * _cellWidth, top = y * _cellHeight, w = _cellWidth, h = _cellHeight;
        switch (ch)
        {
            case '█': dc.DrawRectangle(Brushes[fgIndex], null, new Rect(left, top, w, h)); break;
            case '▀': dc.DrawRectangle(Brushes[fgIndex], null, new Rect(left, top, w, Math.Round(h / 2))); break;
            case '▄': dc.DrawRectangle(Brushes[fgIndex], null, new Rect(left, top + Math.Round(h / 2), w, h - Math.Round(h / 2))); break;
            case '▌': dc.DrawRectangle(Brushes[fgIndex], null, new Rect(left, top, Math.Round(w / 2), h)); break;
            case '▐': dc.DrawRectangle(Brushes[fgIndex], null, new Rect(left + Math.Round(w / 2), top, w - Math.Round(w / 2), h)); break;
            default:
            {
                byte alpha = ch switch { '░' => 64, '▒' => 128, _ => 192 };
                int key = (fgIndex << 8) | alpha;
                if (!_shadeBrushes.TryGetValue(key, out var brush))
                {
                    var c = Palette[fgIndex];
                    brush = new SolidColorBrush(Color.FromArgb(alpha, c.R, c.G, c.B));
                    brush.Freeze();
                    _shadeBrushes[key] = brush;
                }
                dc.DrawRectangle(brush, null, new Rect(left, top, w, h));
                break;
            }
        }
    }

    // --- myš --------------------------------------------------------------------

    private int _mouseCellX = -1, _mouseCellY = -1;

    /// <summary>Pozice myši v buňkách mřížky (0-based, oříznutá na obrazovku).</summary>
    private (int X, int Y) CellAt(Point p)
    {
        int x = _cellWidth > 0 ? (int)(p.X / _cellWidth) : 0;
        int y = _cellHeight > 0 ? (int)(p.Y / _cellHeight) : 0;
        return (Math.Max(0, Math.Min(_cols - 1, x)), Math.Max(0, Math.Min(_rows - 1, y)));
    }

    private static uint ButtonState(MouseDevice m)
    {
        uint state = 0;
        if (m.LeftButton == MouseButtonState.Pressed) state |= Native.FromLeft1stButtonPressed;
        if (m.RightButton == MouseButtonState.Pressed) state |= Native.RightmostButtonPressed;
        return state;
    }

    /// <summary>Pošle interpretu stav myši ve tvaru MOUSE_EVENT_RECORD konzole.</summary>
    private void PushMouse(MouseEventArgs e, bool moved)
    {
        if (MouseAllowed?.Invoke() == false) return;
        var (x, y) = CellAt(e.GetPosition(this));
        if (moved && x == _mouseCellX && y == _mouseCellY) return; // pohyb v rámci jedné buňky FAND nezajímá
        _mouseCellX = x; _mouseCellY = y;
        uint mods = 0;
        if ((Keyboard.Modifiers & ModifierKeys.Shift) != 0) mods |= Native.ShiftPressed;
        if ((Keyboard.Modifiers & ModifierKeys.Control) != 0) mods |= Native.LeftCtrlPressed;
        if ((Keyboard.Modifiers & ModifierKeys.Alt) != 0) mods |= Native.LeftAltPressed;
        Native.FandPushMouse(x, y, ButtonState(e.MouseDevice), moved ? Native.MouseMoved : 0, mods);
    }

    protected override void OnMouseDown(MouseButtonEventArgs e)
    {
        Focus();
        if (e.ChangedButton == MouseButton.Left || e.ChangedButton == MouseButton.Right)
        {
            CaptureMouse();
            PushMouse(e, false);
            e.Handled = true;
        }
        base.OnMouseDown(e);
    }

    protected override void OnMouseUp(MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left || e.ChangedButton == MouseButton.Right)
        {
            PushMouse(e, false);
            if (e.MouseDevice.LeftButton != MouseButtonState.Pressed
                && e.MouseDevice.RightButton != MouseButtonState.Pressed)
            {
                ReleaseMouseCapture();
            }
            e.Handled = true;
        }
        base.OnMouseUp(e);
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        PushMouse(e, true);
        base.OnMouseMove(e);
    }

    // --- klávesnice -------------------------------------------------------------

    protected override void OnMouseWheel(MouseWheelEventArgs e)
    {
        if ((Keyboard.Modifiers & ModifierKeys.Control) != 0)
        {
            FontSizeSetting += e.Delta > 0 ? 1 : -1;
            StatusChanged?.Invoke($"písmo {_fontSize:0} px");
            e.Handled = true;
            return;
        }
        base.OnMouseWheel(e);
    }

    private static uint ControlState(ModifierKeys mods, Key key)
    {
        uint state = 0;
        if ((mods & ModifierKeys.Shift) != 0) state |= Native.ShiftPressed;
        if ((mods & ModifierKeys.Control) != 0) state |= Native.LeftCtrlPressed;
        if ((mods & ModifierKeys.Alt) != 0) state |= Native.LeftAltPressed;
        if (KeyCodes.IsEnhanced(key)) state |= Native.EnhancedKey;
        return state;
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
        Key key = e.Key == Key.System ? e.SystemKey : e.Key;
        if (key == Key.None || KeyCodes.IsModifierKey(key)) { base.OnKeyDown(e); return; }

        var mods = Keyboard.Modifiers;
        bool ctrl = (mods & ModifierKeys.Control) != 0;
        bool alt = (mods & ModifierKeys.Alt) != 0;
        int vk = KeyInterop.VirtualKeyFromKey(key);
        _lastVirtualKey = vk;

        // čeká interpret na editaci pole? pak klávesa patří editoru
        bool printable = KeyCodes.CharFor(key) == 0 && !KeyCodes.IsNavigationOrFunction(key) && !ctrl && !alt;
        if (!printable && RedirectKey?.Invoke(key, mods) == true)
        {
            e.Handled = true;
            return;
        }

        bool shift = (mods & ModifierKeys.Shift) != 0;
        // Shift+Insert nebo Ctrl+V: vložení schránky do pole pod kurzorem
        if ((key == Key.Insert && shift && !ctrl && !alt) || (key == Key.V && ctrl && !alt && !shift))
        {
            PasteFromClipboard();
            e.Handled = true;
            return;
        }
        // Ctrl+Shift+C: kopie celé obrazovky; Ctrl+C: kopie zvýrazněného pole pod kurzorem
        if (key == Key.C && ctrl && !alt)
        {
            try
            {
                if (shift) { Clipboard.SetText(ScreenText()); StatusChanged?.Invoke("obrazovka zkopírována"); }
                else
                {
                    string field = CurrentFieldText();
                    Clipboard.SetText(field);
                    StatusChanged?.Invoke(field.Length > 0 ? $"zkopírováno '{field}'" : "pod kurzorem není pole");
                }
            }
            catch { }
            e.Handled = true;
            return;
        }

        ushort ch = KeyCodes.CharFor(key);
        if (ch != 0 || KeyCodes.IsNavigationOrFunction(key))
        {
            Native.FandPushKey((ushort)vk, 0, ch, ControlState(mods, key), 1);
            e.Handled = true;
            return;
        }

        if (ctrl && alt)
        {
            // AltGr: znak dodá TextInput
            base.OnKeyDown(e);
            return;
        }
        if (ctrl)
        {
            // Ctrl+písmeno: konzole posílá řídicí znak 1..26
            ushort c = key >= Key.A && key <= Key.Z ? (ushort)(vk - 'A' + 1) : (ushort)0;
            Native.FandPushKey((ushort)vk, 0, c, ControlState(mods, key), 1);
            e.Handled = true;
            return;
        }
        if (alt)
        {
            Native.FandPushKey((ushort)vk, 0, 0, ControlState(mods, key), 1);
            e.Handled = true;
            return;
        }
        // tisknutelné znaky dodá OnTextInput
        base.OnKeyDown(e);
    }

    protected override void OnTextInput(TextCompositionEventArgs e)
    {
        if (string.IsNullOrEmpty(e.Text)) { base.OnTextInput(e); return; }
        if (RedirectText?.Invoke(e.Text) == true)
        {
            e.Handled = true;
            return;
        }
        foreach (char c in e.Text)
        {
            if (c == '\r' || c == '\n' || c == '\t' || c == '\b' || c == 27) continue; // ty chodí přes OnKeyDown
            PushChar(c);
        }
        e.Handled = true;
    }

    private void PushChar(char c)
    {
        // pro tisknutelný znak FAND používá jen znak a Shift; VK dodáme z posledního KeyDown
        uint state = char.IsUpper(c) ? Native.ShiftPressed : 0;
        Native.FandPushKey((ushort)_lastVirtualKey, 0, c, state, 1);
    }

    /// <summary>
    /// Vložení mimo editaci: první znak jde FANDu jako psaní (tím se editace pole spustí),
    /// zbytek si hostitel podrží a vloží do editoru, jakmile se objeví (viz PendingPaste).
    /// </summary>
    public Action<string>? PendingPaste { get; set; }

    private void PasteFromClipboard()
    {
        string text;
        try { text = Clipboard.GetText(); } catch { return; }
        if (string.IsNullOrEmpty(text)) return;
        // jen první řádek, bez řídicích znaků
        int nl = text.IndexOfAny(new[] { '\r', '\n' });
        if (nl >= 0) text = text.Substring(0, nl);
        text = new string(text.Where(c => c >= 32).ToArray());
        if (text.Length == 0) return;
        if (RedirectText?.Invoke(text) == true) return; // editor už čeká, dostane celý text
        PushChar(text[0]);
        if (text.Length > 1) PendingPaste?.Invoke(text);
    }

    /// <summary>Obsah pole, které DataEditor hlásí jako aktuální; jinak oblast kolem kurzoru.</summary>
    public string CurrentFieldText()
    {
        // DataEditor zná celou hodnotu pole, i když je na obrazovce jen její část
        var buf = new byte[1024];
        if (Native.FandGetCurrentField(out _, out _, out _, buf, buf.Length) != 0)
        {
            string full = Cp852.Decode(buf).Trim(); // číselná pole jsou zarovnaná doprava, úvodní mezery nejsou hodnota
            if (full.Length > 0) return full;
        }
        if (_fieldX >= 0 && _fieldY >= 0 && _fieldY < _rows && _fieldLen > 0)
        {
            var sb = new StringBuilder();
            for (int i = _fieldX; i < Math.Min(_cols, _fieldX + _fieldLen); i++)
                sb.Append(Cp852.ToChar((byte)(_cells[_fieldY * _cols + i] & 0xFF)));
            string t = sb.ToString().Trim();
            return t.Length > 0 && t.All(c => c == '.') ? "" : t; // prázdné pole FAND vykresluje tečkami
        }
        return HighlightedRunAtCursor();
    }

    /// <summary>
    /// Text souvislé oblasti se stejným atributem kolem kurzoru (zvýrazněné pole pod kurzorem),
    /// bez okrajových mezer. Prázdný řetězec, když kurzor není na obrazovce.
    /// </summary>
    public string HighlightedRunAtCursor()
    {
        int x = _cursorX, y = _cursorY;
        if (x < 0 || y < 0 || x >= _cols || y >= _rows) return "";
        byte attr = (byte)(_cells[y * _cols + x] >> 8);
        int from = x, to = x;
        while (from > 0 && (byte)(_cells[y * _cols + from - 1] >> 8) == attr) from--;
        while (to < _cols - 1 && (byte)(_cells[y * _cols + to + 1] >> 8) == attr) to++;
        var sb = new StringBuilder();
        for (int i = from; i <= to; i++) sb.Append(Cp852.ToChar((byte)(_cells[y * _cols + i] & 0xFF)));
        return sb.ToString().Trim();
    }
}
