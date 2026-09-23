#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <linux/fdtable.h>
#include <linux/slab.h>        
#include <linux/rbtree.h>      
#include <linux/proc_fs.h>  
#include <linux/seq_file.h>
#include <linux/fs.h>   
#include <linux/uaccess.h>
#include <linux/string.h>

static int pidstart;
static int pidcount;
MODULE_PARM_DESC(pidstart,"Pid_Start");
MODULE_PARM_DESC(pidcount,"Pid_Count");
module_param(pidstart, int, 0644);
module_param(pidcount, int, 0644);
struct data
{
    struct rb_node node;
    pid_t pid;
    long state;
    long vm_size;
    long num_files;
    long static_prio;
    int num_threads;
    char comm_name[TASK_COMM_LEN];
};
struct rb_root tree_root = RB_ROOT;
static void insert_node_rb(struct rb_node **node, struct data *new_node, struct rb_node *prev) {
    if (*node == NULL) {  
        rb_link_node(&new_node->node, prev, node); 
        rb_insert_color(&new_node->node, &tree_root);  
        return;
    }

    struct data *current_node_data = container_of(*node, struct data, node);

    if (new_node->pid > current_node_data->pid) {
        insert_node_rb(&((*node)->rb_right), new_node, *node);
    } else if (new_node->pid < current_node_data->pid) {
        insert_node_rb(&((*node)->rb_left), new_node, *node);
    }
}

static void insert_new_node_rb(struct data *new_node) {
    insert_node_rb(&(tree_root.rb_node), new_node, NULL);
}
/*static int insert_Nodes(struct rb_root *root, struct data *new_node)
{
    struct rb_node **current_node = &(root->rb_node), *previous_node =NULL;
    if(current_node == NULL)
    {
        rb_link_node(&new_node->node,previous_node,current_node);
        rb_insert_color(&new_node->node,root);
    }
    else
    {
        while(current_node)
        {
            struct data *this = container_of(*current_node,struct data,node);
            previous_node = *current_node;
            if(new_node->pid>this-> pid)
            current_node = &((*current_node)->rb_right);
            else if(new_node->pid<this-> pid)
            current_node = &((*current_node)->rb_left);
            else
            return -1;
        }
    }
    rb_link_node(&new_node->node,previous_node,current_node);
    rb_insert_color(&new_node->node,root);
    return 0;
}*/

static void redblack_inorder_traversal(struct rb_node *root){
    if(root == NULL){
        return;
    }
    redblack_inorder_traversal(root->rb_left);
    struct data *current_node = container_of(root,struct data,node);
    pr_info("pid: %d, state: %ld, virtual_memory_size: %ld, number_of_files_used: %ld, static_priority: %ld, number_of_threads: %d, command_name: %s\n",current_node->pid,current_node-> state,current_node->vm_size,current_node-> num_files,current_node->static_prio,current_node->num_threads,current_node->comm_name);
    redblack_inorder_traversal(root->rb_right);
}
static void inorder_rb_traverse(void){
    redblack_inorder_traversal(tree_root.rb_node);
}

static void free_rb_tree(struct rb_node *root){
    if(root==NULL){
        return;
    }
    free_rb_tree(root->rb_left);
    free_rb_tree(root->rb_right);
    struct data *current_node = container_of(root,struct data,node);
    rb_erase(root,&tree_root);
    kfree(current_node);
}

static void delete_tree(void){
    free_rb_tree(tree_root.rb_node);
}
static void load_info_rb(void)
{
    int pid_end = pidstart+pidcount-1;
    struct task_struct *tasks;
    for_each_process(tasks)
    {
        pid_t temp_pid = tasks->pid;
        if((tasks->tgid == tasks->pid) && (temp_pid>= pidstart && temp_pid<=pid_end))
        {
            struct data *storage = kmalloc(sizeof(*storage),GFP_KERNEL);
            storage->pid = tasks->pid;
            storage->state = task_state_index(tasks);
            if(tasks->mm)
            storage->vm_size = tasks->mm->total_vm;
            else
            storage->vm_size  = 0;
            storage->static_prio = tasks->static_prio;
            storage->num_threads = get_nr_threads(tasks);
            strscpy(storage->comm_name,tasks->comm,TASK_COMM_LEN);
            struct fdtable *f = files_fdtable(tasks->files);
            int count_f = 0;
            if(f && f->fd)
            {
                for(int i=0; i<f->max_fds; i++)
                {
                    if(f->fd[i]!=NULL)
                    {
                        count_f++;
                    }
                }
            }
            storage->num_files = count_f;
            insert_new_node_rb(storage);
            

        }


    }

}

static int inorder_show(struct seq_file *the_file, struct rb_node *root){
    if(root==NULL){
        return 0;
    }
    inorder_show(the_file,root->rb_left);
    struct data *current_node = container_of(root,struct data,node);
    seq_printf(the_file,"pid: %d, state: %ld, virtual_memory_size: %ld, number_of_files_used: %ld, static_priority: %ld, number_of_threads: %d, command_name: %s\n",current_node->pid,current_node-> state,current_node->vm_size,current_node-> num_files,current_node->static_prio,current_node->num_threads,current_node->comm_name);
    inorder_show(the_file,root->rb_right);
    return 0;
}
static int show(struct seq_file *thefile, void *nothing){
    return inorder_show(thefile, tree_root.rb_node);
}

static int open(struct inode *temp ,struct file *target_file){
    return single_open(target_file,show,NULL);
}
static const struct proc_ops proc_fops = {
       
        .proc_open    = open,
        .proc_read    = seq_read,
        .proc_lseek  = seq_lseek,
        .proc_release = single_release,
};




static int __init processinfo_init(void) {
    
    load_info_rb();
    pr_info("Checkpoint 1");
    inorder_rb_traverse();
    if(!proc_create("processinfo",0,NULL,&proc_fops))
    {
        pr_err("/proc/processinfo Failed \n");
        delete_tree();
        return -ENOMEM;
    }
    printk(KERN_INFO "processinfo Module Loaded\n");
    return 0;  
}


static void __exit processinfo_exit(void) {
    remove_proc_entry("processinfo",NULL);
    delete_tree();
    printk(KERN_INFO "processinfo Module Unloaded!\n");
}


module_init(processinfo_init);
module_exit(processinfo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Moin Khan, Ali Furkan Kaya, Berfin Cetinkaya");
MODULE_DESCRIPTION("Process Info using RB Tree");
MODULE_VERSION("1.0");

