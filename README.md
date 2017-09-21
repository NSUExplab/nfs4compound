# NFS v4 Compound

This repository contains 3 project.

### 1. TarFS

It is just a study project. It is file system driver for mounting tar files as read/only file system.

### 2. Test modules (rpc_exploits)

Kernel modules for checking possibility of making RPC compounds containing several lookup operations.

### 3. Modified NFS (nfs_open_compound_fix)

Patch for Linux VFS framework and NFSv4 client module. With this patch NFS will send compound lookup requests, significantly improving performance of open(2) and similas syscalls (stat(2), access(2)...).

Current version of the patch is written for stock CentOS 7 kernel.

This is work in progress, do not try this in production environment!!!
