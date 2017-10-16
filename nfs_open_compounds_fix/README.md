### Modified NFS (nfs_open_compound_fix)

Patch for Linux VFS framework and NFSv4 client module. 

With this patch NFS will send compound lookup requests, 
significantly improving performance of open(2) and similar syscalls (stat(2), access(2)...).

This is very WIP.  We only know that this can handle some synthetic workloads without kernel panic (and we do not even know what happens on other types of workload).

Do not try this in production environment!!!!

Current version of the patch is written for stock CentOS 7 kernel.

Instruction: 

1. Download kernel source from Centos 7 repository

2. Unpack

3. Replace original files with corresponding files from this directory

4. Build and install using standard kernel package build procedure

## ToDo

1.  Convert this to patch.

2.  Port to newer kernel versions

3.  Do proper testing
