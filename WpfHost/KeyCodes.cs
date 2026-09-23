using System.Windows.Input;

namespace WpfHost;

/// <summary>
/// Kódování kláves tak, jak je čeká cppfand (PressedKey::KeyCombination v Drivers/keyboard.cpp):
/// 0x8000 = neznaková klávesa (dolní byte = virtual key), jinak dolní byte = znak;
/// 0x0400 Alt, 0x0200 Ctrl, 0x0100 Shift.
/// </summary>
internal static class KeyCodes
{
    public const ushort Virtual = 0x8000;
    public const ushort Alt = 0x0400;
    public const ushort Ctrl = 0x0200;
    public const ushort Shift = 0x0100;

    public const ushort Enter = 13;
    public const ushort Esc = 27;
    public const ushort Tab = 9;
    public const ushort Backspace = 8;

    /// <summary>Znak, který konzole dodává k neznakové klávese (Enter, Esc, Tab, Backspace); 0 = žádný.</summary>
    public static ushort CharFor(Key key) => key switch
    {
        Key.Enter => 13,
        Key.Escape => 27,
        Key.Tab => 9,
        Key.Back => 8,
        _ => 0,
    };

    /// <summary>Mac nemá klávesu Insert, zastupuje ji F11 (i s modifikátory).</summary>
    public static Key MapInsert(Key key) => key == Key.F11 ? Key.Insert : key;

    public static bool IsNavigationOrFunction(Key key) => key is
        Key.F1 or Key.F2 or Key.F3 or Key.F4 or Key.F5 or Key.F6 or Key.F7 or Key.F8 or Key.F9 or Key.F10 or Key.F11 or Key.F12
        or Key.Left or Key.Right or Key.Up or Key.Down or Key.Home or Key.End or Key.PageUp or Key.PageDown
        or Key.Insert or Key.Delete;

    public static bool IsEnhanced(Key key) => key is
        Key.Left or Key.Right or Key.Up or Key.Down or Key.Home or Key.End or Key.PageUp or Key.PageDown
        or Key.Insert or Key.Delete;

    public static bool IsModifierKey(Key key) => key is
        Key.LeftShift or Key.RightShift or Key.LeftCtrl or Key.RightCtrl or Key.LeftAlt or Key.RightAlt
        or Key.LWin or Key.RWin or Key.CapsLock or Key.NumLock or Key.Scroll;

    /// <summary>Kombinace pro ukončovací klávesu editace pole (neznakové klávesy a Ctrl/Alt kombinace).</summary>
    public static ushort Encode(Key key, ModifierKeys mods)
    {
        ushort flags = 0;
        if ((mods & ModifierKeys.Shift) != 0) flags |= Shift;
        if ((mods & ModifierKeys.Control) != 0) flags |= Ctrl;
        if ((mods & ModifierKeys.Alt) != 0) flags |= Alt;

        ushort ch = CharFor(key);
        if (ch != 0)
        {
            // Enter/Esc/Tab/Backspace chodí jako znak; Shift u řídicího znaku zůstává (Shift+Tab)
            return (ushort)(flags | ch);
        }
        int vk = KeyInterop.VirtualKeyFromKey(key);
        return (ushort)(Virtual | flags | (vk & 0xFF));
    }
}
