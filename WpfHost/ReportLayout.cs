using System.Windows;
using System.Windows.Media;

namespace WpfHost;

/// <summary>
/// Rozvržení sestavy na papír: velikost neproporcionálního písma tak, aby se nejdelší
/// řádek vešel na šířku a stránka sestavy (oddělená 0x0C) na výšku, a rozdělení
/// příliš dlouhých stránek. Jednotky jsou DIP (1/96 palce); PDF, DOCX a ODT si
/// je přepočtou.
/// </summary>
public sealed class ReportLayout
{
    public const string FontFamilyName = "Consolas";

    /// <summary>Okraj papíru ze všech stran (15 mm).</summary>
    public static readonly double MarginDip = MmToDip(15);

    private const double MaxFontDip = 12 * 96.0 / 72;     // 12 pt, víc je na sestavu zbytečně velké
    private const double MinFontDip = 7 * 96.0 / 72;      // 7 pt, menší by se na výšku stránky nepřizpůsobovalo

    // metriky Consolas v em (šířka znaku, výška řádku, účaří); načtou se z písma
    private static readonly double AdvanceEm, LineEm, BaselineEm;

    static ReportLayout()
    {
        AdvanceEm = 0.5498; LineEm = 1.1709; BaselineEm = 0.9427;
        var tf = new Typeface(new FontFamily(FontFamilyName), FontStyles.Normal, FontWeights.Normal, FontStretches.Normal);
        if (tf.TryGetGlyphTypeface(out var g) && g.CharacterToGlyphMap.TryGetValue('W', out ushort gi))
        {
            AdvanceEm = g.AdvanceWidths[gi];
            LineEm = g.Height;
            BaselineEm = g.Baseline;
        }
    }

    public static double MmToDip(double mm) => mm * 96.0 / 25.4;

    public ReportLayout(ReportDocument doc, Size paper, bool landscape, List<List<ReportLine>> pages,
        double fontSize)
    {
        Document = doc;
        Paper = paper;
        Landscape = landscape;
        Pages = pages;
        FontSize = fontSize;
    }

    public ReportDocument Document { get; }
    /// <summary>Rozměr papíru v DIP (už otočený podle orientace).</summary>
    public Size Paper { get; }
    public bool Landscape { get; }
    public List<List<ReportLine>> Pages { get; }
    public double FontSize { get; }
    public double CellWidth => FontSize * AdvanceEm;
    public double LineHeight => FontSize * LineEm;
    public double Baseline => FontSize * BaselineEm;
    public double Margin => MarginDip;

    /// <summary>A4 v DIP.</summary>
    public static Size A4 => new(MmToDip(210), MmToDip(297));

    /// <summary>Rozvržení pro daný papír a orientaci (paper je na výšku).</summary>
    public static ReportLayout Create(ReportDocument doc, Size portraitPaper, bool landscape, double fontStep = 0)
    {
        Size paper = landscape ? new Size(portraitPaper.Height, portraitPaper.Width) : portraitPaper;
        double availW = paper.Width - 2 * MarginDip;
        double availH = paper.Height - 2 * MarginDip;

        double font = MaxFontDip;
        double units = doc.MaxUnits;
        if (units > 0) font = Math.Min(font, availW / (units * AdvanceEm));
        // stránka sestavy se má vejít celá, pokud to písmo ještě snese
        int maxLines = doc.MaxLines;
        if (maxLines > 0)
        {
            double byHeight = availH / (maxLines * LineEm);
            if (byHeight < font) font = Math.Max(byHeight, Math.Min(font, MinFontDip));
        }
        // DOCX a ODT zadávají písmo po půl bodech: zaokrouhlit dolů, ať se řádek nezalomí
        if (fontStep > 0) font = Math.Floor(font * 0.75 / fontStep) * fontStep / 0.75;

        int perPage = Math.Max(1, (int)Math.Floor(availH / (font * LineEm) + 1e-6));
        var pages = new List<List<ReportLine>>();
        foreach (var page in doc.Pages)
        {
            if (page.Count <= perPage) { pages.Add(page); continue; }
            for (int i = 0; i < page.Count; i += perPage)
                pages.Add(page.GetRange(i, Math.Min(perPage, page.Count - i)));
        }
        if (pages.Count == 0) pages.Add(new List<ReportLine>());
        return new ReportLayout(doc, paper, landscape, pages, font);
    }

    /// <summary>
    /// Vybere orientaci: přednost má ta, ve které se stránky sestavy nemusí dělit,
    /// pak ta s větším písmem.
    /// </summary>
    public static ReportLayout CreateBest(ReportDocument doc, Size portraitPaper, double fontStep = 0)
    {
        var portrait = Create(doc, portraitPaper, false, fontStep);
        var landscape = Create(doc, portraitPaper, true, fontStep);
        bool portraitSplits = portrait.Pages.Count > doc.Pages.Count;
        bool landscapeSplits = landscape.Pages.Count > doc.Pages.Count;
        if (portraitSplits != landscapeSplits) return portraitSplits ? landscape : portrait;
        return landscape.FontSize > portrait.FontSize * 1.05 ? landscape : portrait;
    }

    /// <summary>Pro každý úsek řádku: levý okraj v DIP. Neproporcionální písmo, takže stačí počítat znaky.</summary>
    public IEnumerable<(ReportRun Run, double X)> Place(ReportLine line)
    {
        double x = Margin;
        foreach (var run in line.Runs)
        {
            yield return (run, x);
            x += run.Units * CellWidth;
        }
    }

    public double LineTop(int index) => Margin + index * LineHeight;
}
