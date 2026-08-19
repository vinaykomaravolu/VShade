$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$output = Split-Path -Parent $MyInvocation.MyCommand.Path
$size = 128

function Save-Icon([string]$name, [scriptblock]$draw) {
    $bitmap = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::Transparent)
    & $draw $graphics
    $graphics.Dispose()
    $bitmap.Save((Join-Path $output $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

function New-Brush([int]$r, [int]$g, [int]$b) {
    return New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, $r, $g, $b))
}

Save-Icon "folder.png" {
    param($g)
    $tab = New-Brush 196 148 48
    $body = New-Brush 232 191 74
    $shade = New-Brush 176 132 40
    $g.FillRectangle($tab, 28, 42, 34, 16)
    $g.FillRectangle($body, 24, 50, 80, 50)
    $g.FillRectangle($shade, 24, 78, 80, 22)
    $tab.Dispose(); $body.Dispose(); $shade.Dispose()
}

Save-Icon "file.png" {
    param($g)
    $paper = New-Brush 210 214 220
    $fold = New-Brush 168 174 184
    $line = New-Brush 138 143 152
    $g.FillRectangle($paper, 40, 24, 48, 80)
    $points = @(
        (New-Object System.Drawing.Point 70, 24),
        (New-Object System.Drawing.Point 88, 24),
        (New-Object System.Drawing.Point 88, 42)
    )
    $g.FillPolygon($fold, $points)
    $g.FillRectangle($line, 48, 52, 32, 6)
    $g.FillRectangle($line, 48, 66, 32, 6)
    $g.FillRectangle($line, 48, 80, 24, 6)
    $paper.Dispose(); $fold.Dispose(); $line.Dispose()
}

Save-Icon "texture.png" {
    param($g)
    $panel = New-Brush 42 64 102
    $sun = New-Brush 255 214 102
    $far = New-Brush 122 168 214
    $near = New-Brush 91 141 239
    $g.FillRectangle($panel, 28, 36, 72, 56)
    $g.FillEllipse($sun, 76, 44, 16, 16)
    $g.FillPolygon($far, @(
        (New-Object System.Drawing.Point 32, 88),
        (New-Object System.Drawing.Point 58, 52),
        (New-Object System.Drawing.Point 78, 88)
    ))
    $g.FillPolygon($near, @(
        (New-Object System.Drawing.Point 62, 88),
        (New-Object System.Drawing.Point 86, 58),
        (New-Object System.Drawing.Point 96, 88)
    ))
    $panel.Dispose(); $sun.Dispose(); $far.Dispose(); $near.Dispose()
}

function Draw-Cube($g, $cx, $cy, $scale, $topRgb, $leftRgb, $rightRgb) {
    $w = 24 * $scale
    $h = 14 * $scale
    $p0 = New-Object System.Drawing.Point ([int]$cx), ([int]($cy - $h))
    $p1 = New-Object System.Drawing.Point ([int]($cx + $w)), ([int]($cy - $h * 0.5))
    $p2 = New-Object System.Drawing.Point ([int]$cx), ([int]$cy)
    $p3 = New-Object System.Drawing.Point ([int]($cx - $w)), ([int]($cy - $h * 0.5))
    $p4 = New-Object System.Drawing.Point ([int]($cx - $w)), ([int]($cy + $h * 0.5))
    $p5 = New-Object System.Drawing.Point ([int]$cx), ([int]($cy + $h))
    $p6 = New-Object System.Drawing.Point ([int]($cx + $w)), ([int]($cy + $h * 0.5))
    $topBrush = New-Brush @topRgb
    $leftBrush = New-Brush @leftRgb
    $rightBrush = New-Brush @rightRgb
    $g.FillPolygon($topBrush, @($p0, $p1, $p2, $p3))
    $g.FillPolygon($leftBrush, @($p3, $p2, $p5, $p4))
    $g.FillPolygon($rightBrush, @($p1, $p6, $p5, $p2))
    $topBrush.Dispose(); $leftBrush.Dispose(); $rightBrush.Dispose()
}

Save-Icon "model.png" {
    param($g)
    Draw-Cube $g 64 66 1.55 @(154, 224, 162) @(72, 148, 86) @(107, 203, 119)
}

Save-Icon "audio.png" {
    param($g)
    $bar = New-Brush 181 123 255
    $heights = @(18, 34, 52, 30, 22)
    $x = 36
    foreach ($height in $heights) {
        $g.FillRectangle($bar, $x, (64 - $height), 8, (2 * $height))
        $x += 14
    }
    $bar.Dispose()
}

Save-Icon "scene.png" {
    param($g)
    $panel = New-Brush 28 58 62
    $sun = New-Brush 255 214 122
    $far = New-Brush 64 150 142
    $near = New-Brush 78 205 196
    $g.FillRectangle($panel, 28, 36, 72, 56)
    $g.FillEllipse($sun, 78, 42, 16, 16)
    $g.FillPolygon($far, @(
        (New-Object System.Drawing.Point 32, 88),
        (New-Object System.Drawing.Point 56, 50),
        (New-Object System.Drawing.Point 76, 88)
    ))
    $g.FillPolygon($near, @(
        (New-Object System.Drawing.Point 60, 88),
        (New-Object System.Drawing.Point 88, 54),
        (New-Object System.Drawing.Point 96, 88)
    ))
    $panel.Dispose(); $sun.Dispose(); $far.Dispose(); $near.Dispose()
}

Save-Icon "prefab.png" {
    param($g)
    Draw-Cube $g 60 70 1.35 @(255, 198, 128) @(176, 108, 48) @(240, 160, 90)
    $badgeBack = New-Brush 40 42 48
    $badge = New-Brush 255 214 150
    $g.FillEllipse($badgeBack, 74, 32, 24, 24)
    $g.FillEllipse($badge, 78, 36, 16, 16)
    $badgeBack.Dispose(); $badge.Dispose()
}

Write-Output "Wrote icons to $output"
