
#ifndef BDD2_H
#define BDD2_H

int hash_function(int level, int left, int right);
int insert_node(BDD_NODE node, int index, BDD_NODE *nodes_table, BDD_NODE **hash_map);
int probe_from(int index, BDD_NODE **hash_map, BDD_NODE node);
int bdd_node_to_index(BDD_NODE *node);
BDD_NODE* index_to_bdd_node(int index);
int bdd_from_raster_recurse(int w, int h, unsigned char *raster, int curr_level, int row_min, int row_max, int col_min, int col_max);

int bdd_serialize_recurse(BDD_NODE *node, FILE *out, int serial_num);
void output_serial_number(int serial_num, FILE *out);

int build_new_node(int level, FILE *in);
int build_serial_num(FILE *in);

int build_traversal_instructions(int num_bits, int r, int c);
int bdd_apply_recurse(BDD_NODE *node, int level, int instructions);

int bdd_map_recurse(BDD_NODE *node, unsigned char (*func)(unsigned char));

int bdd_zoom_in(BDD_NODE *node, int factor);
int bdd_zoom_out(BDD_NODE *node, int factor);

int bdd_rotate_recurse(BDD_NODE *root, int level, int row_min, int row_max, int col_min, int col_max);

void clear_bdd_index_map();

#endif