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
    private const ushort KeyEsc = 0x1B;                 // VK_ESCAPE, bez VIRTUAL
    private const ushort KeyF2 = Virtual + 0x71;        // VIRTUAL + VK_F2

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

    public TextEditWindow(Native.TextEditInfo info, string text)
    {
        InitializeComponent();

        _colors.LoadFrom(info);
        _originalText = text;

        var doc = new TextDocument(text);
        _state = new AttributeState(doc);
        _colorizer = new FandColorizer(_state, _colors);
        _generator = new ControlCharGenerator();

        Mode = info.Scrolling != 0 ? FandMode.Prohlizeni : FandMode.Editace;

        Editor.Document = doc;
        Editor.Options.ShowBoxForControlCharacters = false;
        Editor.Options.EnableRectangularSelection = true;   // = sloupcovy blok FANDu
        Editor.Options.ConvertTabsToSpaces = false;
        Editor.WordWrap = false;
        Editor.IsReadOnly = info.ReadOnly != 0;
        Editor.Background = new SolidColorBrush(_colors.Color(_colors.Background));
        Editor.Foreground = new SolidColorBrush(_colors.Color(_colors.TxtColor));
        Editor.TextArea.TextView.LineTransformers.Add(_colorizer);
        Editor.TextArea.TextView.ElementGenerators.Add(_generator);

        string name = Cp852.Decode(info.Name).Trim();
        Title = name.Length > 0 ? name : "Text";
        if (Editor.IsReadOnly) Title += " (jen prohlížení)";

        if (info.Pos > 0 && info.Pos < doc.TextLength) Editor.CaretOffset = info.Pos;

        Editor.TextArea.Caret.PositionChanged += (_, _) => UpdateStatus();
        Loaded += (_, _) => { Editor.Focus(); UpdateStatus(); };
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

    private void Finish(ushort key)
    {
        ResultText = Editor.Document.Text;
        ResultPos = Editor.CaretOffset;
        ResultUpdated = ResultText != _originalText;
        ResultKey = key;
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
