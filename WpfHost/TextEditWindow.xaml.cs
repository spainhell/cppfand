using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using ICSharpCode.AvalonEdit.Document;

namespace WpfHost;

/// <summary>
/// Okenni nahrada textoveho editoru FANDu. Interpret ceka v RunTextEdit
/// (Drivers/host.h), hostitel mu tady necha text upravit a vrati ho i s
/// ukoncovaci klavesou. Vykreslovani atributu resi TextEditColorizer.cs.
/// </summary>
public partial class TextEditWindow : Window
{
    // klavesy v kodovani PressedKey::KeyCombination, viz Drivers/constants.h
    private const ushort Virtual = 0x8000;
    private const ushort Ctrl = 0x0200;
    private const ushort KeyEsc = 0x1B;                 // VK_ESCAPE, bez VIRTUAL
    private const ushort KeyEnter = 0x0D;               // VK_RETURN
    private const ushort KeyF1 = Virtual + 0x70;
    private const ushort KeyF2 = Virtual + 0x71;
    private const ushort KeyF6 = Virtual + 0x75;
    private const ushort KeyF10 = Virtual + 0x79;
    private const ushort KeyCtrlHome = Virtual + Ctrl + 0x24;   // VK_HOME
    private const ushort KeyCtrlEnd = Virtual + Ctrl + 0x23;    // VK_END

    /// <summary>
    /// Odkaz nápovědy: text mezi dvojicí 0x13, jako HelpViewer::FindAllWords.
    /// Obyčejná struktura, ne record -- ten na net48 potřebuje IsExternalInit.
    /// </summary>
    private struct Link
    {
        public int Start;
        public int End;
        public Link(int start, int end) { Start = start; End = end; }
    }

    private readonly List<Link> _links = new();
    private int _linkIndex = -1;
    private readonly bool _isHelp;

    private readonly FandColors _colors = new();
    private readonly AttributeState _state;
    private readonly FandColorizer _colorizer;
    private readonly ControlCharGenerator _generator;
    private readonly string _originalText;

    /// <summary>Text po editaci (CP852 uz si prevede volajici).</summary>
    public string ResultText { get; private set; } = "";
    public int ResultPos { get; private set; }
    public bool ResultUpdated { get; private set; }
    public ushort ResultKey { get; private set; } = KeyEsc;

    /// <summary>Zvolený odkaz nápovědy; interpret ho uloží do gc->LexWord.</summary>
    public string ResultWord { get; private set; } = "";

    public TextEditWindow(Native.TextEditInfo info, string text)
    {
        InitializeComponent();

        _colors.LoadFrom(info);
        _originalText = text;
        _isHelp = info.Mode == (int)Native.EditorMode.Help;

        var doc = new TextDocument(text);
        _state = new AttributeState(doc);
        _colorizer = new FandColorizer(_state, _colors);
        _generator = new ControlCharGenerator();

        // Nápověda se kreslí vždycky prohlížecím způsobem: HelpScroll je
        // v TextEditor.cpp:2800 nastaveno na true pro celý režim Help.
        Mode = (_isHelp || info.Scrolling != 0) ? FandMode.Prohlizeni : FandMode.Editace;

        Editor.Document = doc;
        Editor.Options.ShowBoxForControlCharacters = false;
        Editor.Options.EnableRectangularSelection = true;   // = sloupcovy blok FANDu
        Editor.Options.ConvertTabsToSpaces = false;
        Editor.WordWrap = false;
        Editor.IsReadOnly = info.ReadOnly != 0 || _isHelp;
        // TxtColor je celý atribut, proto se rozpadá na písmo a pozadí
        Editor.Background = new SolidColorBrush(_colors.Color(_colors.Background));
        Editor.Foreground = new SolidColorBrush(_colors.Color(FandColors.Fg(_colors.TxtColor)));
        Editor.TextArea.TextView.LineTransformers.Add(_colorizer);
        Editor.TextArea.TextView.ElementGenerators.Add(_generator);

        string name = Cp852.Decode(info.Name).Trim();
        Title = _isHelp ? "Nápověda" : (name.Length > 0 ? name : "Text");
        if (Editor.IsReadOnly && !_isHelp) Title += " (jen prohlížení)";

        if (info.Pos > 0 && info.Pos < doc.TextLength) Editor.CaretOffset = info.Pos;

        if (_isHelp)
        {
            FindLinks(text);
            HintText.Text = "Tab/šipky odkaz · Enter otevřít · F10 zpět · Esc zavřít";
            SelectLinkAt(Editor.CaretOffset);
        }

        Editor.TextArea.Caret.PositionChanged += (_, _) => UpdateStatus();
        Loaded += (_, _) => { Editor.Focus(); UpdateStatus(); };
    }

    /// <summary>Odkazy jsou úseky mezi dvojicemi 0x13, jako HelpViewer::FindAllWords.</summary>
    private void FindLinks(string text)
    {
        int i = 0;
        while (true)
        {
            int start = text.IndexOf('\x13', i);
            if (start < 0) break;
            int end = text.IndexOf('\x13', start + 1);
            if (end < 0) break;
            _links.Add(new Link(start, end));
            i = end + 1;
        }
    }

    /// <summary>Vybere odkaz, ve kterém pozice leží, jinak nejbližší následující.</summary>
    private void SelectLinkAt(int offset)
    {
        if (_links.Count == 0) return;
        int found = _links.FindIndex(l => offset >= l.Start && offset <= l.End);
        if (found < 0) found = _links.FindIndex(l => l.Start >= offset);
        SelectLink(found < 0 ? 0 : found);
    }

    private void SelectLink(int index)
    {
        if (_links.Count == 0) return;
        _linkIndex = (index + _links.Count) % _links.Count;
        Link link = _links[_linkIndex];
        _colorizer.SelectedLinkStart = link.Start;
        _colorizer.SelectedLinkEnd = link.End;
        Editor.CaretOffset = link.Start + 1;
        Editor.ScrollToLine(Editor.Document.GetLineByOffset(link.Start).LineNumber);
        Editor.TextArea.TextView.Redraw();
    }

    /// <summary>Text zvoleného odkazu bez oddělovačů; z něj FAND udělá název kapitoly.</summary>
    private string SelectedLinkText()
    {
        if (_linkIndex < 0 || _linkIndex >= _links.Count) return "";
        Link link = _links[_linkIndex];
        return Editor.Document.GetText(link.Start + 1, link.End - link.Start - 1).Trim();
    }

    private FandMode Mode
    {
        get => _colorizer.Mode;
        set
        {
            _colorizer.Mode = value;
            _generator.Mode = value;
            Editor.TextArea.TextView.Redraw();
        }
    }

    protected override void OnPreviewKeyDown(KeyEventArgs e)
    {
        if (_isHelp && HandleHelpKey(e)) return;

        switch (e.Key)
        {
            // ScrollLock puvodniho editoru (EDEVINPT.PAS:10, TextEditor.cpp:1274);
            // dnesni klavesnice ho casto nemaji, zastupuje ho F12
            case Key.F12:
                Mode = Mode == FandMode.Prohlizeni ? FandMode.Editace : FandMode.Prohlizeni;
                e.Handled = true;
                return;

            case Key.F2:
                Finish(KeyF2);
                e.Handled = true;
                return;

            case Key.Escape:
                Finish(KeyEsc);
                e.Handled = true;
                return;
        }
        base.OnPreviewKeyDown(e);
    }

    /// <summary>
    /// Klávesy nápovědy. Ukončovací sadu má HelpViewer v konstruktoru
    /// (F1, F6, F10, Ctrl+Home, Ctrl+End); podle ní se pak větví
    /// EditorHelp.cpp, proto se musí vrátit přesně.
    /// </summary>
    private bool HandleHelpKey(KeyEventArgs e)
    {
        bool ctrl = (Keyboard.Modifiers & ModifierKeys.Control) != 0;
        bool shift = (Keyboard.Modifiers & ModifierKeys.Shift) != 0;

        switch (e.Key)
        {
            case Key.Tab:
                SelectLink(_linkIndex + (shift ? -1 : 1));
                e.Handled = true;
                return true;

            case Key.Right or Key.Down when !ctrl:
                SelectLink(_linkIndex + 1);
                e.Handled = true;
                return true;

            case Key.Left or Key.Up when !ctrl:
                SelectLink(_linkIndex - 1);
                e.Handled = true;
                return true;

            case Key.Enter:
                Finish(KeyEnter, SelectedLinkText());
                e.Handled = true;
                return true;

            case Key.Home when ctrl:
                Finish(KeyCtrlHome);
                e.Handled = true;
                return true;

            case Key.End when ctrl:
                Finish(KeyCtrlEnd);
                e.Handled = true;
                return true;

            case Key.F1: Finish(KeyF1); e.Handled = true; return true;
            case Key.F6: Finish(KeyF6); e.Handled = true; return true;
            case Key.F10: Finish(KeyF10); e.Handled = true; return true;
            case Key.Escape: Finish(KeyEsc); e.Handled = true; return true;
        }
        return false;
    }

    private void Finish(ushort key, string word = "")
    {
        ResultText = Editor.Document.Text;
        ResultPos = Editor.CaretOffset;
        ResultUpdated = !_isHelp && ResultText != _originalText;
        ResultKey = key;
        ResultWord = word;
        DialogResult = true;
    }

    protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
    {
        // zavreni krizkem se chova jako Esc, aby interpret nezustal viset v RunTextEdit
        if (DialogResult == null) Finish(KeyEsc);
        base.OnClosing(e);
    }

    private void UpdateStatus()
    {
        var caret = Editor.TextArea.Caret;
        PosText.Text = $"řádek {caret.Line}, sloupec {caret.Column}, offset {caret.Offset}";
        AttrText.Text = "platí: " + AttributeState.Describe(_state.AtOffset(caret.Offset));
    }
}
