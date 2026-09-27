using System.IO;
using PdfSharp.Drawing;
using PdfSharp.Fonts;
using PdfSharp.Pdf;

namespace WpfHost;

/// <summary>Export sestavy do PDF (PDFsharp) se stejným rozvržením jako tisk.</summary>
public static class ReportPdf
{
    static ReportPdf()
    {
        // Consolas se vloží do PDF (i s českými znaky); soubory bere přímo z Windows.
        // GlobalFontSettings.UseWindowsFontsUnderWindows v PDFsharp 6.2.4 padá
        // na SynchronizationLockException, proto vlastní resolver.
        if (GlobalFontSettings.FontResolver == null) GlobalFontSettings.FontResolver = new WindowsFontResolver();
    }

    /// <summary>Consolas (a jeho řezy) ze složky Fonts ve Windows.</summary>
    private sealed class WindowsFontResolver : IFontResolver
    {
        private static readonly string FontsDir = Environment.GetFolderPath(Environment.SpecialFolder.Fonts);

        public FontResolverInfo? ResolveTypeface(string familyName, bool isBold, bool isItalic)
        {
            if (!familyName.Equals(ReportLayout.FontFamilyName, StringComparison.OrdinalIgnoreCase)) return null;
            string face = (isBold, isItalic) switch
            {
                (true, true) => "consolaz.ttf",
                (true, false) => "consolab.ttf",
                (false, true) => "consolai.ttf",
                _ => "consola.ttf",
            };
            // chybí-li řez, PDFsharp ho nasimuluje z obyčejného
            if (!File.Exists(Path.Combine(FontsDir, face)))
                return new FontResolverInfo("consola.ttf", isBold, isItalic);
            return new FontResolverInfo(face);
        }

        public byte[]? GetFont(string faceName)
        {
            string path = Path.Combine(FontsDir, faceName);
            return File.Exists(path) ? File.ReadAllBytes(path) : null;
        }
    }

    private const double PtPerDip = 72.0 / 96.0;

    public static void Save(ReportDocument doc, string path, string title)
    {
        var layout = ReportLayout.CreateBest(doc, ReportLayout.A4);
        using var pdf = new PdfDocument();
        pdf.Info.Title = title;
        pdf.Info.Creator = "C++ FAND";

        var fonts = new Dictionary<XFontStyleEx, XFont>();
        XFont FontFor(ReportRun run)
        {
            var style = XFontStyleEx.Regular;
            if (run.IsBold) style |= XFontStyleEx.Bold;
            if (run.IsItalic) style |= XFontStyleEx.Italic;
            if (run.IsUnderline) style |= XFontStyleEx.Underline;
            if (!fonts.TryGetValue(style, out var font))
            {
                font = new XFont(ReportLayout.FontFamilyName, layout.FontSize * PtPerDip, style);
                fonts[style] = font;
            }
            return font;
        }

        for (int p = 0; p < layout.Pages.Count; p++)
        {
            var page = pdf.AddPage();
            page.Width = XUnit.FromPoint(layout.Paper.Width * PtPerDip);
            page.Height = XUnit.FromPoint(layout.Paper.Height * PtPerDip);
            using var gfx = XGraphics.FromPdfPage(page);
            var lines = layout.Pages[p];
            for (int i = 0; i < lines.Count; i++)
            {
                double baseline = (layout.LineTop(i) + layout.Baseline) * PtPerDip;
                foreach (var (run, x) in layout.Place(lines[i]))
                {
                    double xPt = x * PtPerDip;
                    double factor = run.WidthFactor;
                    var state = gfx.Save();
                    if (Math.Abs(factor - 1) > 1e-6) gfx.ScaleAtTransform(factor, 1, new XPoint(xPt, baseline));
                    gfx.DrawString(run.Text, FontFor(run), XBrushes.Black, new XPoint(xPt, baseline), XStringFormats.BaseLineLeft);
                    gfx.Restore(state);
                }
            }
        }

        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        pdf.Save(path);
    }
}
