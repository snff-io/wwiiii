# List of container IDs to stop and remove
container_ids=(
    "10861249b29f"
    "6ab2f0510ef3"
    "62ec4e24ea6c"
    "7093b24fd7d3"
    "16b44566794b"
    "7ebcd4f90e74"
    "bca5905edd63"
)

# Iterate over each container ID, stop and remove it
for container_id in "${container_ids[@]}"; do
    echo "Stopping container: $container_id"
    docker stop "$container_id"
    echo "Removing container: $container_id"
    docker rm "$container_id"
done

images=(
    "3b6b6298a27c"
    "1cee7704beaf"
    "ccfd067290d3"
    "c3d35069696e"
    "228469c40a7e"
    "4345772e539e"
    "532e6bae5946"
    "b9876b4a5664"
)

for img in "${images[@]}"; do 
    echo "Removing Image: $img"
    docker rmi $img -f
done