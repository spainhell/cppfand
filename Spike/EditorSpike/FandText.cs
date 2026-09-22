using System.Text;
using System.Windows.Media;

namespace EditorSpike;

/// <summary>
/// Model textu PC-FANDu pro spike: prevod CP852 a tabulka atributu.
///
/// Rozdil proti WpfHost/Cp852.cs: tam se bajty 0..31 prevadeji na graficke
/// symboly DOSu, protoze jde o obsah obrazovky. Tady jsou to ridici znaky,
/// ktere nesou informaci o atributu, takze je nechavame beze zmeny -- diky
/// tomu offsety v dokumentu odpovidaji 1:1 offsetum ve FANDu.
/// </summary>
internal static class FandText
{
    private static readonly Encoding Cp852;
    private static readonly char[] ToChar = new char[256];

    static FandText()
    {
#if NETCOREAPP
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
#endif
        Cp852 = Encoding.GetEncoding(852);
        var one = new byte[1];
        for (int b = 0; b < 256; b++)
        {
            if (b < 32) { ToChar[b] = (char)b; continue; }   // ridici znak zustava ridicim znakem
            one[0] = (byte)b;
            ToChar[b] = Cp852.GetString(one)[0];
        }
    }

    public static string Decode(byte[] bytes)
    {
        var sb = new StringBuilder(bytes.Length);
        foreach (byte b in bytes) sb.Append(ToChar[b]);
        return sb.ToString();
    }

    public static byte[] Encode(string text) => Cp852.GetBytes(text);

    // 16 barev CGA/VGA, stejne jako WpfHost/TerminalControl.cs
    public static readonly Color[] Palette =
    {
        Color.FromRgb(0, 0, 0), Color.FromRgb(0, 0, 170), Color.FromRgb(0, 170, 0), Color.FromRgb(0, 170, 170),
        Color.FromRgb(170, 0, 0), Color.FromRgb(170, 0, 170), Color.FromRgb(170, 85, 0), Color.FromRgb(170, 170, 170),
        Color.FromRgb(85, 85, 85), Color.FromRgb(85, 85, 255), Color.FromRgb(85, 255, 85), Color.FromRgb(85, 255, 255),
        Color.FromRgb(255, 85, 85), Color.FromRgb(255, 85, 255), Color.FromRgb(255, 255, 85), Color.FromRgb(255, 255, 255),
    };

    public static readonly string[] PaletteNames =
    {
        "0 černá", "1 modrá", "2 zelená", "3 azurová", "4 červená", "5 purpurová", "6 hnědá", "7 světle šedá",
        "8 tmavě šedá", "9 jasně modrá", "10 jasně zelená", "11 jasně azurová", "12 jasně červená",
        "13 jasně purpurová", "14 žlutá", "15 bílá",
    };

    /// <summary>
    /// Prepinaci ridici znaky, presne jako TextEditor.h:249
    /// (CtrlKey = { '\x13','\x17','\x11','\x04','\x02','\x05','\x01' }).
    /// Index odpovida ColKey[] z TextEditor.cpp:3192 a prekladu v TextEditorScreen::Color().
    /// </summary>
    public sealed class Attr
    {
        public char Ch;
        public int ColKeyIndex;
        public string Name = "";
    }

    public static readonly Attr[] Attributes =
    {
        new Attr { Ch = '\x13', ColKeyIndex = 1, Name = "^S podtržení (tUnderline)" },
        new Attr { Ch = '\x17', ColKeyIndex = 2, Name = "^W kurzíva (tItalic)" },
        new Attr { Ch = '\x11', ColKeyIndex = 3, Name = "^Q dvojitá šířka (tDWidth)" },
        new Attr { Ch = '\x04', ColKeyIndex = 4, Name = "^D dvojitý průchod (tDStrike)" },
        new Attr { Ch = '\x02', ColKeyIndex = 5, Name = "^B zvýraznění (tEmphasized)" },
        new Attr { Ch = '\x05', ColKeyIndex = 6, Name = "^E zhuštěně (tCompressed)" },
        new Attr { Ch = '\x01', ColKeyIndex = 7, Name = "^A elite (tElite)" },
    };

    public const char PageBreak = '\x0C';   // konec stranky, ScrollWrline ho kresli jako blok 219

    public static bool IsAttr(char c)
    {
        foreach (var a in Attributes) if (a.Ch == c) return true;
        return false;
    }

    public static int ColKeyIndexOf(char c)
    {
        foreach (var a in Attributes) if (a.Ch == c) return a.ColKeyIndex;
        return 0;   // tCtrl -- ostatni ridici znaky
    }

    public static string NameOf(char c)
    {
        foreach (var a in Attributes) if (a.Ch == c) return a.Name;
        return "^" + (char)(c + 64);
    }
}
