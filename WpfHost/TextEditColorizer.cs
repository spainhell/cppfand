using System.Text;
using System.Windows.Media;
using System.Windows.Media.TextFormatting;
using ICSharpCode.AvalonEdit.Document;
using ICSharpCode.AvalonEdit.Rendering;

namespace WpfHost;

internal enum FandMode
{
    /// <summary>TextEditorScreen::EditWrline -- ridici znak je videt, zabira sloupec.</summary>
    Editace,
    /// <summary>TextEditorScreen::ScrollWrline -- ridici znak je skryty, prepina barvu textu.</summary>
    Prohlizeni,
}

/// <summary>
/// Barvy pro editor. Atribut z FANDu je bajt, dolni nibble je barva pisma
/// a horni pozadi -- proto se vsude maskuje 0x0F. Hodnoty plni interpret
/// z FAND.CFG (ColKey[] v TextEditor.cpp:3192), vychozi jsou jen pojistka,
/// kdyby pozadavek prisel s nulami.
/// </summary>
internal sealed class FandColors
{
    public int[] ColKey = { 12, 11, 13, 14, 10, 9, 15, 6 };
    public int TxtColor = 7;
    public int BlockColor = 1;
    public int Background = 0;

    /// <summary>Prevezme barvy z pozadavku interpretu; nuly znamenaji "nenastaveno".</summary>
    public void LoadFrom(Native.TextEditInfo info)
    {
        for (int i = 0; i < ColKey.Length && i < info.ColKey.Length; i++)
            if (info.ColKey[i] != 0) ColKey[i] = info.ColKey[i] & 0x0F;

        if (info.TxtColor != 0)
        {
            TxtColor = info.TxtColor & 0x0F;
            Background = (info.TxtColor >> 4) & 0x0F;
        }
        if (info.BlockColor != 0) BlockColor = info.BlockColor & 0x0F;
    }

    public Color Color(int paletteIndex) => TerminalControl.PaletteColor(paletteIndex);

    public Brush Brush(int paletteIndex)
    {
        var b = new SolidColorBrush(Color(paletteIndex));
        b.Freeze();
        return b;
    }
}

/// <summary>
/// Stav atributu ("ColorOrd" z TextEditor.h) na zacatku kazdeho radku.
///
/// ColorOrd je retezec prave aktivnich prepinacu v poradi zapnuti; barva textu
/// je dana tim poslednim. Ridici znak funguje jako prepinac: kdyz uz v retezci
/// je, odebere se, jinak se prida.
///
/// Pozor: original ma na dvou mistech ruznou semantiku odebrani. ScrollWrline
/// (TextEditorScreen.cpp) odebere jen ten jeden znak, kdezto SetColorOrd
/// (TextEditor.cpp:1019) vola co.erase(pp), coz v std::string zahodi vse od
/// pozice pp az do konce. Spike se drzi varianty ze ScrollWrline.
/// </summary>
internal sealed class AttributeState
{
    private readonly TextDocument _doc;
    private string[] _lineStart = Array.Empty<string>();
    private bool _valid;

    public AttributeState(TextDocument doc)
    {
        _doc = doc;
        _doc.Changed += (_, _) => _valid = false;
        _doc.TextChanged += (_, _) => _valid = false;
    }

    public static string Toggle(string co, char c)
    {
        int pos = co.IndexOf(c);
        return pos < 0 ? co + c : co.Remove(pos, 1);
    }

    public static int ColorIndexOf(string co, FandColors colors) =>
        co.Length == 0 ? colors.TxtColor : colors.ColKey[FandAttr.ColKeyIndexOf(co[co.Length - 1])];

    private void Rebuild()
    {
        _lineStart = new string[_doc.LineCount + 1];
        string co = "";
        int i = 0;
        foreach (DocumentLine line in _doc.Lines)
        {
            _lineStart[i++] = co;
            string text = _doc.GetText(line.Offset, line.Length);
            foreach (char c in text)
                if (FandAttr.IsAttr(c)) co = Toggle(co, c);
        }
        _lineStart[i] = co;
        _valid = true;
    }

    /// <summary>ColorOrd platny na zacatku radku (1-based cislo radku).</summary>
    public string AtLineStart(int lineNumber)
    {
        if (!_valid) Rebuild();
        int i = lineNumber - 1;
        return i >= 0 && i < _lineStart.Length ? _lineStart[i] : "";
    }

    /// <summary>ColorOrd platny na dane pozici v dokumentu -- pro stavovy radek.</summary>
    public string AtOffset(int offset)
    {
        DocumentLine line = _doc.GetLineByOffset(offset);
        string co = AtLineStart(line.LineNumber);
        string text = _doc.GetText(line.Offset, offset - line.Offset);
        foreach (char c in text)
            if (FandAttr.IsAttr(c)) co = Toggle(co, c);
        return co;
    }

    public static string Describe(string co)
    {
        if (co.Length == 0) return "žádný atribut";
        var sb = new StringBuilder();
        foreach (char c in co)
        {
            if (sb.Length > 0) sb.Append(" + ");
            sb.Append(FandAttr.NameOf(c));
        }
        return sb.ToString();
    }
}

/// <summary>
/// Obarveni textu. Editace a prohlizeni se chovaji ruzne -- viz TextEditorScreen.cpp:
/// v EditWrline dostava bezny text vzdy TxtColor a obarveny je jen samotny ridici
/// znak, kdezto v ScrollWrline se podle aktivniho atributu barvi navazujici text.
/// </summary>
internal sealed class FandColorizer : DocumentColorizingTransformer
{
    private readonly AttributeState _state;
    private readonly FandColors _colors;

    public FandMode Mode { get; set; } = FandMode.Editace;

    public FandColorizer(AttributeState state, FandColors colors)
    {
        _state = state;
        _colors = colors;
    }

    protected override void ColorizeLine(DocumentLine line)
    {
        if (line.Length == 0) return;

        string text = CurrentContext.Document.GetText(line.Offset, line.Length);
        string co = _state.AtLineStart(line.LineNumber);

        if (Mode == FandMode.Editace)
        {
            // jen ridici znaky, kazdy svoji barvou; zbytek nechavame na TxtColor
            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];
                if (c >= 32) continue;
                Brush brush = _colors.Brush(_colors.ColKey[FandAttr.ColKeyIndexOf(c)]);
                int start = line.Offset + i;
                ChangeLinePart(start, start + 1, el => el.TextRunProperties.SetForegroundBrush(brush));
            }
            return;
        }

        // prohlizeni: barvu drzi ColorOrd a plati az do dalsiho prepinace
        int segStart = 0;
        int colorIndex = AttributeState.ColorIndexOf(co, _colors);
        for (int i = 0; i < text.Length; i++)
        {
            if (!FandAttr.IsAttr(text[i])) continue;
            Paint(line, segStart, i, colorIndex);
            co = AttributeState.Toggle(co, text[i]);
            colorIndex = AttributeState.ColorIndexOf(co, _colors);
            segStart = i;   // samotny prepinac je skryty, barvu uz ma novou
        }
        Paint(line, segStart, text.Length, colorIndex);
    }

    private void Paint(DocumentLine line, int from, int to, int colorIndex)
    {
        if (to <= from) return;
        Brush brush = _colors.Brush(colorIndex);
        ChangeLinePart(line.Offset + from, line.Offset + to,
            el => el.TextRunProperties.SetForegroundBrush(brush));
    }
}

/// <summary>
/// Vizual ridicich znaku. V editaci se ridici znak zobrazi jako pismeno c+64
/// (^S jako S) a zabira jeden sloupec, presne jako EditWrline. V prohlizeni se
/// skryje na nulovou sirku, jako to dela ScrollWrline; 0x0C se kresli jako
/// plny blok, coz v originalu odpovida znaku 219.
/// </summary>
internal sealed class ControlCharGenerator : VisualLineElementGenerator
{
    public FandMode Mode { get; set; } = FandMode.Editace;

    public override int GetFirstInterestedOffset(int startOffset)
    {
        int end = CurrentContext.VisualLine.LastDocumentLine.EndOffset;
        var doc = CurrentContext.Document;
        for (int i = startOffset; i < end; i++)
        {
            char c = doc.GetCharAt(i);
            if (c < 32 && c != '\r' && c != '\n' && c != '\t') return i;
        }
        return -1;
    }

    public override VisualLineElement ConstructElement(int offset)
    {
        char c = CurrentContext.Document.GetCharAt(offset);
        if (c >= 32 || c == '\r' || c == '\n' || c == '\t') return null!;

        if (Mode == FandMode.Editace)
            return new FormattedTextElement(((char)(c + 64)).ToString(), 1);

        if (c == FandAttr.PageBreak)
            return new FormattedTextElement("█", 1);

        return new HiddenElement(1);
    }

    /// <summary>Znak, ktery je v dokumentu, ale na obrazovce nezabira zadne misto.</summary>
    private sealed class HiddenElement : VisualLineElement
    {
        public HiddenElement(int documentLength) : base(1, documentLength) { }

        public override TextRun CreateTextRun(int startVisualColumn, ITextRunConstructionContext context)
            => new TextCharacters("​".ToCharArray(), 0, 1, TextRunProperties);
    }
}
