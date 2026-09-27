using System.Globalization;
using System.IO;
using System.Text;

namespace WpfHost;

/// <summary>
/// Export sestavy do Wordu (DOCX) a LibreOffice/OpenOffice (ODT). Oba formáty jsou
/// zip s XML; pro neproporcionální text s několika styly stačí pár souborů, takže
/// se skládají ručně bez knihoven. Každý řádek sestavy je jeden odstavec s pevnou
/// výškou řádku, stránky sestavy začínají novou stránkou dokumentu.
/// </summary>
public static class ReportOffice
{
    private static readonly CultureInfo Inv = CultureInfo.InvariantCulture;
    private static readonly UTF8Encoding Utf8 = new(false);

    // Word i ODF zadávají písmo po půl bodech; rozvržení se na to zaokrouhlí dolů
    private const double HalfPoint = 0.5;

    private static string Pt(double dip) => (dip * 0.75).ToString("0.##", Inv) + "pt";
    private static int Twips(double dip) => (int)Math.Round(dip * 15);   // 1 DIP = 1/96", 1 twip = 1/1440"

    // ------------------------------------------------------------------ DOCX

    public static void SaveDocx(ReportDocument doc, string path)
    {
        var layout = ReportLayout.CreateBest(doc, ReportLayout.A4, HalfPoint);
        int halfPoints = (int)Math.Round(layout.FontSize * 0.75 * 2);
        int lineTwips = (int)Math.Floor(layout.LineHeight * 15);

        var body = new StringBuilder();
        for (int p = 0; p < layout.Pages.Count; p++)
        {
            var lines = layout.Pages[p];
            if (lines.Count == 0) lines = new List<ReportLine> { new() };
            for (int i = 0; i < lines.Count; i++)
            {
                body.Append("<w:p>");
                if (p > 0 && i == 0) body.Append("<w:pPr><w:pageBreakBefore/></w:pPr>");
                foreach (var run in lines[i].Runs)
                {
                    body.Append("<w:r>");
                    // pořadí prvků předepisuje schéma CT_RPr: b, i, …, w, …, u
                    var rpr = new StringBuilder();
                    if (run.IsBold) rpr.Append("<w:b/>");
                    if (run.IsItalic) rpr.Append("<w:i/>");
                    int scale = (int)Math.Round(run.WidthFactor * 100);
                    if (scale != 100) rpr.Append("<w:w w:val=\"").Append(scale).Append("\"/>");
                    if (run.IsUnderline) rpr.Append("<w:u w:val=\"single\"/>");
                    if (rpr.Length > 0) body.Append("<w:rPr>").Append(rpr).Append("</w:rPr>");
                    body.Append("<w:t xml:space=\"preserve\">").Append(Xml(run.Text)).Append("</w:t></w:r>");
                }
                body.Append("</w:p>");
            }
        }

        int w = Twips(layout.Paper.Width), h = Twips(layout.Paper.Height), m = Twips(layout.Margin);
        string orient = layout.Landscape ? " w:orient=\"landscape\"" : "";
        string document =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
            "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body>" +
            body +
            $"<w:sectPr><w:pgSz w:w=\"{w}\" w:h=\"{h}\"{orient}/>" +
            $"<w:pgMar w:top=\"{m}\" w:right=\"{m}\" w:bottom=\"{m}\" w:left=\"{m}\" w:header=\"0\" w:footer=\"0\" w:gutter=\"0\"/>" +
            "</w:sectPr></w:body></w:document>";

        string font = ReportLayout.FontFamilyName;
        string styles =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
            "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">" +
            "<w:docDefaults><w:rPrDefault><w:rPr>" +
            $"<w:rFonts w:ascii=\"{font}\" w:hAnsi=\"{font}\" w:cs=\"{font}\" w:eastAsia=\"{font}\"/>" +
            $"<w:sz w:val=\"{halfPoints}\"/><w:szCs w:val=\"{halfPoints}\"/><w:lang w:val=\"cs-CZ\"/>" +
            "</w:rPr></w:rPrDefault><w:pPrDefault><w:pPr>" +
            $"<w:spacing w:before=\"0\" w:after=\"0\" w:line=\"{lineTwips}\" w:lineRule=\"exact\"/>" +
            "</w:pPr></w:pPrDefault></w:docDefaults>" +
            "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\"><w:name w:val=\"Normal\"/></w:style>" +
            "</w:styles>";

        const string contentTypes =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">" +
            "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>" +
            "<Default Extension=\"xml\" ContentType=\"application/xml\"/>" +
            "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>" +
            "<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>" +
            "</Types>";
        const string rels =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" +
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>" +
            "</Relationships>";
        const string documentRels =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" +
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>" +
            "</Relationships>";

        WriteZip(path, new[]
        {
            ("[Content_Types].xml", contentTypes, true),
            ("_rels/.rels", rels, true),
            ("word/_rels/document.xml.rels", documentRels, true),
            ("word/document.xml", document, true),
            ("word/styles.xml", styles, true),
        });
    }

    // ------------------------------------------------------------------ ODT

    public static void SaveOdt(ReportDocument doc, string path)
    {
        var layout = ReportLayout.CreateBest(doc, ReportLayout.A4, HalfPoint);
        string font = ReportLayout.FontFamilyName;

        // automatické styly úseků podle kombinace tučně/kurzíva/podtržení/šířka
        var spanStyles = new Dictionary<string, string>();
        string SpanStyle(ReportRun run)
        {
            int scale = (int)Math.Round(run.WidthFactor * 100);
            string key = $"{(run.IsBold ? 1 : 0)}{(run.IsItalic ? 1 : 0)}{(run.IsUnderline ? 1 : 0)}_{scale}";
            if (key == "000_100") return "";
            if (!spanStyles.TryGetValue(key, out string? name))
            {
                name = "T" + (spanStyles.Count + 1);
                spanStyles[key] = name;
            }
            return name;
        }

        var body = new StringBuilder();
        for (int p = 0; p < layout.Pages.Count; p++)
        {
            var lines = layout.Pages[p];
            if (lines.Count == 0) lines = new List<ReportLine> { new() };
            for (int i = 0; i < lines.Count; i++)
            {
                string pStyle = p > 0 && i == 0 ? "P2" : "P1";
                body.Append("<text:p text:style-name=\"").Append(pStyle).Append("\">");
                bool lineStart = true;
                foreach (var run in lines[i].Runs)
                {
                    string span = SpanStyle(run);
                    if (span.Length > 0) body.Append("<text:span text:style-name=\"").Append(span).Append("\">");
                    AppendOdfText(body, run.Text, ref lineStart);
                    if (span.Length > 0) body.Append("</text:span>");
                }
                body.Append("</text:p>");
            }
        }

        var auto = new StringBuilder();
        string lineHeight = Pt(layout.LineHeight);
        auto.Append("<style:style style:name=\"P1\" style:family=\"paragraph\">")
            .Append($"<style:paragraph-properties fo:margin-top=\"0pt\" fo:margin-bottom=\"0pt\" fo:line-height=\"{lineHeight}\"/>")
            .Append("</style:style>");
        auto.Append("<style:style style:name=\"P2\" style:family=\"paragraph\" style:parent-style-name=\"Standard\">")
            .Append($"<style:paragraph-properties fo:margin-top=\"0pt\" fo:margin-bottom=\"0pt\" fo:line-height=\"{lineHeight}\" fo:break-before=\"page\"/>")
            .Append("</style:style>");
        foreach (var kv in spanStyles)
        {
            string[] parts = kv.Key.Split('_');
            var props = new StringBuilder();
            if (parts[0][0] == '1') props.Append(" fo:font-weight=\"bold\"");
            if (parts[0][1] == '1') props.Append(" fo:font-style=\"italic\"");
            if (parts[0][2] == '1') props.Append(" style:text-underline-style=\"solid\" style:text-underline-width=\"auto\" style:text-underline-color=\"font-color\"");
            if (parts[1] != "100") props.Append(" style:text-scale=\"").Append(parts[1]).Append("%\"");
            auto.Append($"<style:style style:name=\"{kv.Value}\" style:family=\"text\"><style:text-properties{props}/></style:style>");
        }

        const string ns =
            " xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\"" +
            " xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\"" +
            " xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\"" +
            " xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\"" +
            " xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\"" +
            " office:version=\"1.2\"";
        string fontDecl =
            "<office:font-face-decls>" +
            $"<style:font-face style:name=\"{font}\" svg:font-family=\"{font}\" style:font-pitch=\"fixed\"/>" +
            "</office:font-face-decls>";

        string content =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            $"<office:document-content{ns}>" + fontDecl +
            "<office:automatic-styles>" + auto + "</office:automatic-styles>" +
            "<office:body><office:text>" + body + "</office:text></office:body></office:document-content>";

        string orientation = layout.Landscape ? "landscape" : "portrait";
        string styles =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            $"<office:document-styles{ns}>" + fontDecl +
            "<office:styles>" +
            "<style:default-style style:family=\"paragraph\">" +
            $"<style:text-properties style:font-name=\"{font}\" fo:font-size=\"{Pt(layout.FontSize)}\" fo:language=\"cs\" fo:country=\"CZ\"/>" +
            "</style:default-style>" +
            "<style:style style:name=\"Standard\" style:family=\"paragraph\"/>" +
            "</office:styles>" +
            "<office:automatic-styles><style:page-layout style:name=\"pm1\">" +
            $"<style:page-layout-properties fo:page-width=\"{Pt(layout.Paper.Width)}\" fo:page-height=\"{Pt(layout.Paper.Height)}\"" +
            $" style:print-orientation=\"{orientation}\" fo:margin-top=\"{Pt(layout.Margin)}\" fo:margin-bottom=\"{Pt(layout.Margin)}\"" +
            $" fo:margin-left=\"{Pt(layout.Margin)}\" fo:margin-right=\"{Pt(layout.Margin)}\"/>" +
            "</style:page-layout></office:automatic-styles>" +
            "<office:master-styles><style:master-page style:name=\"Standard\" style:page-layout-name=\"pm1\"/></office:master-styles>" +
            "</office:document-styles>";

        const string manifest =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
            "<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" manifest:version=\"1.2\">" +
            "<manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.2\" manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>" +
            "<manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/xml\"/>" +
            "<manifest:file-entry manifest:full-path=\"styles.xml\" manifest:media-type=\"text/xml\"/>" +
            "</manifest:manifest>";

        // mimetype musí být první a nekomprimovaný (ODF 1.2, část 3, 3.3)
        WriteZip(path, new[]
        {
            ("mimetype", "application/vnd.oasis.opendocument.text", false),
            ("META-INF/manifest.xml", manifest, true),
            ("content.xml", content, true),
            ("styles.xml", styles, true),
        });
    }

    /// <summary>
    /// ODF slučuje mezery; víc mezer za sebou a mezera na začátku odstavce se
    /// musí zapsat jako &lt;text:s text:c="n"/&gt;.
    /// </summary>
    private static void AppendOdfText(StringBuilder sb, string text, ref bool lineStart)
    {
        int i = 0;
        while (i < text.Length)
        {
            if (text[i] == ' ')
            {
                int n = 1;
                while (i + n < text.Length && text[i + n] == ' ') n++;
                if (n == 1 && !lineStart) sb.Append(' ');
                else sb.Append("<text:s text:c=\"").Append(n).Append("\"/>");
                i += n;
            }
            else
            {
                int start = i;
                while (i < text.Length && text[i] != ' ') i++;
                sb.Append(Xml(text.Substring(start, i - start)));
            }
            lineStart = false;
        }
    }

    // ------------------------------------------------------------------ společné

    private static string Xml(string s)
    {
        var sb = new StringBuilder(s.Length);
        foreach (char c in s)
        {
            switch (c)
            {
                case '&': sb.Append("&amp;"); break;
                case '<': sb.Append("&lt;"); break;
                case '>': sb.Append("&gt;"); break;
                case '"': sb.Append("&quot;"); break;
                default:
                    if (c >= 32 || c == '\t') sb.Append(c);   // řídicí znaky XML nepřipouští
                    break;
            }
        }
        return sb.ToString();
    }

    private static void WriteZip(string path, (string Name, string Content, bool Compress)[] entries)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using var file = new FileStream(path, FileMode.Create, FileAccess.Write);
        using var zip = new SimpleZip(file);
        foreach (var (name, content, compress) in entries) zip.Add(name, Utf8.GetBytes(content), compress);
    }
}
