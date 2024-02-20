#!/bin/sh
Echo "entrypoint.sh..."

PATH=/opt/wwiv:$PATH
export PATH		

chown -R wwiv:wwiv .

HOME=/srv/wwiv exec /sbin/runuser -p -u wwiv -- "$@"


