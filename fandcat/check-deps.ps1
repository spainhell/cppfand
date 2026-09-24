<#
.SYNOPSIS
    Lists dependencies of the fandio library on the rest of CppFand.

.DESCRIPTION
    1. Builds fandcat, which links fandio.lib with /WHOLEARCHIVE and nothing
       else except fandbase and Logging. Every unresolved external symbol reported by the
       linker is a symbol fandio needs from another project.
    2. Optionally (default) builds the other static libraries and looks up in
       their object files where each missing symbol is defined.
    3. Scans fandio sources for #include of headers outside fandio.
    4. Writes the result to fandcat\DEPENDENCIES.md.

    When fandcat links successfully, fandio has no dependencies left.

.PARAMETER NoAttribution
    Skip building the rest of the solution; the report then lists symbols
    without the file that defines them.
#>
param(
    [string]$Configuration = 'Debug',
    [string]$Platform = 'x64',
    [switch]$NoAttribution
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = & $vswhere -latest -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
$dumpbin = & $vswhere -latest -find 'VC\Tools\MSVC\**\bin\Hostx64\x64\dumpbin.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found' }

function Invoke-Build([string]$project) {
    $log = [IO.Path]::GetTempFileName()
    & $msbuild (Join-Path $root $project) "-p:Configuration=$Configuration" "-p:Platform=$Platform" `
        "-p:SolutionDir=$root\" -m -nologo -v:m -clp:ErrorsOnly | Out-File -Encoding utf8 $log
    $code = $LASTEXITCODE
    $lines = Get-Content $log
    Remove-Item $log
    return @{ ExitCode = $code; Lines = $lines }
}

function Format-Symbol([string]$s) {
    $s = $s -replace 'class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >', 'std::string'
    $s = $s -replace '\b(class|struct|enum) ', ''
    $s = $s -replace '__cdecl |__ptr64', ''
    $s = $s -replace '^(public|protected|private): ', ''
    return $s.Trim()
}

# --- 1. link fandcat --------------------------------------------------------

Write-Host "Building fandcat ($Configuration|$Platform)..."
$build = Invoke-Build 'fandcat\fandcat.vcxproj'

$compileErrors = $build.Lines | Where-Object { $_ -match ': (fatal )?error C\d+' }
if ($compileErrors) {
    $compileErrors | Write-Host
    throw 'fandcat has compile errors, fix them first'
}

# symbol (mangled) -> @{ Name; Users = set of fandio objects (fandbase ones prefixed "fandbase/") }
$missing = @{}
$rx = '^(?:(?<lib>fandio|fandbase)\.lib\()?(?<obj>[^()\s]+?)\.obj\)? : error LNK20(01|19): unresolved external symbol (?:"(?<dem>.+?)" \((?<mang>[^()\s]+)\)|(?<plain>\S+))'
foreach ($line in $build.Lines) {
    $m = [regex]::Match($line, $rx)
    if (-not $m.Success) { continue }
    $mang = if ($m.Groups['mang'].Success) { $m.Groups['mang'].Value } else { $m.Groups['plain'].Value }
    $name = if ($m.Groups['dem'].Success) { Format-Symbol $m.Groups['dem'].Value } else { $mang }
    if (-not $missing.ContainsKey($mang)) {
        $missing[$mang] = @{ Name = $name; Users = New-Object 'System.Collections.Generic.SortedSet[string]' }
    }
    $user = $m.Groups['obj'].Value
    if ($m.Groups['lib'].Value -eq 'fandbase') { $user = "fandbase/$user" }
    [void]$missing[$mang].Users.Add($user)
}

if ($build.ExitCode -ne 0 -and $missing.Count -eq 0) {
    $build.Lines | Write-Host
    throw 'fandcat build failed for another reason than unresolved symbols'
}

# --- 2. where are the symbols defined --------------------------------------

# mangled symbol -> "Project/File"
$definedIn = @{}
if (-not $NoAttribution -and $missing.Count -gt 0) {
    Write-Host 'Building the rest of the solution for symbol lookup...'
    $core = Invoke-Build 'Core\Core.vcxproj'
    if ($core.ExitCode -ne 0) {
        $core.Lines | Write-Host
        throw 'Core build failed'
    }

    $intDir = if ($Platform -eq 'Win32') { $Configuration } else { "$Platform\$Configuration" }
    $projects = Get-ChildItem $root -Directory | Where-Object {
        $_.Name -notin @('fandio', 'fandbase', 'fandcat', 'Logging') -and (Test-Path (Join-Path $_.FullName "$($_.Name).vcxproj"))
    }
    foreach ($p in $projects) {
        $objDir = Join-Path $p.FullName $intDir
        if (-not (Test-Path $objDir)) { continue }
        foreach ($obj in Get-ChildItem $objDir -Filter *.obj) {
            foreach ($l in & $dumpbin /symbols /nologo $obj.FullName) {
                # e.g. "01A 00000000 SECT7  notype ()    External     | ?RunError@@YAXG@Z (void __cdecl RunError(unsigned short))"
                if ($l -notmatch '\bSECT\w+\s.*\bExternal\s+\|\s+(\S+)') { continue }
                $sym = $Matches[1]
                if ($missing.ContainsKey($sym) -and -not $definedIn.ContainsKey($sym)) {
                    $definedIn[$sym] = "$($p.Name)/$($obj.BaseName)"
                }
            }
        }
    }
}

# --- 3. includes outside fandio (fandbase and Logging are allowed) ------

$includes = @{}   # header -> set of fandio files
foreach ($f in Get-ChildItem (Join-Path $root 'fandio') -File | Where-Object { $_.Extension -in '.cpp', '.h' }) {
    foreach ($l in Get-Content $f.FullName) {
        if ($l -match '^\s*#\s*include\s+"\.\./(?!fandio/|fandbase/|Logging/)([^"]+)"') {
            $h = $Matches[1]
            if (-not $includes.ContainsKey($h)) { $includes[$h] = New-Object 'System.Collections.Generic.SortedSet[string]' }
            [void]$includes[$h].Add($f.Name)
        }
    }
}

# --- 4. report ---------------------------------------------------------------

$sb = New-Object System.Text.StringBuilder
function Add([string]$s = '') { [void]$sb.AppendLine($s) }

Add '# fandio dependencies'
Add
Add "Generated by ``fandcat\check-deps.ps1`` ($Configuration|$Platform). Do not edit by hand."
Add
Add 'fandcat links `fandio.lib` with `/WHOLEARCHIVE` and only `fandbase.lib` and `Logging.lib`.'
Add 'Each symbol below is used by fandio but defined elsewhere in CppFand.'
Add 'The goal is an empty list.'
Add
Add "- unresolved symbols: **$($missing.Count)**"
Add "- headers included from outside fandio: **$($includes.Count)**"
Add

if ($missing.Count -gt 0) {
    $groups = $missing.Keys | Group-Object { if ($definedIn.ContainsKey($_)) { $definedIn[$_] } else { '(unknown)' } } |
        Sort-Object @{ Expression = 'Count'; Descending = $true }, Name

    Add '## Summary by defining file'
    Add
    Add '| Defined in | Symbols | Used from fandio |'
    Add '|---|---:|---|'
    foreach ($g in $groups) {
        $users = New-Object 'System.Collections.Generic.SortedSet[string]'
        foreach ($k in $g.Group) { $users.UnionWith($missing[$k].Users) }
        Add "| $($g.Name) | $($g.Count) | $(($users -join ', ')) |"
    }
    Add

    Add '## Symbols'
    foreach ($g in $groups) {
        Add
        Add "### $($g.Name)"
        Add
        foreach ($k in ($g.Group | Sort-Object { $missing[$_].Name })) {
            Add "- ``$($missing[$k].Name)`` - $(($missing[$k].Users -join ', '))"
        }
    }
    Add
}

if ($includes.Count -gt 0) {
    Add '## Headers from outside fandio'
    Add
    Add '| Header | Included from |'
    Add '|---|---|'
    foreach ($h in ($includes.Keys | Sort-Object)) {
        Add "| $h | $(($includes[$h] -join ', ')) |"
    }
}

$out = Join-Path $PSScriptRoot 'DEPENDENCIES.md'
[IO.File]::WriteAllText($out, ($sb.ToString() -replace "`r?`n", "`r`n"), (New-Object System.Text.UTF8Encoding($false)))

if ($build.ExitCode -eq 0) {
    Write-Host 'fandcat links: fandio has no dependencies on the rest of CppFand.'
} else {
    Write-Host "$($missing.Count) unresolved symbols, $($includes.Count) external headers. See $out"
}
