#!/bin/sh
echo "Additional entrypoint commands executed"
echo "Container name: $HOSTNAME"
echo "docker exec -it $HOSTNAME /bin/sh"
sleep 300
# Additional commands here
sh
