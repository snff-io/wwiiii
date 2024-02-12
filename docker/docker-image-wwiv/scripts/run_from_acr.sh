# Authenticate with Azure
az login

# Log in to Azure Container Registry
az acr login --name containercityregistry

# Pull the Docker image from ACR
docker pull containercityregistry.azurecr.io/docker-image-wwiv:cap

# Run a container from the pulled image
docker run -it containercityregistry.azurecr.io/docker-image-wwiv:cap -p 23:2323 -p 2323:2323 /bin/bash