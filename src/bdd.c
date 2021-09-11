#include <stdlib.h>
#include <stdio.h>

#include "bdd.h"
#include "debug.h"
#include "bdd2.h"
#include "my_math.h"

/*
 * Macros that take a pointer to a BDD node and obtain pointers to its left
 * and right child nodes, taking into account the fact that a node N at level l
 * also implicitly represents nodes at levels l' > l whose left and right children
 * are equal (to N).
 *
 * You might find it useful to define macros to do other commonly occurring things;
 * such as converting between BDD node pointers and indices in the BDD node table.
 */
#define LEFT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->left)
#define RIGHT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->right)

int current_bdd_node_index = BDD_NUM_LEAVES;

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 *
 * The function aborts if the arguments passed are out-of-bounds.
 */
int bdd_lookup(int level, int left, int right) {
    if (left == right) {
        return left;
    }
    BDD_NODE new_node = {level, left, right};
    int index = hash_function(level, left, right);
    BDD_NODE curr_node;
    BDD_NODE **hash_map = bdd_hash_map;
    hash_map += index;
    while (*hash_map != NULL) {
        curr_node = **hash_map;
        if (curr_node.level == level && curr_node.left == left && curr_node.right == right) {
            return bdd_node_to_index(*hash_map);
        }
        hash_map++;
        if (hash_map >= (hash_map + BDD_HASH_SIZE)) {
            hash_map = bdd_hash_map;
        }
    }
    int insert_index = insert_node(new_node, current_bdd_node_index, bdd_nodes, hash_map);
    return insert_index;
}

int hash_function(int level, int left, int right) {
    int temp = level + left + right;
    return (temp % BDD_HASH_SIZE);
}

int insert_node(BDD_NODE node, int index, BDD_NODE *nodes_table, BDD_NODE **hash_map) {
    if (current_bdd_node_index >= BDD_NODES_MAX) {
        return -1;
    }
    nodes_table += index;
    *nodes_table = node;
    *hash_map = nodes_table;
    /* printf("Index: %d\tLevel: %d\tLeft: %d\tRight: %d\n", index, node.level, node.left, node.right); */
    current_bdd_node_index++;
    return index;
}

int bdd_node_to_index(BDD_NODE *node) {
    return node - bdd_nodes;
}

BDD_NODE* index_to_bdd_node(int index) {
    return bdd_nodes + index;
}





BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    int level = bdd_min_level(w, h);
    if (level > BDD_LEVELS_MAX) {
        return NULL;
    }
    int index = bdd_from_raster_recurse(w, h, raster, level, 0, power(2, level / 2), 0, power(2, level / 2));
    if (index == -1) {
        return NULL;
    }
    return index_to_bdd_node(index);
}

int bdd_min_level(int w, int h) {
    int value = 1;
    int level = 0;
    while (value < w || value < h) {
        value *= 2;
        level++;
    }
    return level * 2;
}

int power(int base, int exponent) {
    if (exponent == 0) {
        return 1;
    }
    int result = 1;
    for (; exponent > 0; exponent--) {
        result *= base;
    }
    return result;
}

int bdd_from_raster_recurse(int w, int h, unsigned char *raster, int curr_level, int row_min, int row_max, int col_min, int col_max) {
    /* printf("Level: %d\tRow min: %d\tRow max: %d\tCol min: %d\tCol max: %d\n", curr_level, row_min, row_max, col_min, col_max); */
    if (curr_level == 0) {
        if (row_min >= h || col_min >= w) { /* Outside of raster */
            return 0;
        } else {
            /* printf("Row: %d\tCol: %d\tValue: %d\n", row_min, col_min, *(raster + (row_min * w) + col_min)); */
            return *(raster + (row_min * w) + col_min);
        }
    }
    int left;
    int right;
    if (curr_level % 2 == 0) { /* Choose between top and bottom strips */
        left = bdd_from_raster_recurse(w, h, raster, curr_level - 1, row_min, (row_min + row_max) / 2, col_min, col_max);
        right = bdd_from_raster_recurse(w, h, raster, curr_level - 1, (row_min + row_max) / 2, row_max, col_min, col_max);
    } else { /* Choose between left and right sub-squares */
        left = bdd_from_raster_recurse(w, h, raster, curr_level - 1, row_min, row_max, col_min, (col_min + col_max) / 2);
        right = bdd_from_raster_recurse(w, h, raster, curr_level - 1, row_min, row_max, (col_min + col_max) / 2, col_max);
    }
    if (left == -1 || right == -1) {
        return -1;
    }
    return bdd_lookup(curr_level, left, right);
}






void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            *(raster + (row * w) + col) = bdd_apply(node, row, col);
        }
    }
}







int bdd_serialize(BDD_NODE *node, FILE *out) {
    clear_bdd_index_map();
    bdd_serialize_recurse(node, out, 1);
    return 0;
}

int bdd_serialize_recurse(BDD_NODE *node, FILE *out, int serial_num) {
    int node_index = bdd_node_to_index(node);
    if (*(bdd_index_map + node_index) != 0) { /* If node has already been serialized */
        return serial_num;
    }
    if (node_index <= 255) {
        fputc('@', out);
        fputc(node_index, out);
        *(bdd_index_map + node_index) = serial_num; /* Map node index to serial number */
        return serial_num + 1;
    }
    int new_serial_num;
    new_serial_num = bdd_serialize_recurse(index_to_bdd_node((node) -> left), out, serial_num);
    new_serial_num = bdd_serialize_recurse(index_to_bdd_node((node) -> right), out, new_serial_num);
    fputc(((node) -> level) + 64, out); /* Serialize level opcode */
    int left_serial = *(bdd_index_map + (node) -> left);
    int right_serial = *(bdd_index_map + (node) -> right);
    output_serial_number(left_serial, out);
    output_serial_number(right_serial, out);
    *(bdd_index_map + node_index) = new_serial_num; /* Map node index to serial number */
    return new_serial_num + 1;
}

void output_serial_number(int serial_num, FILE *out) {
    int mask = 0x000000FF;
    int curr_byte;
    int bits_to_shift = 0;
    for (int i = 0; i < 4; i++) {
        curr_byte = serial_num & mask;
        curr_byte >>= bits_to_shift;
        fputc(curr_byte, out);
        mask <<= 8;
        bits_to_shift += 8;
    }
    return;
}





BDD_NODE *bdd_deserialize(FILE *in) {
    clear_bdd_index_map();
    int serial_num = 1;
    int curr_char = fgetc(in);
    int bdd_node_index;
    while (curr_char != EOF) {
        if (curr_char == '@') {
            curr_char = fgetc(in);
            if (curr_char == EOF) {
                return NULL;
            }
            bdd_node_index = curr_char;
            *(bdd_index_map + serial_num) = curr_char; /* Map serial number to node index */
        } else if (curr_char >= 'A' && curr_char <= '`') {
            bdd_node_index = build_new_node(curr_char - 64, in);
            if (bdd_node_index == -1) {
                return NULL;
            }
            *(bdd_index_map + serial_num) = bdd_node_index; /* Map serial number to node index */
        } else {
            return NULL;
        }
        serial_num++;
        curr_char = fgetc(in);
    }
    return index_to_bdd_node(bdd_node_index);
}

int build_new_node(int level, FILE *in) {
    int left_serial = build_serial_num(in);
    if (left_serial == -1) {
        return -1;
    }
    int right_serial = build_serial_num(in);
    if (right_serial == -1) {
        return -1;
    }
    return bdd_lookup(level, *(bdd_index_map + left_serial), *(bdd_index_map + right_serial));
}

int build_serial_num(FILE *in) {
    int curr_char;
    int bits_to_shift = 0;
    int serial_num = 0;
    for (int j = 0; j < 4; j++) { /* Four bytes to read for each node */
        curr_char = fgetc(in);
        if (curr_char == EOF) {
            return -1;
        }
        curr_char <<= bits_to_shift;
        serial_num |= curr_char;
        bits_to_shift += 8;
    }
    return serial_num;
}





unsigned char bdd_apply(BDD_NODE *node, int r, int c) {
    int node_index = bdd_node_to_index(node);
    if (node_index <= 255) {
        return node_index;
    }
    int level = (node) -> level;
    int instructions = build_traversal_instructions(level / 2, r, c);
    return bdd_apply_recurse(node, level, instructions);
}

int build_traversal_instructions(int num_bits, int r, int c) {
    int result = 0;
    int bits_to_shift = 0;
    int bit_index = num_bits - 1;
    int mask = 0x1 << bit_index;
    int current_bit;
    while (bit_index >= 0) {
        current_bit = r & mask;
        current_bit >>= bit_index;
        current_bit <<= bits_to_shift;
        result |= current_bit;
        bits_to_shift++;
        current_bit = c & mask;
        current_bit >>= bit_index;
        current_bit <<= bits_to_shift;
        result |= current_bit;
        bits_to_shift++;
        bit_index--;
        mask >>= 1;
    }
    return result;
}

int bdd_apply_recurse(BDD_NODE *node, int level, int instructions) {
    if (level == 0) {
        return bdd_node_to_index(node);
    } else if (((node) -> level) < level) {
        /* printf("Skipped instruction at level %d: %d\n", level, instructions & 1); */
        return bdd_apply_recurse(node, level - 1, instructions >> 1);
    }
    int instruction = instructions & 1; /* Get rightmost instruction */
    /* printf("Instruction at level %d: %d\n", level, instruction); */
    if (instruction == 0) {
        return bdd_apply_recurse(index_to_bdd_node((node) -> left), level - 1, instructions >> 1);
    }
    return bdd_apply_recurse(index_to_bdd_node((node) -> right), level - 1, instructions >> 1);
}






BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    clear_bdd_index_map();
    int index = bdd_map_recurse(node, func);
    if (index == -1) {
        return NULL;
    }
    return index_to_bdd_node(index);
    /* return index_to_bdd_node(bdd_map_recurse(node, func)); */
}

int bdd_map_recurse(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    /* printf("\tIndex: %d\tLevel: %d\tLeft: %d\tRight: %d\n", bdd_node_to_index(node), (*node).level, (*node).left, (*node).right); */
    int bdd_node_index = bdd_node_to_index(node);
    if (*(bdd_index_map + bdd_node_index) != 0) { /* If node has already been mapped */
        return *(bdd_index_map + bdd_node_index);
    }
    if (bdd_node_index <= 255) {
        return func(bdd_node_index);
    }
    int left = bdd_map_recurse(index_to_bdd_node((node) -> left), func);
    int right = bdd_map_recurse(index_to_bdd_node((node) -> right), func);
    if (left == -1 || right == -1) {
        return -1;
    }
    int new_index = bdd_lookup((node) -> level, left, right);
    *(bdd_index_map + bdd_node_index) = new_index; /* Map old node to new node */
    return new_index;
}







BDD_NODE *bdd_rotate(BDD_NODE *node, int level) {
    if (level % 2 == 1) {
        level += 1;
    }
    int side_length = power(2, (level / 2));
    int root_index = bdd_rotate_recurse(node, level, 0, side_length, 0, side_length);
    if (root_index == -1) {
        return NULL;
    }
    return index_to_bdd_node(root_index);
}

int bdd_rotate_recurse(BDD_NODE *root, int level, int row_min, int row_max, int col_min, int col_max) {
    if (level == 0) {
        return bdd_apply(root, row_min, col_min);
    }
    int row_middle = (row_min + row_max) / 2;
    int col_middle = (col_min + col_max) / 2;
    int top_left = bdd_rotate_recurse(root, level - 2, row_min, row_middle, col_min, col_middle);
    int top_right = bdd_rotate_recurse(root, level - 2, row_min, row_middle, col_middle, col_max);
    int bot_left = bdd_rotate_recurse(root, level - 2, row_middle, row_max, col_min, col_middle);
    int bot_right = bdd_rotate_recurse(root, level - 2, row_middle, row_max, col_middle, col_max);
    if (top_left == -1 || top_right == -1 || bot_left == -1 || bot_right == -1) {
        return -1;
    }
    int left_index = bdd_lookup(level - 1, top_right, bot_right);
    int right_index = bdd_lookup(level - 1, top_left, bot_left);
    return bdd_lookup(level, left_index, right_index);
}





BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor) {
    clear_bdd_index_map();
    /* printf("Building zoom bdd for factor %d...\n", factor); */
    int new_root_index;
    if (factor < 0) {
        new_root_index = bdd_zoom_out(node, -factor);
    } else {
        new_root_index = bdd_zoom_in(node, factor);
    }
    if (new_root_index == -1) {
        return NULL;
    }
    return index_to_bdd_node(new_root_index);
}

int bdd_zoom_in(BDD_NODE *node, int factor) {
    int bdd_node_index = bdd_node_to_index(node);
    if (bdd_node_index <= 255) {
        return bdd_node_index;
    }
    if (*(bdd_index_map + bdd_node_index) != 0) { /* If node has already been mapped */
        return *(bdd_index_map + bdd_node_index);
    }
    int left = bdd_zoom_in(index_to_bdd_node((node) -> left), factor);
    int right = bdd_zoom_in(index_to_bdd_node((node) -> right), factor);
    if (left == -1 || right == -1) {
        return -1;
    }
    int new_index = bdd_lookup(((node) -> level) + (2 * factor), left, right);
    *(bdd_index_map + bdd_node_index) = new_index; /* Map old node to new node */
    return new_index;
}

int bdd_zoom_out(BDD_NODE *node, int factor) {
    int bdd_node_index = bdd_node_to_index(node);
    if (bdd_node_index <= 255) {
        return bdd_node_index;
    }
    if (((node) -> level) <= (2 * factor)) {
        if (bdd_node_index == 0) { /* If node covers an all black raster */
            return 0;
        } else {
            return 255;
        }
    }
    if (*(bdd_index_map + bdd_node_index) != 0) { /* If node has already been mapped */
        return *(bdd_index_map + bdd_node_index);
    }
    int left = bdd_zoom_out(index_to_bdd_node((node) -> left), factor);
    int right = bdd_zoom_out(index_to_bdd_node((node) -> right), factor);
    if (left == -1 || right == -1) {
        return -1;
    }
    int new_index = bdd_lookup(((node) -> level) - (2 * factor), left, right);
    *(bdd_index_map + bdd_node_index) = new_index; /* Map old node to new node */
    return new_index;
}


void clear_bdd_index_map() {
    int *map = bdd_index_map;
    for (int i = 0; i < BDD_NODES_MAX; i++) {
        *map = 0;
        map++;
    }
    return;
}
