# Navigate to the source directory
wwiv_source_root=$(find / -name "WWIV_SOURCE_ROOT" 2>/dev/null | head -n 1) 
wwiv_source_root="$(dirname "$(readlink -f "$wwiv_source_root")")"
cd "$wwiv_source_root/build"

make bbs
cp bbs/bbs /opt/wwiv