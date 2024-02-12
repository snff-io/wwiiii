az container create \
    --resource-group container_city \
    --name wwivcnt \
    --image containercityregistry.azurecr.io/docker-image-wwiv:caq  \
    --vnet "/subscriptions/c3f8ae7e-c3c4-422e-8695-41fd0509302c/resourceGroups/container_city/providers/Microsoft.Network/virtualNetworks/container_city_vnet" \
    --subnet "default" \
    --registry-username containercityregistry \
    --registry-password "4YtwuG3cGctQXuJWOcdU1WLYCu5EteDOFgnuoOoQH1+ACRD9eZgv"