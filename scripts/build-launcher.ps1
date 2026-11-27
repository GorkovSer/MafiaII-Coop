param([Parameter(Mandatory=$true)][string]$Toolchain,[string]$OutputDirectory='build/launcher')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$toolRoot=if([IO.Path]::IsPathRooted($Toolchain)){$Toolchain}else{[IO.Path]::GetFullPath((Join-Path $root $Toolchain))}
$cc=Join-Path $toolRoot 'bin/i686-w64-mingw32-clang.exe'
$windres=Join-Path $toolRoot 'bin/i686-w64-mingw32-windres.exe'
$out=Join-Path $root $OutputDirectory
New-Item -ItemType Directory -Path $out -Force | Out-Null
Add-Type -AssemblyName System.Drawing
# Original geometric M / II mark, generated as build resources; no game assets.
$frames=@()
foreach($size in @(16,32,48,256)) {
    $bitmap=New-Object Drawing.Bitmap($size,$size)
    $graphics=[Drawing.Graphics]::FromImage($bitmap)
    $stream=New-Object IO.MemoryStream
    $white=New-Object Drawing.SolidBrush([Drawing.Color]::FromArgb(247,246,241))
    $red=New-Object Drawing.SolidBrush([Drawing.Color]::FromArgb(157,39,53))
    try {
        $graphics.SmoothingMode='AntiAlias';$graphics.Clear([Drawing.Color]::FromArgb(19,20,23))
        $graphics.ScaleTransform($size/64.0,$size/64.0)
        $points=[Drawing.PointF[]]@((New-Object Drawing.PointF(9,48)),(New-Object Drawing.PointF(9,16)),
            (New-Object Drawing.PointF(15,16)),(New-Object Drawing.PointF(24,30)),
            (New-Object Drawing.PointF(33,16)),(New-Object Drawing.PointF(39,16)),
            (New-Object Drawing.PointF(39,48)),(New-Object Drawing.PointF(32,48)),
            (New-Object Drawing.PointF(32,29)),(New-Object Drawing.PointF(24,41)),
            (New-Object Drawing.PointF(16,29)),(New-Object Drawing.PointF(16,48)))
        $graphics.FillPolygon($white,$points)
        $graphics.FillRectangle($red,44,16,5,32);$graphics.FillRectangle($red,53,16,5,32)
        $bitmap.Save($stream,[Drawing.Imaging.ImageFormat]::Png)
        $frames+=,@{size=$size;bytes=$stream.ToArray()}
    } finally {$white.Dispose();$red.Dispose();$stream.Dispose();$graphics.Dispose();$bitmap.Dispose()}
}
$icon=Join-Path $out 'launcher.ico'
$file=[IO.File]::Create($icon);$writer=New-Object IO.BinaryWriter($file)
try {
    $writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$frames.Count)
    $offset=6+16*$frames.Count
    foreach($frame in $frames) {
        $edge=if($frame.size -eq 256){0}else{$frame.size}
        $writer.Write([byte]$edge);$writer.Write([byte]$edge);$writer.Write([byte]0);$writer.Write([byte]0)
        $writer.Write([uint16]1);$writer.Write([uint16]32)
        $writer.Write([uint32]$frame.bytes.Length);$writer.Write([uint32]$offset)
        $offset+=$frame.bytes.Length
    }
    foreach($frame in $frames){$writer.Write([byte[]]$frame.bytes)}
} finally {$writer.Dispose();$file.Dispose()}
$manifest=(Join-Path $root 'src/launcher/frontend.manifest').Replace('\','/')
$rc=Join-Path $out 'frontend.rc';$object=Join-Path $out 'frontend-resource.o'
[IO.File]::WriteAllText($rc,('1 ICON "'+$icon.Replace('\','/')+'"'+"`r`n"+'1 24 "'+$manifest+'"'),[Text.Encoding]::UTF8)
& $windres -i $rc -o $object
if($LASTEXITCODE -ne 0){throw 'Frontend resource compilation failed'}
& $cc -std=c11 -O2 -Wall -Wextra -Werror -static -municode -mwindows (Join-Path $root 'src/launcher/frontend.c') $object -o (Join-Path $out 'MafiaII-Coop.exe') -luser32
if($LASTEXITCODE -ne 0){throw 'Frontend compilation failed'}
Write-Output (Join-Path $out 'MafiaII-Coop.exe')
