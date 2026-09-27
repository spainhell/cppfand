using System.IO;
using System.IO.Compression;
using System.Text;

namespace WpfHost;

/// <summary>
/// Minimální zapisovač zipu pro DOCX a ODT. ZipArchive v .NET Frameworku ukládá
/// i CompressionLevel.NoCompression metodou deflate, ale ODF vyžaduje, aby
/// soubor mimetype byl první a uložený metodou stored (bez komprese).
/// </summary>
internal sealed class SimpleZip : IDisposable
{
    private readonly Stream _out;
    private readonly List<(string Name, uint Crc, uint Compressed, uint Size, ushort Method, uint Offset)> _entries = new();

    public SimpleZip(Stream output) { _out = output; }

    public void Add(string name, byte[] data, bool compress)
    {
        byte[] stored = data;
        ushort method = 0;
        if (compress)
        {
            using var ms = new MemoryStream();
            using (var deflate = new DeflateStream(ms, CompressionLevel.Optimal, leaveOpen: true))
                deflate.Write(data, 0, data.Length);
            stored = ms.ToArray();
            method = 8;
        }
        uint crc = Crc32(data);
        uint offset = (uint)_out.Position;
        byte[] nameBytes = Encoding.UTF8.GetBytes(name);

        var w = new BinaryWriter(_out, Encoding.UTF8, leaveOpen: true);
        w.Write(0x04034b50u);            // local file header
        w.Write((ushort)20);             // verze pro rozbalení
        w.Write((ushort)0x0800);         // UTF-8 názvy
        w.Write(method);
        w.Write((ushort)0);              // čas
        w.Write((ushort)0x21);           // datum 1980-01-01
        w.Write(crc);
        w.Write((uint)stored.Length);
        w.Write((uint)data.Length);
        w.Write((ushort)nameBytes.Length);
        w.Write((ushort)0);              // extra
        w.Write(nameBytes);
        w.Write(stored);
        w.Flush();
        _entries.Add((name, crc, (uint)stored.Length, (uint)data.Length, method, offset));
    }

    public void Dispose()
    {
        var w = new BinaryWriter(_out, Encoding.UTF8, leaveOpen: true);
        uint start = (uint)_out.Position;
        foreach (var e in _entries)
        {
            byte[] nameBytes = Encoding.UTF8.GetBytes(e.Name);
            w.Write(0x02014b50u);        // central directory header
            w.Write((ushort)20);         // verze, která vytvořila
            w.Write((ushort)20);
            w.Write((ushort)0x0800);
            w.Write(e.Method);
            w.Write((ushort)0);
            w.Write((ushort)0x21);
            w.Write(e.Crc);
            w.Write(e.Compressed);
            w.Write(e.Size);
            w.Write((ushort)nameBytes.Length);
            w.Write((ushort)0);          // extra
            w.Write((ushort)0);          // komentář
            w.Write((ushort)0);          // disk
            w.Write((ushort)0);          // interní atributy
            w.Write(0u);                 // externí atributy
            w.Write(e.Offset);
            w.Write(nameBytes);
        }
        uint size = (uint)_out.Position - start;
        w.Write(0x06054b50u);            // end of central directory
        w.Write((ushort)0);
        w.Write((ushort)0);
        w.Write((ushort)_entries.Count);
        w.Write((ushort)_entries.Count);
        w.Write(size);
        w.Write(start);
        w.Write((ushort)0);
        w.Flush();
    }

    private static readonly uint[] CrcTable = Enumerable.Range(0, 256).Select(n =>
    {
        uint c = (uint)n;
        for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        return c;
    }).ToArray();

    private static uint Crc32(byte[] data)
    {
        uint c = 0xFFFFFFFFu;
        foreach (byte b in data) c = CrcTable[(c ^ b) & 0xFF] ^ (c >> 8);
        return c ^ 0xFFFFFFFFu;
    }
}
