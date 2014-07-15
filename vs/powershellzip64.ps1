[Reflection.Assembly]::LoadWithPartialName( "System.IO.Compression.FileSystem" )
$src_folder = "E:\simulationcraft\simc-601-alpha-win64"
$destfile = "E:\simulationcraft\simc-601-alpha-win64.zip"
$compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
$includebasedir = $false
remove-item -path $destfile
[System.IO.Compression.ZipFile]::CreateFromDirectory($src_folder,$destfile,$compressionLevel, $includebasedir )