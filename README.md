# NFS v4 Compound

This repository contains 3 project.

### 1. TarFS

It is just a study project. It is file system driver for mounting tar files as file system for reading.

### 2. Test modules (rpc_exploits)

It is kernel modules for checking possibility of making RPC compounds containing several lookup operations.

### 3. Modified NFS (nfs_open_compound_fix)

It is patch for NFS. With this patch NFS not sending request on each lookup during file search, but stores it in special list, creates a compound containing all lookups and sends it to server.