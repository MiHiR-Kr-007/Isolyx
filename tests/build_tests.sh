cd "$(dirname "$0")/.."

echo "Making test binaries and putting in rootfs_base"
for test_src in tests/*.cpp; do
    filename=$(basename -- "$test_src")
    test_name="${filename%.*}"
    
    echo "Compiling $test_name..."
    g++ -static "$test_src" -o "rootfs_base/$test_name"
    
    if [ $? -eq 0 ]; then
        echo "  -> Successfully placed in rootfs_base/$test_name"
    else
        echo "  -> Failed to compile $test_name"
    fi
done

echo "All tests built."
