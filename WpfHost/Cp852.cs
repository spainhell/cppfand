using System.Text;

namespace WpfHost;

/// <summary>Převod mezi bajty obrazovky FANDu (CP852, řídicí znaky jako grafika DOSu) a Unicode.</summary>
internal static class Cp852
{
    private static readonly Encoding Encoding;
    private static readonly char[] ToCharTable = new char[256];

    // znaky 0..31 a 127 se v DOSu zobrazují jako grafické symboly (FAND je používá: šipky, ► ◄ ...)
    private static readonly char[] ControlGlyphs =
    {
        ' ', '☺', '☻', '♥', '♦', '♣', '♠', '•',
        '◘', '○', '◙', '♂', '♀', '♪', '♫', '☼',
        '►', '◄', '↕', '‼', '¶', '§', '▬', '↨',
        '↑', '↓', '→', '←', '∟', '↔', '▲', '▼',
    };

    static Cp852()
    {
#if NETCOREAPP
        // .NET (Core) nema kodove stranky vestavene; .NET Framework je ma
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
#endif
        Encoding = Encoding.GetEncoding(852);
        var one = new byte[1];
        for (int b = 0; b < 256; b++)
        {
            if (b < 32) { ToCharTable[b] = ControlGlyphs[b]; continue; }
            if (b == 127) { ToCharTable[b] = '⌂'; continue; }
            one[0] = (byte)b;
            ToCharTable[b] = Encoding.GetString(one)[0];
        }
    }

    public static char ToChar(byte b) => ToCharTable[b];

    /// <summary>Text z pole s nulovým ukončením (jak ho posílá C++).</summary>
    public static string Decode(byte[] cString)
    {
        int len = Array.IndexOf(cString, (byte)0);
        if (len < 0) len = cString.Length;
        var sb = new StringBuilder(len);
        for (int i = 0; i < len; i++) sb.Append(ToCharTable[cString[i]]);
        return sb.ToString();
    }

    /// <summary>Unicode → CP852 s nulovým ukončením; nepřevoditelné znaky jako '?'.</summary>
    public static byte[] EncodeZ(string text)
    {
        var bytes = Encoding.GetBytes(text);
        var result = new byte[bytes.Length + 1];
        Array.Copy(bytes, result, bytes.Length);
        return result;
    }

    public static byte EncodeChar(char c)
    {
        var bytes = Encoding.GetBytes(new[] { c });
        return bytes.Length > 0 ? bytes[0] : (byte)'?';
    }
}
