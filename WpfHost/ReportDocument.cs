using System.Text;

namespace WpfHost;

/// <summary>Styl úseku textu podle přepínacích znaků PC-FANDu (viz FandAttr.Attributes).</summary>
[Flags]
public enum ReportStyle
{
    None = 0,
    Underline = 1,      // ^S
    Italic = 2,         // ^W
    Wide = 4,           // ^Q dvojitá šířka
    DoubleStrike = 8,   // ^D dvojitý průchod, tiskne se tučně
    Bold = 16,          // ^B zvýraznění
    Compressed = 32,    // ^E zhuštěně (17 cpi)
    Elite = 64,         // ^A elite (12 cpi)
}

public sealed class ReportRun
{
    public ReportRun(string text, ReportStyle style) { Text = text; Style = style; }
    public string Text { get; }
    public ReportStyle Style { get; }
    public bool IsBold => (Style & (ReportStyle.Bold | ReportStyle.DoubleStrike)) != 0;
    public bool IsItalic => (Style & ReportStyle.Italic) != 0;
    public bool IsUnderline => (Style & ReportStyle.Underline) != 0;

    /// <summary>Šířka znaku vůči běžnému (10 cpi): zhuštěně 17 cpi, elite 12 cpi, široce dvojnásobek.</summary>
    public double WidthFactor
    {
        get
        {
            double f = (Style & ReportStyle.Compressed) != 0 ? 10.0 / 17.0
                     : (Style & ReportStyle.Elite) != 0 ? 10.0 / 12.0
                     : 1.0;
            return (Style & ReportStyle.Wide) != 0 ? f * 2 : f;
        }
    }

    /// <summary>Šířka úseku v běžných znacích.</summary>
    public double Units => Text.Length * WidthFactor;
}

public sealed class ReportLine
{
    public List<ReportRun> Runs { get; } = new();
    public double Units => Runs.Sum(r => r.Units);
    public bool IsEmpty => Runs.Count == 0;
}

/// <summary>
/// Výstup sestavy (PRINTER.TXT) nebo textu rozložený na stránky, řádky a úseky
/// se stylem. Text přichází jako CP852 dekódovaný přes <see cref="FandAttr.Decode"/>,
/// tedy s řídicími znaky 0..31 zachovanými: přepínače atributů (^S, ^B, ^E, …)
/// mění styl, 0x0C (FormFeed v Report.cpp) začíná novou stránku.
///
/// ESC sekvence tiskáren z FAND.CFG se tu nepoužívají; na tiskárně Windows se
/// atributy kreslí stylem písma.
/// </summary>
public sealed class ReportDocument
{
    public List<List<ReportLine>> Pages { get; } = new();

    /// <summary>Nejdelší řádek v běžných znacích.</summary>
    public double MaxUnits => Pages.SelectMany(p => p).Select(l => l.Units).DefaultIfEmpty(0).Max();

    /// <summary>Nejvíc řádků na jedné stránce (oddělené 0x0C).</summary>
    public int MaxLines => Pages.Select(p => p.Count).DefaultIfEmpty(0).Max();

    public static ReportDocument Parse(string text)
    {
        var doc = new ReportDocument();
        var page = new List<ReportLine>();
        var line = new ReportLine();
        var sb = new StringBuilder();
        var style = ReportStyle.None;
        int column = 0;

        void FlushRun()
        {
            if (sb.Length == 0) return;
            line.Runs.Add(new ReportRun(sb.ToString(), style));
            sb.Clear();
        }
        void EndLine()
        {
            FlushRun();
            page.Add(line);
            line = new ReportLine();
            column = 0;
        }
        void EndPage()
        {
            doc.Pages.Add(page);
            page = new List<ReportLine>();
        }

        foreach (char c in text)
        {
            switch (c)
            {
                case '\r':
                    continue;
                case '\n':
                    EndLine();
                    continue;
                case FandAttr.PageBreak:
                    // FormFeed stojí obvykle na začátku řádku; rozepsaný řádek dokončíme
                    if (!line.IsEmpty || sb.Length > 0) EndLine();
                    EndPage();
                    continue;
                case '\t':
                    int spaces = 8 - column % 8;
                    sb.Append(' ', spaces);
                    column += spaces;
                    continue;
            }
            if (c < 32)
            {
                var toggle = StyleOf(c);
                if (toggle != ReportStyle.None)
                {
                    FlushRun();
                    style ^= toggle;
                }
                // ostatní řídicí znaky (uživatelské kódy tiskárny ^X ^V ^T, 0x10 …) nemají na Windows význam
                continue;
            }
            sb.Append(c);
            column++;
        }
        if (!line.IsEmpty || sb.Length > 0) EndLine();
        if (page.Count > 0 || doc.Pages.Count == 0) EndPage();

        // sestava končí obvykle FormFeedem: poslední prázdnou stránku netiskneme
        while (doc.Pages.Count > 1 && doc.Pages[doc.Pages.Count - 1].All(l => l.IsEmpty))
            doc.Pages.RemoveAt(doc.Pages.Count - 1);
        return doc;
    }

    private static ReportStyle StyleOf(char c) => c switch
    {
        '\x13' => ReportStyle.Underline,
        '\x17' => ReportStyle.Italic,
        '\x11' => ReportStyle.Wide,
        '\x04' => ReportStyle.DoubleStrike,
        '\x02' => ReportStyle.Bold,
        '\x05' => ReportStyle.Compressed,
        '\x01' => ReportStyle.Elite,
        _ => ReportStyle.None,
    };
}
