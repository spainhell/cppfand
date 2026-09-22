using System.Text;

namespace WpfHost;

/// <summary>
/// Atributy textu PC-FANDu a prevod CP852 pro textovy editor.
///
/// Proti <see cref="Cp852"/> je rozdil v ridicich znacich: tam se bajty 0..31
/// prevadeji na graficke symboly DOSu, protoze jde o obsah obrazovky. Tady nesou
/// informaci o atributu, takze zustavaji ridicimi znaky a offsety v dokumentu
/// odpovidaji 1:1 offsetum ve FANDu.
/// </summary>
internal static class FandAttr
{
    private static readonly Encoding Encoding;
    private static readonly char[] ToCharTable = new char[256];

    static FandAttr()
    {
#if NETCOREAPP
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
#endif
        Encoding = Encoding.GetEncoding(852);
        var one = new byte[1];
        for (int b = 0; b < 256; b++)
        {
            if (b < 32) { ToCharTable[b] = (char)b; continue; }
            one[0] = (byte)b;
            ToCharTable[b] = Encoding.GetString(one)[0];
        }
    }

    public static string Decode(byte[] bytes, int length)
    {
        var sb = new StringBuilder(length);
        for (int i = 0; i < length; i++) sb.Append(ToCharTable[bytes[i]]);
        return sb.ToString();
    }

    public static byte[] Encode(string text)
    {
        var result = new byte[text.Length];
        for (int i = 0; i < text.Length; i++)
        {
            char c = text[i];
            result[i] = c < 32 ? (byte)c : Encoding.GetBytes(new[] { c })[0];
        }
        return result;
    }

    /// <summary>
    /// Prepinaci ridici znaky, jako TextEditor.h:249
    /// (CtrlKey = { '\x13','\x17','\x11','\x04','\x02','\x05','\x01' }).
    /// Index je do ColKey[], viz TextEditorScreen::Color().
    /// </summary>
    public static readonly (char Ch, int ColKeyIndex, string Name)[] Attributes =
    {
        ('\x13', 1, "^S podtržení"),
        ('\x17', 2, "^W kurzíva"),
        ('\x11', 3, "^Q dvojitá šířka"),
        ('\x04', 4, "^D dvojitý průchod"),
        ('\x02', 5, "^B zvýraznění"),
        ('\x05', 6, "^E zhuštěně"),
        ('\x01', 7, "^A elite"),
    };

    public const char PageBreak = '\x0C';   // konec stránky; ScrollWrline ho kreslí jako blok 219

    public static bool IsAttr(char c)
    {
        foreach (var a in Attributes) if (a.Ch == c) return true;
        return false;
    }

    public static int ColKeyIndexOf(char c)
    {
        foreach (var a in Attributes) if (a.ColKeyIndex > 0 && a.Ch == c) return a.ColKeyIndex;
        return 0;   // tCtrl -- ostatní řídicí znaky
    }

    public static string NameOf(char c)
    {
        foreach (var a in Attributes) if (a.Ch == c) return a.Name;
        return "^" + (char)(c + 64);
    }
}
