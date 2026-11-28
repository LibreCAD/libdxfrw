// SPDX-License-Identifier: GPL-2.0-or-later
// Diagnostic only: observe whether an independent ACadSharp build retains a
// selected DWG entity as a typed RTEXT. UnknownEntity is NOT semantic proof.
using ACadSharp.Entities;
using ACadSharp.IO;
using System.Globalization;

if (args.Length != 2 || !ulong.TryParse(args[1], NumberStyles.AllowHexSpecifier,
        CultureInfo.InvariantCulture, out ulong handle))
{
    Console.Error.WriteLine("usage: acadsharp_rtext_probe DRAWING.dwg HANDLE_HEX");
    return 2;
}
try
{
    using var reader = new DwgReader(args[0]);
    reader.Configuration.KeepUnknownEntities = true;
    reader.OnNotification += (_, e) =>
        Console.Error.WriteLine($"NOTICE {e.NotificationType}: {e.Message}");
    var document = reader.Read();
    var entity = document.GetCadObject(handle);
    if (entity == null)
    {
        Console.WriteLine($"HANDLE {handle:X} MISSING; SEMANTIC_EVIDENCE=NO");
        return 3;
    }
    Console.WriteLine($"HANDLE {handle:X} TYPE={entity.GetType().FullName}");
    if (entity is UnknownEntity)
    {
        Console.WriteLine("RTEXT_RETAINED_AS_UNKNOWN; SEMANTIC_EVIDENCE=NO");
        return 3;
    }
    Console.WriteLine("TYPED_OBJECT_REQUIRES_SEPARATE_FIELD_ADMISSION; SEMANTIC_EVIDENCE=NO");
    return 3;
}
catch (Exception exception)
{
    Console.Error.WriteLine($"READ FAILED {exception.GetType().FullName}: {exception.Message}");
    return 1;
}
