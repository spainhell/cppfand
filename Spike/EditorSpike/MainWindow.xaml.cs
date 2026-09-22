using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using ICSharpCode.AvalonEdit.Document;

namespace EditorSpike;

public partial class MainWindow : Window
{
    private readonly FandColors _colors = new();
    private AttributeState _state = null!;
    private FandColorizer _colorizer = null!;
    private ControlCharGenerator _generator = null!;

    public MainWindow()
    {
        InitializeComponent();

        Editor.Options.ShowBoxForControlCharacters = false;   // vlastni vizual si delame sami
        Editor.Options.EnableRectangularSelection = true;     // = sloupcovy blok FANDu (ColBlock)
        Editor.Options.EnableTextDragDrop = true;
        Editor.Options.ConvertTabsToSpaces = false;
        Editor.WordWrap = false;

        BuildColorControls();
        ApplyColors();

        Editor.TextArea.Caret.PositionChanged += (_, _) => UpdateStatus();
        Editor.TextArea.SelectionChanged += (_, _) => UpdateStatus();

        // Original prepina prohlizeci rezim podle ScrollLocku -- EDEVINPT.PAS:10 cte
        // BIOS bajt 0040:0017, bit 0x10, v C++ je to TextEditor.cpp:1274. Na dnesnich
        // klavesnicich ScrollLock casto neni, takze ho zastupuje F12; tu PC-FAND nepouziva.
        PreviewKeyDown += (_, e) =>
        {
            if (e.Key != Key.F12) return;
            ModeBox.SelectedIndex = ModeBox.SelectedIndex == 1 ? 0 : 1;
            e.Handled = true;
        };

        Loaded += (_, _) =>
        {
            string start = App.StartFile ?? Path.Combine(AppContext.BaseDirectory, "vzorek-help.t00");
            if (File.Exists(start)) LoadFile(start);
            else FileText.Text = $"{start} nenalezen, otevřete soubor ručně";
            Editor.Focus();
        };
    }

    private void LoadFile(string path)
    {
        var sw = System.Diagnostics.Stopwatch.StartNew();
        byte[] bytes = File.ReadAllBytes(path);
        var doc = new TextDocument(FandText.Decode(bytes));

        _state = new AttributeState(doc);
        _colorizer = new FandColorizer(_state, _colors) { Mode = CurrentMode };
        _generator = new ControlCharGenerator { Mode = CurrentMode };

        Editor.Document = doc;
        Editor.TextArea.TextView.LineTransformers.Clear();
        Editor.TextArea.TextView.LineTransformers.Add(_colorizer);
        Editor.TextArea.TextView.ElementGenerators.Clear();
        Editor.TextArea.TextView.ElementGenerators.Add(_generator);

        // stav atributu se pocita pro cely dokument, at je zmereny i on
        _state.AtLineStart(doc.LineCount);
        sw.Stop();

        FileText.Text = $"{Path.GetFileName(path)} · {bytes.Length} B · {doc.LineCount} řádků · načteno za {sw.ElapsedMilliseconds} ms";
        UpdateStatus();
    }

    private FandMode CurrentMode =>
        ModeBox.SelectedIndex == 1 ? FandMode.Prohlizeni : FandMode.Editace;

    private void Open(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Textový soubor FANDu",
            Filter = "Texty FANDu (*.T00;*.TXT;*.t00)|*.T00;*.TXT;*.t00|Všechny soubory (*.*)|*.*",
            InitialDirectory = Directory.Exists(@"C:\PCFAND\orig_ulohy\UCTO2024")
                ? @"C:\PCFAND\orig_ulohy\UCTO2024"
                : AppContext.BaseDirectory,
        };
        if (dlg.ShowDialog() == true) LoadFile(dlg.FileName);
    }

    private void ModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_colorizer == null) return;
        _colorizer.Mode = CurrentMode;
        _generator.Mode = CurrentMode;
        Editor.TextArea.TextView.Redraw();
        UpdateStatus();
    }

    private void FontChanged(object sender, SelectionChangedEventArgs e)
    {
        if (Editor == null) return;
        var item = (ComboBoxItem)FontBox.SelectedItem;
        Editor.FontSize = double.Parse((string)item.Content);
    }

    private void BuildColorControls()
    {
        AddColorRow("text bez atributu (tNorm)", () => _colors.TxtColor, v => _colors.TxtColor = v);
        AddColorRow("pozadí", () => _colors.Background, v => _colors.Background = v);
        AddColorRow("ostatní řídicí znaky (tCtrl)", () => _colors.ColKey[0], v => _colors.ColKey[0] = v);
        foreach (var a in FandText.Attributes)
        {
            int idx = a.ColKeyIndex;
            AddColorRow(a.Name, () => _colors.ColKey[idx], v => _colors.ColKey[idx] = v);
        }
    }

    private void AddColorRow(string label, Func<int> get, Action<int> set)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 2, 12, 2) };
        panel.Children.Add(new TextBlock { Text = label, Width = 190, VerticalAlignment = VerticalAlignment.Center });

        var combo = new ComboBox { Width = 130 };
        for (int i = 0; i < FandText.PaletteNames.Length; i++)
        {
            combo.Items.Add(new ComboBoxItem
            {
                Content = FandText.PaletteNames[i],
                Background = new SolidColorBrush(FandText.Palette[i]),
                Foreground = new SolidColorBrush(i < 8 ? Colors.White : Colors.Black),
            });
        }
        combo.SelectedIndex = get();
        combo.SelectionChanged += (_, _) => { set(combo.SelectedIndex); ApplyColors(); };
        panel.Children.Add(combo);
        ColorGrid.Children.Add(panel);
    }

    private void ApplyColors()
    {
        Editor.Background = new SolidColorBrush(FandText.Palette[_colors.Background]);
        Editor.Foreground = new SolidColorBrush(FandText.Palette[_colors.TxtColor]);
        Editor.TextArea.TextView.Redraw();
    }

    private void UpdateStatus()
    {
        if (Editor.Document == null || _state == null) return;

        var caret = Editor.TextArea.Caret;
        int offset = caret.Offset;
        PosText.Text = $"řádek {caret.Line}, sloupec {caret.Column}, offset {offset}";

        AttrText.Text = "platí: " + AttributeState.Describe(_state.AtOffset(offset));

        DocumentLine line = Editor.Document.GetLineByNumber(caret.Line);
        string konec = line.DelimiterLength switch
        {
            2 => "tvrdý konec (CR LF)",
            1 => "měkký konec (CR)",
            _ => "bez konce",
        };
        // Delka radku se nijak neomezuje. Limit 255 v originalu byl jen dusledek
        // pascalovskeho string[255], ne vlastnost formatu textu.
        LineText.Text = $"délka řádku {line.Length}, {konec}";
    }
}
