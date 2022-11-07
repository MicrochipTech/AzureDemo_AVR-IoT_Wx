az login

Write-Host "`nInstalling necessary azure extions"
az extension add --name azure-cli-iot-ext

Write-Host "`n================================================================================"
[string]$hubName=(Read-Host "Insert your IoT Hub name")
Write-Host "================================================================================`n"

[string]$connectionString=(az iot hub show-connection-string --hub-name $hubName --policy-name service --query connectionString)
[string]$resourceGroupName=(az iot hub show --name $hubName --query resourcegroup)

Write-Host "`n================================================================================"
Write-Host "All consumer group names for $hubName hub:"
az iot hub consumer-group list --hub-name $hubName --query [].name
[string]$consumerGroupName=(Read-Host "Insert an existing Consumer Group name from above or a new one")
Write-Host "================================================================================"
az iot hub consumer-group create --hub-name $hubName --name $consumerGroupName

Write-Host "`n================================================================================"
Write-Host "All App Service Plan names:"
az appservice plan list --resource-group $resourceGroupName --query [].name
[string]$appServicePlanName=(Read-Host "Insert an existing App Service Plan name from above or a new one")
Write-Host "================================================================================"
az appservice plan create --name $appServicePlanName --resource-group $resourceGroupName --sku FREE

Write-Host "`n================================================================================"
[string]$webAppName=(Read-Host "Insert an App name; This will be a public name and must be unique!!!")
Write-Host "================================================================================"
az webapp create -n $webAppName -g $resourceGroupName -p $appServicePlanName --deployment-local-git
az webapp config appsettings set -n $webAppName -g $resourceGroupName --settings EventHubConsumerGroup=$consumerGroupName IotHubConnectionString=$connectionString
az webapp config set -n $webAppName -g $resourceGroupName --web-sockets-enabled true
az webapp update -n $webAppName -g $resourceGroupName --https-only true

Write-Host "`n================================================================================"
Write-Host "Current Web App deployment user:"
az webapp deployment user show --query publishingUserName
[string]$webAppAdminUserName=(Read-Host "Insert the current Web App deployment user or switch to a new one")
Write-Host "================================================================================"
az webapp deployment user set --user-name $webAppAdminUserName

Write-Host "`n================================================================================"
git clone https://bitbucket.microchip.com/scm/~m18029/azure_web_app_data_visualization.git
cd azure_web_app_data_visualization
[string]$gitRepoURL=(az webapp deployment source config-local-git -n $webAppName -g $resourceGroupName --query url)
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Creating a branch for the Web App and pushing the code"
git remote add webapp $gitRepoURL
git push webapp master:master
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Web Application state"
az webapp show -n $webAppName -g $resourceGroupName --query state
Write-Host "================================================================================"

Write-Host "`n================================================================================"
Write-Host "Opening Web Application"
az webapp browse -g $resourceGroupName -n $webAppName
Write-Host "================================================================================"