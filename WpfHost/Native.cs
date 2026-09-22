using System.Runtime.InteropServices;
using System.Text;

namespace WpfHost;

/// <summary>P/Invoke na exporty cppfandlib.dll (viz DynamicLibrary/dllmain.cpp a Drivers/host.h).</summary>
public static class Native
{
    private const string Dll = "cppfandlib.dll";

    [StructLayout(LayoutKind.Sequential)]
    public struct ScreenInfo
    {
        public int Cols;
        public int Rows;
        public int CursorX;       // 0-based
        public int CursorY;       // 0-based
        public int CursorVisible;
        public int CursorSize;    // 1 = normální, 50 = velký (přepis)
        public int Running;
        public int FieldX;        // zvýrazněné pole v prohlížecím režimu, 0-based; -1 = žádné
        public int FieldY;
        public int FieldLen;
    }

    /// <summary>Musí odpovídat FandHost::FieldEditRequest (10× int, char[256], char[64], char).</summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FieldEditRequest
    {
        public int X;
        public int Y;
        public int Width;
        public int MaxLen;
        public int FieldType;
        public int Pos;
        public int InsertMode;
        public int Star;
        public int DelOnFirstKey;
        public int TimeoutMs;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)] public byte[] Text;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)] public byte[] Mask;
        public byte Attr;
    }

    /// <summary>Hodnoty FieldType z fandio/FieldDescr.h.</summary>
    public enum FieldType { Unknown = 0, Fixed = 1, Alfanum = 2, Numeric = 3, Date = 4, Text = 5, Bool = 6, Real = 7 }

    /// <summary>Musí odpovídat struct FandTextEditInfo v DynamicLibrary/dllmain.cpp.</summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct TextEditInfo
    {
        public int Mode;
        public int TextType;
        public int Pos;
        public int Scroll;
        public int Scrolling;
        public int ReadOnly;
        public int TextLength;      // bajty CP852
        public int BreakKeyCount;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)] public byte[] ColKey;
        public byte TxtColor;
        public byte BlockColor;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)] public byte[] Name;
    }

    /// <summary>Hodnoty EditorMode z TextEditor/TextEditor.h.</summary>
    public enum EditorMode
    {
        Unknown = 0, Normal = 1, Text = 2, Help = 3, View = 4, Edit = 5,
        FrameSingle = 6, FrameDouble = 7, DeleteFrame = 8, NotFrame = 9,
    }

    /// <summary>Hodnoty TextType z TextEditor/TextEditor.h.</summary>
    public enum TextKind { Unknown = 0, File = 1, Local = 2, Memo = 3 }

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    /// <summary>mode = "D" ladicí běh, "T" editace textového souboru, "" běžné spuštění.</summary>
    public static extern int FandStart(string fandDir, string workDir, string rdbName, string mode);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandSetScreenSize(int cols, int rows);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandIsRunning();

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandExitCode();

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandLastError(StringBuilder buffer, int capacity);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandStop();

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandWait(int timeoutMs);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong FandGetScreen([Out] ushort[] cells, int capacity, out ScreenInfo info);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong FandScreenVersion();

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandPushKey(ushort virtualKey, ushort scanCode, ushort unicodeChar, uint controlKeyState, int keyDown);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandPushMouse(int x, int y, uint buttonState, uint eventFlags, uint controlKeyState);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandSetFieldEditHost(int enabled);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandPollFieldEdit(out FieldEditRequest request);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandGetCurrentField(out int x, out int y, out int len, byte[] textCp852, int capacity);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandCompleteFieldEdit(byte[] textCp852, int pos, int insertMode, ushort key);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandSetTextEditHost(int enabled);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandPollTextEdit(out TextEditInfo info);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandGetTextEditText([Out] byte[] buffer, int capacity);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern int FandGetTextEditBreakKeys([Out] ushort[] buffer, int capacity);

    [DllImport(Dll, CallingConvention = CallingConvention.Cdecl)]
    public static extern void FandCompleteTextEdit(byte[] textCp852, int textLength, int pos, int scroll, int updated,
        ushort key, byte[]? wordCp852, int wordLength);

    public static string LastError()
    {
        var sb = new StringBuilder(512);
        FandLastError(sb, sb.Capacity);
        return sb.ToString();
    }

    // dwControlKeyState (KEY_EVENT_RECORD)
    public const uint RightAltPressed = 0x0001;
    public const uint LeftAltPressed = 0x0002;
    public const uint RightCtrlPressed = 0x0004;
    public const uint LeftCtrlPressed = 0x0008;
    public const uint ShiftPressed = 0x0010;
    public const uint EnhancedKey = 0x0100;

    // dwButtonState / dwEventFlags (MOUSE_EVENT_RECORD)
    public const uint FromLeft1stButtonPressed = 0x0001;
    public const uint RightmostButtonPressed = 0x0002;
    public const uint MouseMoved = 0x0001;
}
