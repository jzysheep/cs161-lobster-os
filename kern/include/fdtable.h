#include <fdfile.h>
#include <spinlock.h>

#define FD_MAX 256

struct fd_table {
        struct fd_file *fdt_table[FD_MAX];
        struct spinlock fdt_spinlock;
};

struct fd_table * fd_table_create(void);
void fd_table_destroy(struct fd_table *fd_table);

/* Find a free pid in the a fd table and point it to the fd struct.
   Return the file descriptor if found; otherwise, return -1 */
int add_file_to_fd_table(struct fd_table *fd_table, struct fd_file *file);
struct fd_file * get_file_from_fd_table(struct fd_table *fd_table, int fd);
int release_fd_from_fd_table(struct fd_table *fd_table, int fd);
