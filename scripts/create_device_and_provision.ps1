Write-Host "Please log in to your Azure portal"

Write-Host "`n================================================================================"
[string]$hubName = $(Read-Host "`nInsert the Azure Iot Hub name where you want to create the device")
Write-Host "================================================================================"

[string]$COMport = $(Read-Host "Insert the COM port of the device")
Write-Host "================================================================================"

[string] $commonName = (python .\provision.py $COMport |Select-String "Done provisioning device")

Write-Host "`n$commonName`n"

$commonName = $commonName.ToString().substring(25)

az iot hub device-identity create -n $hubName -d $commonName --am x509_ca