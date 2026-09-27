using System.Globalization;
using System.Printing;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace WpfHost;

/// <summary>Tisk sestavy na tiskárně Windows přes běžný tiskový dialog.</summary>
public static class ReportPrinter
{
    /// <summary>
    /// Zobrazí tiskový dialog a vytiskne. Vrací false, když uživatel tisk zrušil.
    /// Orientace se předvyplní podle sestavy (široké sestavy na šířku).
    /// </summary>
    public static bool PrintWithDialog(Window? owner, ReportDocument doc, string title, int copies = 1)
    {
        var dialog = new PrintDialog { UserPageRangeEnabled = true };
        try
        {
            var suggested = ReportLayout.CreateBest(doc, ReportLayout.A4);
            dialog.PrintTicket.PageOrientation = suggested.Landscape ? PageOrientation.Landscape : PageOrientation.Portrait;
            if (copies > 1) dialog.PrintTicket.CopyCount = copies;
            dialog.MinPage = 1;
            dialog.MaxPage = (uint)Math.Max(1, suggested.Pages.Count);
        }
        catch (Exception)
        {
            // tiskárna nemusí orientaci nebo kopie podporovat; dialog si poradí sám
        }

        if (dialog.ShowDialog() != true) return false;

        var layout = LayoutFor(dialog, doc);
        DocumentPaginator paginator = new ReportPaginator(layout);
        if (dialog.PageRangeSelection == PageRangeSelection.UserPages)
            paginator = new PageRangePaginator(paginator, dialog.PageRange);
        dialog.PrintDocument(paginator, string.IsNullOrWhiteSpace(title) ? "FAND" : title);
        return true;
    }

    /// <summary>Rozvržení pro papír a orientaci zvolenou v dialogu.</summary>
    private static ReportLayout LayoutFor(PrintDialog dialog, ReportDocument doc)
    {
        Size paper = ReportLayout.A4;
        bool landscape = false;
        try
        {
            var size = dialog.PrintTicket.PageMediaSize;
            if (size?.Width is double w && size.Height is double h && w > 0 && h > 0)
                paper = new Size(Math.Min(w, h), Math.Max(w, h));
            landscape = dialog.PrintTicket.PageOrientation is PageOrientation.Landscape or PageOrientation.ReverseLandscape;
        }
        catch (Exception)
        {
            // bez informací o papíru tiskneme na A4 podle šířky oblasti tisku
            landscape = dialog.PrintableAreaWidth > dialog.PrintableAreaHeight;
        }
        return ReportLayout.Create(doc, paper, landscape);
    }

    /// <summary>Stránka sestavy jako vizuál; sdílí ho tisk i náhled.</summary>
    public static DrawingVisual RenderPage(ReportLayout layout, int pageIndex)
    {
        var visual = new DrawingVisual();
        using var dc = visual.RenderOpen();
        // bílé pozadí, jinak by náhled a některé ovladače kreslily průhledně
        dc.DrawRectangle(Brushes.White, null, new Rect(layout.Paper));
        var family = new FontFamily(ReportLayout.FontFamilyName);
        var page = layout.Pages[pageIndex];
        for (int i = 0; i < page.Count; i++)
        {
            double top = layout.LineTop(i);
            foreach (var (run, x) in layout.Place(page[i]))
            {
                var typeface = new Typeface(family,
                    run.IsItalic ? FontStyles.Italic : FontStyles.Normal,
                    run.IsBold ? FontWeights.Bold : FontWeights.Normal,
                    FontStretches.Normal);
                var text = new FormattedText(run.Text, CultureInfo.CurrentCulture, FlowDirection.LeftToRight,
                    typeface, layout.FontSize, Brushes.Black, 1.0);
                if (run.IsUnderline) text.SetTextDecorations(TextDecorations.Underline);
                double factor = run.WidthFactor;
                // zhuštěné a široké písmo: vodorovné zúžení/roztažení běžného glyfu
                if (Math.Abs(factor - 1) > 1e-6) dc.PushTransform(new ScaleTransform(factor, 1, x, top));
                dc.DrawText(text, new Point(x, top));
                if (Math.Abs(factor - 1) > 1e-6) dc.Pop();
            }
        }
        return visual;
    }

    private sealed class ReportPaginator : DocumentPaginator
    {
        private readonly ReportLayout _layout;
        public ReportPaginator(ReportLayout layout) { _layout = layout; }

        public override DocumentPage GetPage(int pageNumber)
        {
            if (pageNumber < 0 || pageNumber >= _layout.Pages.Count) return DocumentPage.Missing;
            var rect = new Rect(_layout.Paper);
            return new DocumentPage(RenderPage(_layout, pageNumber), _layout.Paper, rect, rect);
        }

        public override bool IsPageCountValid => true;
        public override int PageCount => _layout.Pages.Count;
        public override Size PageSize { get => _layout.Paper; set { } }
        public override IDocumentPaginatorSource? Source => null;
    }

    /// <summary>Jen stránky z rozsahu zadaného v dialogu (číslované od 1).</summary>
    private sealed class PageRangePaginator : DocumentPaginator
    {
        private readonly DocumentPaginator _inner;
        private readonly int _first, _count;

        public PageRangePaginator(DocumentPaginator inner, PageRange range)
        {
            _inner = inner;
            _first = Math.Max(0, range.PageFrom - 1);
            int last = Math.Min(inner.PageCount, Math.Max(range.PageFrom, range.PageTo));
            _count = Math.Max(0, last - _first);
        }

        public override DocumentPage GetPage(int pageNumber) => _inner.GetPage(_first + pageNumber);
        public override bool IsPageCountValid => true;
        public override int PageCount => _count;
        public override Size PageSize { get => _inner.PageSize; set => _inner.PageSize = value; }
        public override IDocumentPaginatorSource? Source => null;
    }
}
