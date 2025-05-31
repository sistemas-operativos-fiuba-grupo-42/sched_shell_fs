#!/bin/bash

# Test script for fisopfs
# Run in the project root after compiling with `make`

# Configuration
MOUNT_POINT="prueba"
FILEDISK="test.fisopfs"
FS_BINARY="./fisopfs"

# Function to clean up mount point exists
mkdir $MOUNT_POINT

# Function to start the filesystem
start_filesystem() {
    rm -r $MOUNT_POINT
    rm -rf ../$FILEDISK

    mkdir -p $MOUNT_POINT
    $FS_BINARY $MOUNT_POINT --filedisk $FILEDISK
    sleep 2
    if mount | grep -q $MOUNT_POINT; then
        echo "Filesystem mounted successfully"
    else
        echo "Failed to mount filesystem"
        exit 1
    fi
}

start_filesystem_persistence() {
    rm -rf $MOUNT_POINT
    mkdir -p $MOUNT_POINT
    $FS_BINARY $MOUNT_POINT --filedisk $FILEDISK
    sleep 2
    if mount | grep -q $MOUNT_POINT; then
        echo "Filesystem mounted successfully"
    else
        echo "Failed to mount filesystem"
        exit 1
    fi
}

# Function to stop the filesystem
stop_filesystem() {
    if mount | grep -q $MOUNT_POINT; then
        umount $MOUNT_POINT
        sleep 2
        echo "Filesystem unmounted"
    else
        echo "Filesystem was not mounted"
    fi
}

# Test 1: File creation
test_file_creation() {
    echo "Test 1: File creation"
    start_filesystem
    cd $MOUNT_POINT
    touch file1.txt
    echo "Hello" > file2.txt
    if [ -f file1.txt ] && [ -f file2.txt ]; then
        echo -e "\e[32m PASS \e[37m : Files created successfully"
    else
        echo -e "\e[31m FAIL \e[37m : File creation failed"
    fi
    cat file2.txt
    if [ "$(cat file2.txt)" = "Hello" ]; then
        echo -e "\e[32m PASS \e[37m : File content correct"
    else
        echo -e "\e[31m FAIL \e[37m : File content incorrect"
    fi
    cd ..
    stop_filesystem
}

# Test 2: File reading
test_file_reading() {
    echo "Test 2: File reading"
    start_filesystem
    cd $MOUNT_POINT
    echo "Test content" > test.txt
    cat test.txt
    if [ "$(cat test.txt)" = "Test content" ]; then
        echo -e "\e[32m PASS \e[37m : File read correctly"
    else
        echo -e "\e[31m FAIL \e[37m : File read failed"
    fi
    cd ..
    stop_filesystem
}

# Test 3: File writing (truncate and append)
test_file_writing() {
    echo "Test 3: File writing"
    start_filesystem
    cd $MOUNT_POINT
    echo "Initial" > write.txt
    echo "Appended" >> write.txt
    cat write.txt
    if [ "$(cat write.txt)" = "Initial
Appended" ]; then
        echo -e "\e[32m PASS \e[37m : Append write successful"
    else
        echo -e "\e[31m FAIL \e[37m : Append write failed"
    fi
    echo "Truncated" > write.txt
    if [ "$(cat write.txt)" = "Truncated" ]; then
        echo -e "\e[32m PASS \e[37m : Truncate write successful"
    else
        echo -e "\e[31m FAIL \e[37m : Truncate write failed"
    fi
    cd ..
    stop_filesystem
}

# Test 4: File deletion
test_file_deletion() {
    echo "Test 4: File deletion"
    start_filesystem
    cd $MOUNT_POINT
    touch file.txt
    rm file.txt
    if [ ! -f file.txt ]; then
        echo -e "\e[32m PASS \e[37m: File deleted successfully"
    else
        echo -e "\e[31m FAIL \e[37m: File deletion failed"
    fi
    cd ..
    stop_filesystem
}

# Test 5: Directory creation
test_dir_creation() {
    echo "Test 5: Directory creation"
    start_filesystem
    cd $MOUNT_POINT
    mkdir dir1
    if [ -d dir1 ]; then
        echo -e "\e[32m PASS \e[37m: Directory created successfully"
    else
        echo -e "\e[31m FAIL \e[37m : Directory creation failed"
    fi
    cd ..
    stop_filesystem
}

# Test 6: Directory listing
test_dir_listing() {
    echo "Test 6: Directory listing"
    start_filesystem
    cd $MOUNT_POINT
    mkdir dir1
    touch dir1/file1.txt
    ls -al dir1
    if ls -al dir1 | grep -q "\.$\|\.\.$"; then
        echo -e "\e[32m PASS \e[37m: Directory listing includes . and .."
    else
        echo -e "\e[31m FAIL \e[37m : Directory listing missing . or .."
    fi
    cd ..
    stop_filesystem
}

# Test 7: Directory deletion
test_dir_deletion() {
    echo "Test 7: Directory deletion"
    start_filesystem
    cd $MOUNT_POINT
    mkdir dir1
    rmdir dir1
    if [ ! -d dir1 ]; then
        echo -e "\e[32m PASS \e[37m: Empty directory deleted successfully"
    else
        echo -e "\e[31m FAIL \e[37m : Empty directory deletion failed"
    fi
    mkdir dir2
    touch dir2/file.txt
    rmdir dir2 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "\e[32m PASS \e[37m: Non-empty directory deletion failed as expected"
    else
        echo -e "\e[31m FAIL \e[37m : Non-empty directory deletion succeeded unexpectedly"
    fi
    cd ..
    stop_filesystem
}


# Test 8: Binary file handling
test_binary_handling() {
    echo "Test 8: Binary file handling"
    start_filesystem
    cd $MOUNT_POINT
    echo -ne "\x00\xFF\xAA" > binary.bin
    if [ "$(od -tx1 binary.bin | head -n1 | awk '{print $2 $3 $4}')" = "00ffaa" ]; then
        echo -e "\e[32m PASS \e[37m: Binary file handled correctly"
    else
        echo -e "\e[31m FAIL \e[37m : Binary file handling failed"
    fi
    cd ..
    stop_filesystem
}

# Test 9: Persistence
test_persistence() {
    echo "Test 9: Persistence"
    start_filesystem
    cd $MOUNT_POINT
    echo "Persistent data" > persist.txt
    cd ..
    stop_filesystem
    start_filesystem_persistence
    cd $MOUNT_POINT
    if [ "$(cat persist.txt)" = "Persistent data" ]; then
        echo -e "\e[32m PASS \e[37m: Data persisted across mounts"
    else
        echo -e "\e[31m FAIL \e[37m : Data not persisted"
    fi
    cd ..
    stop_filesystem
}

# Run all tests
test_file_creation
echo -e "\n"

test_file_reading
echo -e "\n"

test_file_writing
echo -e "\n"

test_file_deletion
echo -e "\n"

test_dir_creation
echo -e "\n"

test_dir_listing
echo -e "\n"

test_dir_deletion
echo -e "\n"

test_binary_handling
echo -e "\n"

test_persistence
echo -e "\n"

# Clean up
rm -f ../$FILEDISK
rmdir $MOUNT_POINT