#include "tarfs.h"

static unsigned unix_types[] = {8, 0, 0, 2, 6, 4, 0, 0};

int is_entry_of(char *entry, char *parent){
	int i;
	int namelen, len = strlen(parent);
	for(i = 0; i < len; ++i){
		if (entry[i] != parent[i])
			return false;
	}
	namelen = strlen(entry);
	for (i = len; i < namelen - 1; ++i)
		if (entry[i] == '/')
			return false;
	return true;
}

static int tarfs_readdir(struct file *file, struct dir_context *ctx){
        int len, ent_len, i;
        unsigned type;
        struct super_block *sb;
        struct buffer_head *bh;	
        struct tar_inode *inode = (struct tar_inode*)file->f_inode;
        char name[100];

        if (ctx->pos == 1)
        	return 0;
        sb = file->f_inode->i_sb;
	    if (inode->offset > 0){
	        bh = sb_bread(sb, inode->offset);
	        strncpy(name, bh->b_data, 100);
	        len = strlen(name);
        	brelse(bh);
    	} else {
    		len = 0;
    		name[0] = 0;
    	}

        for(i = 0;;++i){
		bh = sb_bread(sb, i);
		if (*(bh->b_data) == 0){
			break;
		}
		
		ent_len = strlen(bh->b_data);
		if (ent_len > len && is_entry_of(bh->b_data, name)){
			type = bh->b_data[156] - '0' > 0 ? unix_types[bh->b_data[156] - '0'] : unix_types[0];
			//pr_debug("tarfs: put \"%s\" with type %u\n", bh->b_data + len, type);
			if (!dir_emit(ctx, bh->b_data + len, ent_len - len, i + 1, type)){
				brelse(bh);
				return 0;
			}
		}
		i += get_blocks_count(read_octal_int(bh->b_data + 124));
		brelse(bh);
	}
	ctx->pos = 1;
        return 0;
}

static struct dentry* tarfs_lookup(struct inode *dir, struct dentry *dent, unsigned flags){
	struct buffer_head *bh;
	struct tar_inode *inode;
	struct inode *i;
	char name[100];
	int len = 0;
	pr_debug("tarfs: LOOKUP============ %s\n", dent->d_name.name);
	inode = (struct tar_inode*)dir;
	if (inode->offset >= 0){
		bh = sb_bread(dir->i_sb, inode->offset);
		strncpy(name, bh->b_data, 100);
		len = strlen(name);
		pr_debug("tarfs: parent name: %s\n", name);
		brelse(bh);
	}
	else{
		pr_debug("tarfs: parent is root\n");
		name[0] = 0;
	}
	strncat(name, dent->d_name.name, 99 - len);
	pr_debug("tarfs: full name: %s\n", name);

	i = create_inode_by_name(dir->i_sb, name);		
	d_add(dent, i);
	if (!i){
		pr_debug("tarfs: inode not found\n");
		return ERR_PTR(-ENOENT);	
	}
	pr_debug("tarfs: inode found\n");
	return NULL;
}

const struct file_operations dir_fops = {
    .llseek = generic_file_llseek,
    .read = generic_read_dir,
    .iterate = tarfs_readdir,
};

const struct inode_operations dir_ops = {
    .lookup = tarfs_lookup,
};
