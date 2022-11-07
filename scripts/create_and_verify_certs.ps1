Write-Host "Login to your Azure account`n"

az login 

Write-Host "`n================================================================================"
[string]$hubName = $(Read-Host "Insert Azure Iot Hub name")
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Generating root certificate...`n"
python .\ca_create_root.py
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Generating signer certificate signing request...`n"
python .\ca_create_signer_csr.py
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Generating signer certificate...`n"
python .\ca_create_signer.py
Write-Host "================================================================================"

Rename-Item -Path ".\signer-ca.crt" -NewName "signer-ca.cer"

[string]$certificateList = (az iot hub certificate list --hub-name $hubName --query value[].name)
Write-Host "The following certificates exist on $hubName"
Write-Host $certificateList

Write-Host "`n================================================================================"
[string]$signerCertName = $(Read-Host "Insert an unique name for signer certificate")
Write-Host "================================================================================"
if($certificateList -match $signerCertName)
{
	"Certificate already exists is your IoT Hub"
	return
}

az iot hub certificate create --hub-name $hubName --name $signerCertName --path .\signer-ca.cer

[string]$certificateEtag = (az iot hub certificate show --hub-name $hubName --name $signerCertName --query etag)
[string]$verificationCode = (az iot hub certificate generate-verification-code --hub-name $hubName --name $signerCertName --etag $certificateEtag --query properties.verificationCode)

python .\generate_verification.py $verificationCode

[string]$certificateEtag = (az iot hub certificate show --hub-name $hubName --name $signerCertName --query etag)
az iot hub certificate verify --hub-name $hubName --name $signerCertName --path .\signer-ca-verification.cer --etag $certificateEtag


